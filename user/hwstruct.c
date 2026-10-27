/*
 * hwstruct.c: run Homework 2's struct-address expressions on 64-bit xv6.
 *
 * Based on the 6.1810 2026 homework "kernel C":
 * https://pdos.csail.mit.edu/6.1810/2026/homework/xv6-c.html, and the
 * "[demo hw.c]" step of lectures/l-c.txt. The handout assumes a 32-bit
 * machine; xv6 is RV64 (LP64), so pointers here are 8 bytes, not 4.
 * Worked 32-bit answer: docs/10-c-in-xv6.md#homework-2-struct-addresses.
 *
 * ---------------------------------------------------------------------------
 *  PREREQUISITES
 * ---------------------------------------------------------------------------
 *
 *   Kind  Read first
 *   ----  ------------------------------------------------------------------
 *   note  ../c-programming/C_Custom_Data_Types.md (padding and offsetof)
 *   note  docs/labs/03-util-memdump.md#structure-layout (LP64 struct ABI)
 *   code  user/ptr.c (where 0x1000 is in this address space)
 *
 * ---------------------------------------------------------------------------
 *  RUNNING IT  (inside xv6)
 * ---------------------------------------------------------------------------
 *
 *    % make qemu
 *    $ hwstruct
 *    sizeof: f=36 g=24 h=32
 *    &h->g.d[2]=0x0000000000001018       guaranteed: arithmetic only
 *    &h->g.c=0x0000000000001008          guaranteed: arithmetic only
 *    &h->f->a=0x0000000000000000         loaded h->f from 0x1000
 *    &h->g.c[10]=0x000000000000000A      loaded h->g.c from 0x1008
 *
 * ---------------------------------------------------------------------------
 *  LAYOUT OF struct h AT 0x1000  (RV64)
 * ---------------------------------------------------------------------------
 *
 *    0x1000  h->f          struct f *, 8 bytes      <- &h->f->a needs this
 *    0x1008  h->g.c        char *, 8 bytes          <- &h->g.c[10] needs this
 *    0x1010  h->g.d[0]     int
 *    0x1014  h->g.d[1]     int
 *    0x1018  h->g.d[2]     int                      <- &h->g.d[2]
 *    0x101C  h->g.d[3]     int
 *    0x1020  end           sizeof(struct h) == 32
 *
 * ---------------------------------------------------------------------------
 *  WHICH ANSWERS ARE GUARANTEED
 * ---------------------------------------------------------------------------
 *
 *   Expression    Computed as                     Touches memory?  Fixed?
 *   ------------  ------------------------------  ---------------  ------
 *   &h->g.d[2]    h + 8 + 8 + 2*4                 no               yes
 *   &h->g.c       h + 8                           no               yes
 *   &h->f->a      load(h + 0) + 0                 8-byte load      no
 *   &h->g.c[10]   load(h + 8) + 10                8-byte load      no
 *
 *  `&` stops the last dereference but not the earlier ones: `h->f->a`
 *  must first read the pointer stored in h->f. Those two answers depend on
 *  whatever this binary happens to have at 0x1000.
 *
 *  Why every print succeeded: 0x1000 is this program's own .data/.bss
 *  page, mapped with PTE_U by kexec(). Here it holds `freep` from
 *  user/umalloc.c (`nm -n user/_hwstruct`: `0000000000001000 b freep`), a
 *  zeroed .bss pointer that malloc() has never set, so both loads read 0.
 *  A program whose page at 0x1000 were unmapped would instead take a page
 *  fault and usertrap() in kernel/trap.c would kill it.
 *
 *  Q: Why doesn't `+ 1` on an address add one bit instead of one byte?
 *  A: Because an address numbers bytes, not bits. RISC-V memory is
 *  byte-addressable: 0x1008 and 0x1009 are neighbouring bytes, and no
 *  address names a single bit. So the units are fixed:
 *
 *    Quantity            Unit    Example in this file
 *    ------------------  ------  ---------------------------------------
 *    an address          byte    0x1008 is byte 4104
 *    sizeof              byte    sizeof(char *) == 8
 *    p + n               n * sizeof(*p) bytes
 *                                h->g.c + 10 moves 10 (char is 1 byte)
 *                                h->g.d + 2 moves 8 (int is 4 bytes)
 *
 *  A single bit is reached by loading its byte (or word) and masking it
 *  with &, |, ^ -- see the fcntl.h flags in docs/10-c-in-xv6.md.
 *
 *  Q: Why does h->g.d[0] start at 0x1010 and not 0x1009?
 *  A: Because h->g.c occupies 8 bytes, not 1. Each address in the layout
 *  above is where a member STARTS; the member then fills every byte up to
 *  the next one:
 *
 *    Member     Starts  Size  Occupies         Next free byte
 *    ---------  ------  ----  ---------------  --------------
 *    h->f       0x1000  8     0x1000..0x1007   0x1008
 *    h->g.c     0x1008  8     0x1008..0x100F   0x1010
 *    h->g.d[0]  0x1010  4     0x1010..0x1013   0x1014
 *
 *  0x1009 is the second byte of the pointer h->g.c. Even if c were a
 *  1-byte char, d[0] would not start at 0x1009: an int needs a 4-byte
 *  aligned address, so the compiler would insert 3 bytes of padding and
 *  start it at 0x100C.
 *
 * ---------------------------------------------------------------------------
 *  WHERE EACH PIECE LIVES IN THIS TREE
 * ---------------------------------------------------------------------------
 *
 *   Idea                           File
 *   -----------------------------  ----------------------------------------
 *   .data/.bss start at 0x1000     user/user.ld
 *   the zeroed word at 0x1000      user/umalloc.c  (static Header *freep)
 *   faulting user loads            kernel/trap.c  usertrap()
 *   32-bit worked answer           docs/10-c-in-xv6.md
 *   exercise details               docs/07-exercises.md
 *
 * ---------------------------------------------------------------------------
 *  KEY POINTS
 * ---------------------------------------------------------------------------
 *
 *  * Member offsets follow from sizes and alignment, and differ by ABI.
 *  * `&a->b->c` still loads a->b; only the final access is skipped.
 *  * "It printed" means the page was mapped, not that the value is meaningful.
 */

#include "kernel/types.h"
#include "user/user.h"

struct f {
  int a;
  char b[32];
};

struct g {
  char *c;
  int d[4];
};

struct h {
  struct f *f;
  struct g g;
};

int
main(void)
{
  struct h *h = (struct h *)0x1000; // an address, not an object we own

  printf("sizeof: f=%d g=%d h=%d\n", (int)sizeof(struct f),
         (int)sizeof(struct g), (int)sizeof(struct h));
  printf("&h->g.d[2]=%p\n", (void *)&h->g.d[2]);
  printf("&h->g.c=%p\n", (void *)&h->g.c);
  printf("&h->f->a=%p\n", (void *)&h->f->a);       // reads h->f
  printf("&h->g.c[10]=%p\n", (void *)&h->g.c[10]); // reads h->g.c

  exit(0);
}
