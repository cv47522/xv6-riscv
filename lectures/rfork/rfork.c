/*
 * rfork.c - parent side of remote fork.
 *
 * rfork() does three things:
 *
 *   1. Describe the parent's address space: the list of mappings from
 *      /proc/self/maps, the TLS pointer, and the callee-saved registers at
 *      the point where rfork() is about to return.
 *
 *   2. fork() a local "page server".  This is an ordinary copy-on-write fork,
 *      so the page server is a frozen snapshot of the parent's memory at the
 *      moment of the rfork, while the parent itself keeps running and changing
 *      its memory.  The page server sends the description to rforkd and then
 *      answers "give me page X" requests by copying its own memory.
 *
 *   3. Return to the parent right away.
 *
 * On the other machine, rforkd starts an empty process at the same addresses,
 * registers all of them with userfaultfd and jumps to the saved registers.
 * The first instruction faults, rforkd fetches that page from the page
 * server, and so on: the child demand-pages itself out of the parent.
 */
#define _GNU_SOURCE
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <asm/prctl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/personality.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <sys/wait.h>

#include "rfork.h"
#include "rfork_proto.h"

extern int rfork_save_context(struct rfork_ctx *ctx) __attribute__((returns_twice));

/* ------------------------------------------------------------------------ */

void rfork_init(char **argv)
{
    int p = personality(0xffffffff);
    if (p == -1 || (p & ADDR_NO_RANDOMIZE))
        return;                                 /* already randomisation-free */
    if (personality(p | ADDR_NO_RANDOMIZE) == -1)
        return;
    execv("/proc/self/exe", argv);              /* re-exec ourselves with ASLR off */
    /* exec failed: carry on with ASLR, it usually still works */
}

/* ------------------------------------------------------------------------ */

static int send_all(int fd, const void *p, size_t n)
{
    const char *c = p;
    while (n) {
        ssize_t r = send(fd, c, n, MSG_NOSIGNAL);
        if (r < 0) { if (errno == EINTR) continue; return -1; }
        c += r; n -= r;
    }
    return 0;
}

static int read_all(int fd, void *p, size_t n)
{
    char *c = p;
    while (n) {
        ssize_t r = read(fd, c, n);
        if (r < 0) { if (errno == EINTR) continue; return -1; }
        if (r == 0) return -1;                  /* EOF */
        c += r; n -= r;
    }
    return 0;
}

static int connect_server(void)
{
    const char *host = getenv("RFORK_HOST");
    const char *ps   = getenv("RFORK_PORT");
    int port = ps ? atoi(ps) : RFORK_PORT;
    if (!host) host = "127.0.0.1";

    struct sockaddr_in sa = { .sin_family = AF_INET, .sin_port = htons(port) };
    if (inet_pton(AF_INET, host, &sa.sin_addr) != 1) { errno = EINVAL; return -1; }

    int s = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (s < 0) return -1;
    if (connect(s, (struct sockaddr *)&sa, sizeof sa) < 0) { close(s); return -1; }
    int one = 1;
    setsockopt(s, IPPROTO_TCP, TCP_NODELAY, &one, sizeof one);  /* request/reply traffic */
    return s;
}

/*
 * Parse /proc/self/maps into regions: text, data, heap, stack, ...
 * The kernel-provided [vdso]/[vvar]/[vsyscall] pages are skipped: the child
 * process gets its own copy of those from its kernel, at the same address
 * (both programs are static binaries running with ASLR off).
 */
static int parse_maps(struct rfork_region *r, int max)
{
    FILE *f = fopen("/proc/self/maps", "r");
    if (!f)
        return -1;

    char line[512], perms[8];
    int n = 0;
    while (fgets(line, sizeof line, f)) {
        /* 00400000-00401000 r--p 00000000 08:02 1234   /home/me/demo */
        unsigned long start, end;
        int off;
        if (sscanf(line, "%lx-%lx %7s %*s %*s %*s%n", &start, &end, perms, &off) < 3)
            continue;
        char *name = line + off;
        while (*name == ' ') name++;
        name[strcspn(name, "\n")] = 0;

        if (name[0] == '[' && name[1] == 'v')   /* [vdso] [vvar] [vvar_vclock] [vsyscall] */
            continue;
        if (n == max) { fclose(f); errno = E2BIG; return -1; }

        const char *base = strrchr(name, '/');
        r[n].start = start;
        r[n].end   = end;
        r[n].prot  = (perms[0] == 'r' ? RFORK_PROT_READ  : 0) |
                     (perms[1] == 'w' ? RFORK_PROT_WRITE : 0) |
                     (perms[2] == 'x' ? RFORK_PROT_EXEC  : 0);
        snprintf(r[n].name, sizeof r[n].name, "%s", base ? base + 1 : name);
        n++;
    }
    fclose(f);
    return n;
}

/*
 * The page server: a copy-on-write snapshot of the parent.  Deliberately no
 * malloc/stdio in here: it must not disturb the memory it is serving.  (It
 * only writes to its own stack frames, which lie below the frame the child
 * resumes in.)
 */
static void __attribute__((noreturn))
serve_pages(int sock, const struct rfork_hdr *hdr, const struct rfork_region *regions)
{
    if (send_all(sock, hdr, sizeof *hdr) < 0 ||
        send_all(sock, regions, hdr->nregions * sizeof *regions) < 0)
        _exit(1);

    for (;;) {
        uint64_t addr;
        char page[RFORK_PAGE];

        if (read_all(sock, &addr, sizeof addr) < 0)
            _exit(0);                           /* connection closed: the child is done */
        memcpy(page, (void *)addr, RFORK_PAGE); /* the page, straight out of the snapshot */
        if (send_all(sock, page, RFORK_PAGE) < 0)
            _exit(1);
    }
}

/* ------------------------------------------------------------------------ */

int rfork(void)
{
    static struct rfork_region regions[RFORK_MAX_REGIONS];
    struct rfork_hdr hdr;

    int sock = connect_server();
    if (sock < 0)
        return -1;

    int n = parse_maps(regions, RFORK_MAX_REGIONS);
    if (n < 0) { close(sock); return -1; }

    hdr.magic    = RFORK_MAGIC;
    hdr.nregions = n;
    /* get fs segment register for thread local storage */
    if (syscall(SYS_arch_prctl, ARCH_GET_FS, &hdr.fs_base) < 0) { close(sock); return -1; }

    /*
     * From here on nothing may change the memory layout or this stack frame
     * until fork() has taken the snapshot.
     */
    if (rfork_save_context(&hdr.ctx) != 0)
        return 0;                       /* <-- the child wakes up here, on the other machine */

    pid_t pid = fork();                 /* snapshot of the parent = the page server */
    if (pid < 0) { close(sock); return -1; }
    if (pid == 0)
        serve_pages(sock, &hdr, regions);

    close(sock);
    return pid;                         /* the parent is free to go */
}

int rfork_wait(int handle)
{
    int status;
    return waitpid(handle, &status, 0) < 0 ? -1 : 0;
}
