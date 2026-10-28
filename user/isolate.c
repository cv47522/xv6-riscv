/*
 * isolate.c: try four ways past the user/kernel boundary; each is refused.
 *
 * Practice for MIT 6.1810 Lecture 3, "OS design":
 * https://pdos.csail.mit.edu/6.1810/2026/lec/l-os.txt (lectures/l-os.txt),
 * the "hardware-level isolation" and "How do system calls work?" parts.
 * MIT published no example code for this lecture.
 *
 * ---------------------------------------------------------------------------
 *  PREREQUISITES
 * ---------------------------------------------------------------------------
 *
 *   Kind  Read first
 *   ----  ------------------------------------------------------------------
 *   note  ../operating-system/The_Process_Abstraction.md (user/kernel mode)
 *   note  docs/11-os-design-in-xv6.md (the boundary in this tree)
 *   code  kernel/trap.c usertrap(), kernel/syscall.c syscall()
 *
 * ---------------------------------------------------------------------------
 *  RUNNING IT  (inside xv6; one attempt per run)
 * ---------------------------------------------------------------------------
 *
 *    % make qemu
 *    $ isolate csr
 *    isolate: csrr sstatus from user mode
 *    usertrap(): unexpected scause 0x2 pid=3
 *                sepc=0x44 stval=0x100027f3
 *    $ isolate kload
 *    isolate: load from TRAMPOLINE 0x0000003FFFFFF000
 *    usertrap(): unexpected scause 0xd pid=4
 *                sepc=0x86 stval=0x3ffffff000
 *    $ isolate badsys
 *    isolate: ecall with a7=99
 *    5 isolate: unknown sys call 99
 *    isolate: ecall returned -1
 *    $ isolate kcopy
 *    isolate: read() into TRAPFRAME 0x0000003FFFFFE000 returned -1
 *    isolate: read() into a user buffer returned 2: "hi"
 *
 *  The pids differ per run; the usertrap() and "unknown sys call" lines
 *  come from the kernel's printk(), not from this program.
 *
 *   Register  csr                        kload
 *   --------  -------------------------  ---------------------------------
 *   scause    2, illegal instruction     0xd, load page fault
 *   sepc      0x44, `csrr a5,sstatus`    0x86, `lbu a5,0(a5)`
 *   stval     the instruction's bits     the address that was refused
 *
 *  Check sepc against user/isolate.asm: 0x100027f3 is printed beside 0x44
 *  there as the encoding of the very csrr that trapped.
 *
 * ---------------------------------------------------------------------------
 *  WHAT EACH ATTEMPT HITS
 * ---------------------------------------------------------------------------
 *
 *   Mode    Attempt                     Stopped by     Outcome
 *   ------  --------------------------  -------------  --------------------
 *   csr     read a supervisor CSR       the CPU        scause 2, killed
 *   kload   load a mapped kernel page   the MMU        scause 0xd, killed
 *   badsys  ecall a number with no      syscall()      -1, process lives
 *           kernel function
 *   kcopy   make the kernel write into  copyout() ->   -1, pipe bytes kept
 *           a kernel page               walkaddr()
 *
 *  The first two are hardware isolation: user mode cannot execute a
 *  privileged instruction, and the MMU refuses a page without PTE_U. The
 *  last two are the "careful system call interface": the kernel treats a7
 *  and every user pointer as hostile input.
 *
 *  NOTE: TRAMPOLINE and TRAPFRAME (kernel/memlayout.h) ARE mapped in this
 *  process's own page table -- proc_pagetable() in kernel/proc.c maps them
 *  without PTE_U. So "kload" faults on permission, not on a missing
 *  mapping, and "kcopy" names the one kernel page whose contents this
 *  very process depends on: its saved registers.
 *
 * ---------------------------------------------------------------------------
 *  THE PATH A REFUSED KCOPY TAKES
 * ---------------------------------------------------------------------------
 *
 *    read(p[0], TRAPFRAME, 2)               user mode
 *      |  ecall, a7 = SYS_read (5)
 *      v
 *    usertrap() -> syscall() -> sys_read()  kernel/trap.c, syscall.c
 *      v
 *    piperead() -> copyout(dst=TRAPFRAME)   kernel/pipe.c, kernel/vm.c
 *      v
 *    walkaddr(): PTE_U clear -> 0           kernel/vm.c
 *    vmfault(): va >= p->sz -> 0            no lazy page to make
 *      v
 *    copyout() returns -1; piperead() returns -1 before nread++,
 *    so the two bytes stay in the pipe for the next read()
 *
 *  Without the PTE_U test in walkaddr(), the kernel would run memmove()
 *  onto its own trapframe with bytes the user chose -- including the
 *  saved epc it later returns to.
 *
 * ---------------------------------------------------------------------------
 *  WHERE EACH PIECE LIVES IN THIS TREE
 * ---------------------------------------------------------------------------
 *
 *   Idea                             File
 *   -------------------------------  --------------------------------------
 *   r_sstatus(), the csrr wrapper    kernel/riscv.h
 *   TRAMPOLINE, TRAPFRAME            kernel/memlayout.h
 *   both mapped without PTE_U        kernel/proc.c  proc_pagetable()
 *   unexpected scause -> killed      kernel/trap.c  usertrap()
 *   a7 bounds check                  kernel/syscall.c  syscall()
 *   user-pointer checks              kernel/vm.c  walkaddr(), copyout()
 *   lecture 3 companion              docs/11-os-design-in-xv6.md
 *   exercise details                 docs/07-exercises.md
 *
 * ---------------------------------------------------------------------------
 *  KEY POINTS
 * ---------------------------------------------------------------------------
 *
 *  * The same r_sstatus() the kernel calls is fatal from user mode.
 *  * Mapped is not accessible: PTE_U decides, in hardware and in copyout().
 *  * A bad request from user code costs the caller, never the kernel.
 */

#include "kernel/types.h"
#include "kernel/riscv.h"
#include "kernel/memlayout.h"
#include "user/user.h"

#define BADSYS 99 // above SYS_sync (22), the last entry in kernel/syscall.h

/*
 * usys.S fixes a7 per stub, so reaching an unused number needs a
 * hand-written ecall; a0 carries the result back exactly as for a stub.
 */
static long
rawsyscall(long num)
{
  register long a7 __asm__("a7") = num;
  register long a0 __asm__("a0");

  __asm__ volatile("ecall" : "=r"(a0) : "r"(a7) : "memory");
  return a0;
}

static void
kcopy(void)
{
  int p[2];
  char buf[3];
  int n;

  if (pipe(p) < 0) {
    fprintf(2, "isolate: pipe failed\n");
    exit(1);
  }
  if (write(p[1], "hi", 2) != 2) {
    fprintf(2, "isolate: write failed\n");
    exit(1);
  }

  n = read(p[0], (char *)TRAPFRAME, 2);
  printf("isolate: read() into TRAPFRAME %p returned %d\n",
         (void *)TRAPFRAME, n);

  // The refused read consumed nothing, so the same bytes are still there.
  n = read(p[0], buf, 2);
  if (n < 0) {
    fprintf(2, "isolate: read failed\n");
    exit(1);
  }
  buf[n] = 0;
  printf("isolate: read() into a user buffer returned %d: \"%s\"\n", n, buf);

  close(p[0]);
  close(p[1]);
}

int
main(int argc, char *argv[])
{
  if (argc != 2) {
    fprintf(2, "usage: isolate csr|kload|badsys|kcopy\n");
    exit(1);
  }

  if (strcmp(argv[1], "csr") == 0) {
    printf("isolate: csrr sstatus from user mode\n");
    r_sstatus();
    printf("isolate: not reached\n");
  } else if (strcmp(argv[1], "kload") == 0) {
    printf("isolate: load from TRAMPOLINE %p\n", (void *)TRAMPOLINE);
    (void)*(volatile char *)TRAMPOLINE;
    printf("isolate: not reached\n");
  } else if (strcmp(argv[1], "badsys") == 0) {
    printf("isolate: ecall with a7=%d\n", BADSYS);
    printf("isolate: ecall returned %d\n", (int)rawsyscall(BADSYS));
  } else if (strcmp(argv[1], "kcopy") == 0) {
    kcopy();
  } else {
    fprintf(2, "isolate: unknown mode %s\n", argv[1]);
    exit(1);
  }
  exit(0);
}
