/*
 * rforkd.c - the "other machine": accepts rfork requests and pages the child
 * in from the parent with userfaultfd.
 *
 * For every connection from a parent:
 *
 *   1. read the parent's description (registers, TLS, memory map);
 *   2. fork + exec rfork-stub with ASLR disabled, connected to us through a
 *      unix socketpair on its fd 3, and forward the description to it;
 *   3. receive the stub's userfaultfd (passed with SCM_RIGHTS);
 *   4. serve page faults: read a uffd_msg, ask the parent's page server for
 *      that page over TCP, install it in the child with UFFDIO_COPY;
 *   5. when the child exits, close the connection; the page server sees EOF.
 *
 * Usage: rforkd [-v] [port]   (default port 7777, stub next to this binary)
 *        -v  log what happens: the parent's memory map, every page fault as it is
 *            served, child start/exit.  Without it rforkd prints nothing but errors.
 */
#define _GNU_SOURCE
#include <errno.h>
#include <libgen.h>
#include <poll.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <linux/userfaultfd.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/ioctl.h>
#include <sys/personality.h>
#include <sys/socket.h>
#include <sys/wait.h>

#include "rfork_proto.h"

static char stub_path[4096];
static int verbose;

static int write_all(int fd, const void *p, size_t n)
{
    const char *c = p;
    while (n) {
        ssize_t r = write(fd, c, n);
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
        if (r == 0) { errno = ECONNRESET; return -1; }
        c += r; n -= r;
    }
    return 0;
}

/* receive one status byte plus a file descriptor from the stub */
static int recv_fd(int sock, unsigned char *status)
{
    struct iovec iov = { status, 1 };
    char cbuf[CMSG_SPACE(sizeof(int))];
    struct msghdr mh = { .msg_iov = &iov, .msg_iovlen = 1,
                         .msg_control = cbuf, .msg_controllen = sizeof cbuf };

    if (recvmsg(sock, &mh, 0) != 1)
        return -1;
    struct cmsghdr *cm = CMSG_FIRSTHDR(&mh);
    if (!cm || cm->cmsg_level != SOL_SOCKET || cm->cmsg_type != SCM_RIGHTS)
        return -1;
    int fd;
    memcpy(&fd, CMSG_DATA(cm), sizeof fd);
    return fd;
}

static const char *region_name(const struct rfork_region *r, int n, uint64_t addr)
{
    for (int i = 0; i < n; i++)
        if (addr >= r[i].start && addr < r[i].end)
            return r[i].name[0] ? r[i].name : "anon";
    return "?";
}

/* ask the parent's page server for one page */
static int fetch_page(int conn, uint64_t addr, void *buf)
{
    if (write_all(conn, &addr, sizeof addr) < 0) return -1;
    return read_all(conn, buf, RFORK_PAGE);
}

static void handle(int conn)
{
    static struct rfork_region regions[RFORK_MAX_REGIONS];
    static char page[RFORK_PAGE] __attribute__((aligned(RFORK_PAGE)));
    struct rfork_hdr hdr;
    int nfaults = 0;

    /* 1. the parent's description of itself */
    if (read_all(conn, &hdr, sizeof hdr) < 0 || hdr.magic != RFORK_MAGIC ||
        hdr.nregions > RFORK_MAX_REGIONS ||
        read_all(conn, regions, hdr.nregions * sizeof regions[0]) < 0) {
        fprintf(stderr, "rforkd: bad request\n");
        return;
    }
    if (verbose)
        printf("rforkd: rfork request: %lu regions, child resumes at rip=%#lx rsp=%#lx\n",
               hdr.nregions, hdr.ctx.rip, hdr.ctx.rsp);
    for (uint64_t i = 0; verbose && i < hdr.nregions; i++)
        printf("rforkd:   %#14lx-%#14lx %c%c%c %s\n", regions[i].start, regions[i].end,
               regions[i].prot & RFORK_PROT_READ  ? 'r' : '-',
               regions[i].prot & RFORK_PROT_WRITE ? 'w' : '-',
               regions[i].prot & RFORK_PROT_EXEC  ? 'x' : '-', regions[i].name);

    /* 2. an empty process that will become the child */
    int sp[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sp) < 0) { perror("socketpair"); return; }

    pid_t pid = fork();
    if (pid < 0) { perror("fork"); close(sp[0]); close(sp[1]); return; }
    if (pid == 0) {
        personality(ADDR_NO_RANDOMIZE);         /* same layout as the parent */
        if (dup2(sp[1], 3) < 0) _exit(127);
        if (sp[1] != 3) close(sp[1]);
        close(sp[0]);                           /* conn and the listen socket are CLOEXEC */
        execl(stub_path, "rfork-stub", (char *)NULL);
        perror("rforkd: exec rfork-stub");
        _exit(127);
    }
    close(sp[1]);

    if (write_all(sp[0], &hdr, sizeof hdr) < 0 ||
        write_all(sp[0], regions, hdr.nregions * sizeof regions[0]) < 0) {
        perror("rforkd: write to stub");
        goto done;
    }

    /* 3. the stub registered everything with userfaultfd and gives us the fd */
    unsigned char status;
    int uffd = recv_fd(sp[0], &status);
    if (uffd < 0) { fprintf(stderr, "rforkd: stub did not send a userfaultfd\n"); goto done; }
    if (verbose && status == RFORK_STUB_USER_MODE_ONLY)
        printf("rforkd: note: userfaultfd runs in UFFD_USER_MODE_ONLY mode "
               "(vm.unprivileged_userfaultfd=0, no CAP_SYS_PTRACE); fine for the demo, "
               "but a syscall touching a not-yet-fetched page would SIGBUS the child\n");
    if (verbose)
        printf("rforkd: child pid %d started, serving page faults\n", pid);

    /* 4. page the child in from the parent, one fault at a time */
    struct pollfd pf[2] = { { uffd, POLLIN, 0 }, { sp[0], POLLIN, 0 } };
    for (;;) {
        if (poll(pf, 2, -1) < 0) { if (errno == EINTR) continue; perror("poll"); break; }

        if (pf[0].revents & POLLIN) {
            struct uffd_msg msg;
            ssize_t n = read(uffd, &msg, sizeof msg);
            if (n == sizeof msg && msg.event == UFFD_EVENT_PAGEFAULT) {
                uint64_t addr = msg.arg.pagefault.address & ~(uint64_t)(RFORK_PAGE - 1);

                if (fetch_page(conn, addr, page) < 0) { perror("rforkd: fetch page"); break; }

                struct uffdio_copy cp = { .dst = addr, .src = (uint64_t)page,
                                          .len = RFORK_PAGE, .mode = 0 };
                if (ioctl(uffd, UFFDIO_COPY, &cp) < 0 && errno != EEXIST)
                    perror("rforkd: UFFDIO_COPY");
                nfaults++;
                if (verbose)
                    printf("rforkd: fault %3d at %#14lx (%s) <- page fetched from parent\n",
                           nfaults, addr, region_name(regions, hdr.nregions, addr));
            }
        }
        if (pf[1].revents & (POLLIN | POLLHUP)) {
            char b;
            if (read(sp[0], &b, 1) <= 0)
                break;                          /* child closed its end: it exited */
        }
    }
    close(uffd);

done:
    close(sp[0]);
    int wstatus = 0;
    waitpid(pid, &wstatus, 0);
    if (verbose)
        printf("rforkd: child pid %d exited with status %d after %d page faults\n", pid,
               WIFEXITED(wstatus) ? WEXITSTATUS(wstatus) : 128 + WTERMSIG(wstatus), nfaults);
    /* 5. closing conn (done by the caller) tells the page server it is finished */
}

int main(int argc, char **argv)
{
    int port = RFORK_PORT;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-v") == 0)
            verbose = 1;
        else if (argv[i][0] >= '0' && argv[i][0] <= '9')
            port = atoi(argv[i]);
        else {
            fprintf(stderr, "usage: %s [-v] [port]\n", argv[0]);
            return 2;
        }
    }

    setvbuf(stdout, NULL, _IOLBF, 0);
    signal(SIGPIPE, SIG_IGN);

    /* rfork-stub lives next to this binary */
    char exe[4096];
    ssize_t n = readlink("/proc/self/exe", exe, sizeof exe - 1);
    if (n < 0) { perror("readlink"); return 1; }
    exe[n] = 0;
    snprintf(stub_path, sizeof stub_path, "%s/rfork-stub", dirname(exe));
    if (access(stub_path, X_OK) < 0) { perror(stub_path); return 1; }

    int ls = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (ls < 0) { perror("socket"); return 1; }
    int one = 1;
    setsockopt(ls, SOL_SOCKET, SO_REUSEADDR, &one, sizeof one);
    struct sockaddr_in sa = { .sin_family = AF_INET, .sin_port = htons(port),
                              .sin_addr.s_addr = htonl(INADDR_ANY) };
    if (bind(ls, (struct sockaddr *)&sa, sizeof sa) < 0 || listen(ls, 8) < 0) {
        perror("bind/listen");
        return 1;
    }
    if (verbose)
        printf("rforkd: listening on port %d\n", port);

    for (;;) {
        int conn = accept4(ls, NULL, NULL, SOCK_CLOEXEC);
        if (conn < 0) { if (errno == EINTR) continue; perror("accept"); break; }
        setsockopt(conn, IPPROTO_TCP, TCP_NODELAY, &one, sizeof one);
        handle(conn);
        close(conn);
    }
    return 0;
}
