/*
 * rfork_proto.h - wire format shared by the parent-side library (rfork.c),
 * the pager daemon on the "other machine" (rforkd.c) and the child stub
 * (stub.c).  The stub is freestanding, so this header may only use <stdint.h>.
 *
 * Message flow:
 *
 *   parent (page server) --TCP--> rforkd:  rfork_hdr, then nregions * rfork_region
 *   rforkd --unix socketpair--> stub:      the same header + regions
 *   stub   --unix socketpair--> rforkd:    1 status byte + the userfaultfd (SCM_RIGHTS)
 *   rforkd --TCP--> page server:           uint64_t address of the page it wants
 *   page server --TCP--> rforkd:           RFORK_PAGE bytes
 *   rforkd closes the TCP connection when the child has exited.
 */
#ifndef RFORK_PROTO_H
#define RFORK_PROTO_H

#include <stdint.h>

#define RFORK_PORT          7777
#define RFORK_MAGIC         0x30316b726f6672ULL      /* "rfork10" */
#define RFORK_PAGE          4096
#define RFORK_MAX_REGIONS   64

/* region protection bits (same values as PROT_READ/WRITE/EXEC on Linux) */
#define RFORK_PROT_READ     1
#define RFORK_PROT_WRITE    2
#define RFORK_PROT_EXEC     4

/* callee-saved register state at the point where rfork() "returns" in the child */
struct rfork_ctx {
    uint64_t rbx, rbp, r12, r13, r14, r15, rsp, rip;
};

struct rfork_region {
    uint64_t start, end;                 /* page aligned, [start, end) */
    uint64_t prot;                       /* RFORK_PROT_* */
    char     name[32];                   /* for logging only, e.g. "demo", "[heap]" */
};

struct rfork_hdr {
    uint64_t         magic;
    struct rfork_ctx ctx;
    uint64_t         fs_base;            /* TLS pointer of the parent's main thread */
    uint64_t         nregions;
};

/* status byte the stub sends back together with the userfaultfd */
#define RFORK_STUB_OK             0
#define RFORK_STUB_USER_MODE_ONLY 1      /* had to fall back to UFFD_USER_MODE_ONLY */

#endif
