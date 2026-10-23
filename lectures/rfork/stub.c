/*
 * stub.c - the process that becomes the remote child.
 *
 * rforkd exec()s this program (with ASLR off) and hands it the parent's
 * memory map and registers on fd 3.  The stub then:
 *
 *   1. creates a userfaultfd for its own address space;
 *   2. mmaps every region of the parent at the same address, as empty
 *      anonymous memory (MAP_FIXED), and registers it with the userfaultfd in
 *      "missing page" mode, so the first touch of any page traps instead of
 *      producing a zero page;
 *   3. sends the userfaultfd to rforkd, which will fetch the pages from the
 *      parent and install them with UFFDIO_COPY;
 *   4. restores the parent's TLS pointer and callee-saved registers and jumps
 *      to the parent's saved return address, with rax = 1.
 *
 * From that moment on this process *is* the child: it executes the parent's
 * code, with the parent's libc and stack, but none of that is here yet.  The
 * first instruction fetch is already a page fault.
 *
 * This file is freestanding (no libc): libc would need a heap, TLS and a
 * stack, all of which would collide with the parent's image.  It is linked at
 * 0x20000, below the lowest address a normal program uses.
 */
#include <stdint.h>
#include <stddef.h>
#include <errno.h>
#include <fcntl.h>
#include <asm/prctl.h>
#include <linux/userfaultfd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/syscall.h>

#include "rfork_proto.h"

#ifndef UFFD_USER_MODE_ONLY
#define UFFD_USER_MODE_ONLY 1
#endif

#define CTRL_FD 3                       /* unix socket to rforkd */

extern char __executable_start[], _end[];   /* our own image, from the linker */

/* ----- raw system calls -------------------------------------------------- */

static long sys6(long n, long a, long b, long c, long d, long e, long f)
{
    long ret;
    register long r10 __asm__("r10") = d;
    register long r8  __asm__("r8")  = e;
    register long r9  __asm__("r9")  = f;
    __asm__ volatile ("syscall"
                      : "=a"(ret)
                      : "a"(n), "D"(a), "S"(b), "d"(c), "r"(r10), "r"(r8), "r"(r9)
                      : "rcx", "r11", "memory");
    return ret;
}
#define sys1(n, a)                sys6(n, (long)(a), 0, 0, 0, 0, 0)
#define sys2(n, a, b)             sys6(n, (long)(a), (long)(b), 0, 0, 0, 0)
#define sys3(n, a, b, c)          sys6(n, (long)(a), (long)(b), (long)(c), 0, 0, 0)
#define sys6_(n, a, b, c, d, e, f) sys6(n, (long)(a), (long)(b), (long)(c), (long)(d), (long)(e), (long)(f))

void *memcpy(void *d, const void *s, size_t n)
{
    char *dd = d; const char *ss = s;
    while (n--) *dd++ = *ss++;
    return d;
}

void *memset(void *d, int c, size_t n)
{
    char *dd = d;
    while (n--) *dd++ = c;
    return d;
}

/* ----- tiny diagnostics on stderr --------------------------------------- */

static void putstr(const char *s)
{
    size_t n = 0;
    while (s[n]) n++;
    sys3(SYS_write, 2, s, n);
}

static void puthex(uint64_t v)
{
    char b[19];
    b[0] = '0'; b[1] = 'x';
    for (int i = 0; i < 16; i++)
        b[2 + i] = "0123456789abcdef"[(v >> (60 - 4 * i)) & 15];
    b[18] = 0;
    putstr(b);
}

static void __attribute__((noreturn)) die(const char *what, long err)
{
    putstr("rfork-stub: ");
    putstr(what);
    putstr(" failed, errno ");
    puthex(-err);
    putstr("\n");
    sys1(SYS_exit, 1);
    for (;;) ;
}

/* ----- I/O on the control socket ---------------------------------------- */

static void read_all(int fd, void *p, size_t n)
{
    char *c = p;
    while (n) {
        long r = sys3(SYS_read, fd, c, n);
        if (r == -EINTR) continue;
        if (r <= 0) die("read from rforkd", r ? r : -ECONNRESET);
        c += r; n -= r;
    }
}

/* send one status byte with a file descriptor attached (SCM_RIGHTS) */
static void send_fd(int sock, unsigned char status, int fd)
{
    struct iovec iov = { &status, 1 };
    char cbuf[CMSG_SPACE(sizeof(int))];
    struct msghdr mh;

    memset(&mh, 0, sizeof mh);
    memset(cbuf, 0, sizeof cbuf);
    mh.msg_iov = &iov;
    mh.msg_iovlen = 1;
    mh.msg_control = cbuf;
    mh.msg_controllen = sizeof cbuf;

    struct cmsghdr *cm = CMSG_FIRSTHDR(&mh);
    cm->cmsg_level = SOL_SOCKET;
    cm->cmsg_type  = SCM_RIGHTS;
    cm->cmsg_len   = CMSG_LEN(sizeof(int));
    memcpy(CMSG_DATA(cm), &fd, sizeof fd);

    long r = sys3(SYS_sendmsg, sock, &mh, 0);
    if (r < 0) die("sendmsg(uffd)", r);
}

/* ----- become the child ------------------------------------------------- */

static void __attribute__((noreturn)) jump_into_parent(const struct rfork_ctx *ctx)
{
    /* %0 is forced into rcx so that it is not one of the registers we load */
    __asm__ volatile (
        "cld\n\t"
        "movq  0(%0), %%rbx\n\t"
        "movq  8(%0), %%rbp\n\t"
        "movq 16(%0), %%r12\n\t"
        "movq 24(%0), %%r13\n\t"
        "movq 32(%0), %%r14\n\t"
        "movq 40(%0), %%r15\n\t"
        "movq 48(%0), %%rsp\n\t"
        "movl $1, %%eax\n\t"            /* rfork_save_context() returns 1: "you are the child" */
        "jmpq *56(%0)\n\t"
        : : "c"(ctx) : "memory");
    __builtin_unreachable();
}

void stub_main(void)
{
    static struct rfork_hdr    hdr;
    static struct rfork_region regs[RFORK_MAX_REGIONS];

    read_all(CTRL_FD, &hdr, sizeof hdr);
    if (hdr.magic != RFORK_MAGIC)
        die("bad magic", -EPROTO);
    if (hdr.nregions > RFORK_MAX_REGIONS)
        die("too many regions", -E2BIG);
    read_all(CTRL_FD, regs, hdr.nregions * sizeof regs[0]);

    /* 1. a userfaultfd for this address space */
    unsigned char status = RFORK_STUB_OK;
    long uffd = sys1(SYS_userfaultfd, O_CLOEXEC | O_NONBLOCK);
    if (uffd == -EPERM) {
        /* vm.unprivileged_userfaultfd = 0: only user-mode faults may be handled */
        uffd = sys1(SYS_userfaultfd, O_CLOEXEC | O_NONBLOCK | UFFD_USER_MODE_ONLY);
        status = RFORK_STUB_USER_MODE_ONLY;
    }
    if (uffd < 0) die("userfaultfd()", uffd);

    struct uffdio_api api;
    memset(&api, 0, sizeof api);
    api.api = UFFD_API;
    long r = sys3(SYS_ioctl, uffd, UFFDIO_API, &api);
    if (r < 0) die("UFFDIO_API", r);

    /* 2. rebuild the parent's address space, empty, every page "missing" */
    uint64_t me_lo = (uint64_t)__executable_start;
    uint64_t me_hi = ((uint64_t)_end + RFORK_PAGE - 1) & ~(uint64_t)(RFORK_PAGE - 1);

    for (uint64_t i = 0; i < hdr.nregions; i++) {
        uint64_t start = regs[i].start, end = regs[i].end;

        if (start < me_hi && end > me_lo) {
            putstr("rfork-stub: parent region ");
            puthex(start); putstr("-"); puthex(end);
            putstr(" overlaps the stub itself\n");
            sys1(SYS_exit, 1);
        }

        int prot = regs[i].prot & RFORK_PROT_READ ? (int)regs[i].prot : PROT_NONE;
        long p = sys6_(SYS_mmap, start, end - start, prot,
                       MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
        if (p != (long)start) {
            putstr("rfork-stub: region "); puthex(start); putstr("-"); puthex(end); putstr(": ");
            die("mmap(MAP_FIXED)", p);
        }

        if (!(regs[i].prot & RFORK_PROT_READ))
            continue;                   /* guard pages: nothing to fetch, ever */

        struct uffdio_register reg;
        memset(&reg, 0, sizeof reg);
        reg.range.start = start;
        reg.range.len   = end - start;
        reg.mode        = UFFDIO_REGISTER_MODE_MISSING;
        r = sys3(SYS_ioctl, uffd, UFFDIO_REGISTER, &reg);
        if (r < 0) die("UFFDIO_REGISTER", r);
    }

    /* 3. hand the userfaultfd to rforkd.  No need to wait for it: if we fault
          before rforkd has picked up the fd, the kernel just queues the fault. */
    send_fd(CTRL_FD, status, (int)uffd);
    sys1(SYS_close, uffd);

    /* 4. become the parent: TLS first (libc needs it), then registers + jump */
    r = sys2(SYS_arch_prctl, ARCH_SET_FS, hdr.fs_base);
    if (r < 0) die("arch_prctl(ARCH_SET_FS)", r);

    jump_into_parent(&hdr.ctx);
}
