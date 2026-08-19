// ex3fork.c: create a new process with fork().
//
// Based on MIT 6.1810 Lecture 1's ex3.c, linked from the schedule on the
// course web site:
// https://pdos.csail.mit.edu/6.1810/2026/lec/l-overview/ex3.c.
// The host manual page for the same call is `man 2 fork`.
//
// ---------------------------------------------------------------------------
//  RUNNING IT  (inside xv6, not on the host)
// ---------------------------------------------------------------------------
//
//    % make qemu
//    $ ex3fork              run it several times and watch the line order
//
//  A typical run:
//
//    $ ex3fork
//    ex3fork: fork() returned 4  (I am pid 3)
//    ex3fork: parent, my child is pid 4
//    ex3fork: fork() returned 0  (I am pid 4)
//    ex3fork: child, my own pid is 4
//    $
//
//  The first line appears TWICE across the two processes even though the
//  source contains one printf().  That is the whole exercise: after fork()
//  there are two processes executing the same instructions, and the only
//  thing telling them apart is the value fork() handed back.
//
//  The four lines can interleave in any order, and the `$` prompt can land
//  in the middle of them.  Nothing here is broken -- see the section on
//  ordering below.
//
// ---------------------------------------------------------------------------
//  WHAT fork() COPIES
// ---------------------------------------------------------------------------
//
//                    parent (pid 3)                child (pid 4)
//                    +--------------------+        +--------------------+
//    instructions    | the ex3fork code   |  copy  | the ex3fork code   |
//    data + stack    | pid, x, buf ...    | =====> | pid, x, buf ...    |
//    FD table        | 0,1,2 -> console   |        | 0,1,2 -> console   |
//    current dir     | /                  |        | /                  |
//                    +--------------------+        +--------------------+
//                    fork() returns 4              fork() returns 0
//
//  The copy is complete and immediate: the child starts life at the same
//  instruction, with the same values in the same variables.  From that
//  point the two memories are SEPARATE -- assigning to a variable in one
//  process is invisible to the other.  The `x` experiment below shows it.
//
//  One thing is copied but not duplicated: the file OFFSET behind each
//  descriptor is shared, because fork() copies the descriptor table (level
//  one) while both tables point at the same struct file (level two).  So
//  parent and child writing fd 1 append to it in turn rather than
//  overwriting each other.  ex5forkexec.c relies on that.
//
//  kfork() in kernel/proc.c does the work: allocproc() for a new struct
//  proc, uvmcopy() for the memory, a loop over p->ofile[] calling
//  filedup(), and idup() for p->cwd.
//
// ---------------------------------------------------------------------------
//  fork()
// ---------------------------------------------------------------------------
//
//   Return   In which process   Means
//   -------  -----------------  --------------------------------------------
//    > 0     the parent         the child's pid; the parent is told who its
//                               child is, because it may want to wait() for
//                               it later
//      0     the child          "you are the new one".  The child is NOT
//                               told its parent's pid here; getpid() gives
//                               its own, and 0 is a pid no process can have
//     -1     the parent         no child was created: the process table is
//                               full (NPROC in kernel/param.h) or memory
//                               ran out.  There is no child to look for
//
//  The asymmetry is deliberate.  A single call site returns twice, in two
//  address spaces, and the return value is the only difference between
//  them -- so `if (pid == 0)` is not a test of a variable so much as a test
//  of WHICH PROCESS is asking.
//
// ---------------------------------------------------------------------------
//  WHY THE OUTPUT ORDER IS NOT FIXED
// ---------------------------------------------------------------------------
//
//  After fork() both processes are runnable, and the scheduler in
//  kernel/proc.c may run either one first, on either hart.  Neither waits
//  for the other, so the four lines can appear in several orders.
//
//  In practice it is worse than reordering, and a real run looks like this:
//
//    ex3fork: forke(x3for)k returned : fork() retu6r n e(dI  0  (I ama mp
//
//  The two processes are interleaved MID-WORD.  printf() in user/printf.c
//  is not atomic: it formats into a small buffer and calls write() as it
//  goes, and the scheduler can switch processes between those writes.  With
//  CPUS=3 they are genuinely running at the same instant on different harts.
//  Nothing serializes console output -- uartputc() takes a lock per
//  character, not per line, so the granularity of the mixing is a byte.
//
//  This is a first look at the problem the locking and scheduling chapters
//  exist to solve.  Run it under `make qemu CPUS=1` and the output usually
//  comes out in whole lines, which is not a fix -- it only makes the race
//  harder to hit.
//
//  A third process joins in: the shell.  It forked us and is sitting in
//  wait(), and wait() returns as soon as ITS child -- the parent process
//  here -- exits.  Our child may still be running at that moment, so the
//  `$` prompt can print between our lines.  The prompt is not a promise
//  that everything finished.
//
//  This program deliberately does NOT call wait(), so that the raw
//  behaviour is visible.  ex5forkexec.c adds it, and the interleaving
//  stops.  A parent that exits before its child leaves an ORPHAN: kexit()
//  in kernel/proc.c reparents it to init (pid 1), which wait()s in a loop
//  forever so that no exited process stays a zombie.
//
// ---------------------------------------------------------------------------
//  WHERE EACH PIECE LIVES IN THIS TREE
// ---------------------------------------------------------------------------
//
//   Idea                        File
//   --------------------------  -------------------------------------------
//   the fork/getpid stubs       user/usys.S  (load a7, then `ecall`)
//   sys_fork, sys_getpid        kernel/sysfile.c, kernel/sysproc.c
//   kfork() itself              kernel/proc.c
//   copying the address space   kernel/vm.c  (uvmcopy())
//   the process table, NPROC    kernel/proc.c, kernel/param.h
//   reparenting on exit         kernel/proc.c  (kexit(), reparent())
//   the shell's fork/wait loop  user/sh.c  (main(), runcmd())
//   this exercise, written up   docs/07-exercises.md
//   the long-form notes         docs/book/ch01-operating-system-interfaces.md
//
// ---------------------------------------------------------------------------
//  THREE THINGS WORTH KEEPING
// ---------------------------------------------------------------------------
//
//  * fork() returns twice, once per process.  Every later idea in this
//    lecture -- exec, redirection, pipelines -- is built on that one fact.
//  * The copy is a snapshot, not a link.  Shared descriptors and offsets
//    survive; shared variables do not.
//  * Without wait(), the parent and child are simply two independent
//    processes, and nothing orders their output.  Interleaved lines are
//    the correct result, not a bug to fix.
//
//  Still open: fork() gives a new process running the SAME program.  How do
//  you make it run a DIFFERENT one?  exec() -- see ex4exec.c.

#include "kernel/types.h"
#include "user/user.h"

int
main(void)
{
  // Set before the fork, so both processes start with x == 1.  Each then
  // assigns its own value into its own copy.
  int x = 1;

  int pid = fork();

  // Checked first, because on failure there is no second process and the
  // pid == 0 branch below would be wrong to take.
  if (pid < 0) {
    fprintf(2, "ex3fork: fork failed\n");
    return 1;
  }

  // Reached in BOTH processes: one printf() in the source, two lines on the
  // console.  getpid() distinguishes the writers.
  printf("ex3fork: fork() returned %d  (I am pid %d)\n", pid, getpid());

  if (pid == 0) {
    // The child.  Assigning x here cannot be seen by the parent: uvmcopy()
    // gave this process its own physical pages.
    x = 22;
    printf("ex3fork: child, my own pid is %d\n", getpid());
    printf("ex3fork: child, x is %d\n", x);

    // exit() rather than falling through to the shared `return 0` below,
    // so the two paths stay visibly separate.  Both reach the same system
    // call in the end -- start() in user/ulib.c does exit(main(...)).
    exit(0);
  }

  // The parent, where `pid` is the child's pid rather than 0.
  x = 11;
  printf("ex3fork: parent, my child is pid %d\n", pid);
  printf("ex3fork: parent, x is %d\n", x);

  // No wait() on purpose -- see the ordering section above.  The parent may
  // well exit before the child prints, handing the child to init.
  return 0;
}
