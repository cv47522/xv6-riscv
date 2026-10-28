/*
 * forkfill.c: fork until the process table is full, then recover.
 *
 * Practice for MIT 6.1810 Lecture 3, "OS design":
 * https://pdos.csail.mit.edu/6.1810/2026/lec/l-os.txt (lectures/l-os.txt),
 * which asks of kfork(): "HW: allocproc; what happens if out of proc's?"
 * MIT published no example code for this lecture.
 *
 * ---------------------------------------------------------------------------
 *  PREREQUISITES
 * ---------------------------------------------------------------------------
 *
 *   Kind  Read first
 *   ----  ------------------------------------------------------------------
 *   code  user/ex8pipefork.c (EOF needs every write end closed)
 *   note  docs/11-os-design-in-xv6.md#fork-kfork-and-a-full-table
 *   code  kernel/proc.c allocproc(), kfork()
 *
 * ---------------------------------------------------------------------------
 *  RUNNING IT  (inside xv6, LAB=util, NPROC = 64)
 * ---------------------------------------------------------------------------
 *
 *    % make qemu
 *    $ forkfill
 *    forkfill: fork returned -1 after 61 children
 *    forkfill: reaped 61 children
 *    forkfill: fork works again
 *
 *  61 = NPROC (64, kernel/param.h) - init - sh - forkfill. Under LAB_FS,
 *  NPROC is 10 and the count drops to 7. A background job or a second
 *  shell lowers it further; the count is "free slots", not a constant.
 *
 * ---------------------------------------------------------------------------
 *  WHAT FILLS AND WHAT EMPTIES THE TABLE
 * ---------------------------------------------------------------------------
 *
 *    forkfill                     proc[] in kernel/proc.c
 *    --------                     -----------------------
 *    pipe(p)
 *    fork() x 61  ------------->  61 UNUSED slots -> RUNNABLE
 *      child: close(p[1]),        each child sleeps in piperead()
 *             read(p[0]) blocks
 *    fork() #62   ------------->  allocproc() finds no UNUSED slot,
 *                 <-------------  returns 0; kfork() returns -1
 *    close(p[1])  ------------->  last write end gone: every read()
 *                                 returns 0, every child exit(0)s
 *    wait() x 61  ------------->  freeproc() marks each slot UNUSED
 *    fork()       ------------->  succeeds again
 *
 *  Each child must close its inherited p[1] before blocking. A child that
 *  kept it would hold a write end open, so no read() could ever see EOF
 *  and the parent's wait() would hang -- the same protocol as ex8pipefork.
 *
 * ---------------------------------------------------------------------------
 *  RETURN VALUES OF fork()
 * ---------------------------------------------------------------------------
 *
 *   Return  Means                        What this program does
 *   ------  ---------------------------  ---------------------------------
 *    > 0    parent: the child's pid      count it and keep forking
 *      0    child                        block on the pipe, then exit(0)
 *     -1    no slot, or no memory        stop: the table is full
 *
 *  allocproc() also returns 0 when kalloc() cannot supply the trapframe
 *  page, and kfork() returns -1 when uvmcopy() runs out of pages. With
 *  this program's few pages per child, the slot limit is reached first.
 *  Either way the kernel fails the one call and keeps running: running
 *  out of proc structures is a user-visible error, not a panic.
 *
 * ---------------------------------------------------------------------------
 *  WHERE EACH PIECE LIVES IN THIS TREE
 * ---------------------------------------------------------------------------
 *
 *   Idea                             File
 *   -------------------------------  --------------------------------------
 *   NPROC                            kernel/param.h
 *   scan for an UNUSED slot          kernel/proc.c  allocproc()
 *   fork returns -1 when it fails    kernel/proc.c  kfork()
 *   slot freed when reaped           kernel/proc.c  kwait(), freeproc()
 *   EOF when last write end closes   kernel/pipe.c  piperead()
 *   lecture 3 companion              docs/11-os-design-in-xv6.md
 *   exercise details                 docs/07-exercises.md
 *
 * ---------------------------------------------------------------------------
 *  KEY POINTS
 * ---------------------------------------------------------------------------
 *
 *  * The process table is a fixed array; fork() fails when it is full.
 *  * A slot comes back only when the parent wait()s, not when a child exits.
 *  * Resource exhaustion by one user is an error code, not a kernel crash.
 */

#include "kernel/types.h"
#include "user/user.h"

int
main(void)
{
  int p[2];
  int pid, n, reaped;
  char c;

  if (pipe(p) < 0) {
    fprintf(2, "forkfill: pipe failed\n");
    exit(1);
  }

  for (n = 0;; n++) {
    pid = fork();
    if (pid < 0)
      break;
    if (pid == 0) {
      close(p[1]);
      read(p[0], &c, 1); // returns 0 once the parent closes p[1]
      exit(0);
    }
  }
  printf("forkfill: fork returned -1 after %d children\n", n);

  close(p[1]);
  for (reaped = 0; wait(0) >= 0; reaped++)
    ;
  close(p[0]);
  printf("forkfill: reaped %d children\n", reaped);

  pid = fork();
  if (pid < 0) {
    fprintf(2, "forkfill: fork still fails\n");
    exit(1);
  }
  if (pid == 0)
    exit(0);
  wait(0);
  printf("forkfill: fork works again\n");
  exit(0);
}
