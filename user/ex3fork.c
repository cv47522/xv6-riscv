// ex3fork.c: create a child process and observe both fork() returns.
//
// Based on MIT 6.1810 Lecture 1's ex3.c:
// https://pdos.csail.mit.edu/6.1810/2026/lec/l-overview/ex3.c.
// Host references: man 2 fork; man 2 getpid.
//
// ---------------------------------------------------------------------------
//  PREREQUISITES
// ---------------------------------------------------------------------------
//
//  <OS> means the sibling repository at ../operating-system.
//
//   Kind  Read first
//   ----  ------------------------------------------------------------------
//   note  <OS>/The_Process_Abstraction.md  (section 7.1)
//   code  <OS>/codes/src/main/virtualization/cpu-process-api/fork_basic.c
//
// ---------------------------------------------------------------------------
//  RUNNING IT  (inside xv6, not on the host)
// ---------------------------------------------------------------------------
//
//    % make qemu
//    $ ex3fork
//    $ ex3fork             run again: output order may change
//    % make qemu CPUS=1    reduces, but does not define, scheduling order
//
// ---------------------------------------------------------------------------
//  ONE CALL, TWO RETURNS
// ---------------------------------------------------------------------------
//
//   Return  Process   Meaning
//   ------  --------  ------------------------------------------------------
//     > 0   parent    the child's pid
//       0   child     this process is the newly created child
//      -1   parent    no child was created
//
//  Both processes resume at the statement after fork(); neither restarts at
//  main(). The scheduler decides which process runs first.
//
//   Copied for the child        Relationship after fork()
//   -------------------------   --------------------------------------------
//   instructions and registers  each process advances independently
//   data, stack, and heap       separate memory; x proves this below
//   descriptor table            separate entries, shared open-file objects
//   current directory           both initially name the same directory
//
// ---------------------------------------------------------------------------
//  WHY THE OUTPUT ORDER IS NOT FIXED
// ---------------------------------------------------------------------------
//
//  Nothing in this program orders the two processes. There is no wait(), so
//  both are runnable the moment fork() returns, and three effects stack up:
//
//   Cause                       Effect on what reaches the console
//   --------------------------  ----------------------------------------
//   the scheduler chooses       either process may print first, and the
//   freely                      choice can differ from run to run
//   CPUS=3 by default           the two really do run at the same instant
//                               on different harts, so it is not merely a
//                               question of who was picked first
//   printf() is not atomic      user/printf.c formats into a small buffer
//                               and calls write() several times per line,
//                               and uartputc() takes its lock per
//                               CHARACTER -- so the streams mix mid-word
//
//  The output is therefore not just reordered; a real run looks like this:
//
//    (ex3fork) forke(x3for)k returned : fork() retu6r n e(dI  0  (I ama mp
//
//  A third process joins in. sh forked this program and is sitting in
//  wait(), which returns as soon as ITS child -- the parent here -- exits.
//  The child may still be running then, so the $ prompt can land mid-line.
//  A prompt is not a promise that everything finished.
//
//  `make qemu CPUS=1` usually yields whole lines. That is not a fix; it only
//  makes the race harder to hit. Ordering needs wait(), which is ex5forkexec.
//
// ---------------------------------------------------------------------------
//  WHERE EACH PIECE LIVES IN THIS TREE
// ---------------------------------------------------------------------------
//
//   Idea                        File
//   --------------------------  -------------------------------------------
//   fork/getpid/exit stubs      user/usys.S
//   process creation            kernel/proc.c  (kfork())
//   process state               kernel/proc.h  (struct proc)
//   scheduling                  kernel/proc.c  (scheduler())
//   init reaping orphans        user/init.c
//   exercise details            docs/07-exercises.md
//
// ---------------------------------------------------------------------------
//  THREE THINGS WORTH KEEPING
// ---------------------------------------------------------------------------
//
//  * fork() returns in two processes, with its return value naming the role.
//  * The child receives private memory but inherited descriptor references.
//  * Without wait(), output order and prompt placement are not guaranteed.

#include "kernel/types.h"
#include "user/user.h"

int
main(void)
{
  int x = 1;
  int pid = fork(); /* fork() copies the same fd table from parent process to child process */

  if (pid < 0) {
    fprintf(2, "(ex3fork) fork failed\n");
    return 1;
  }

  // This statement runs once in each process after fork().
  printf("(ex3fork) fork() returned %d  (I am pid %d)\n", pid, getpid());

  if (pid == 0) {
    // The child's write changes only its private copy of x.
    x = 22;
    printf("(ex3fork) child: my own pid is %d\n", getpid());
    printf("(ex3fork) child: x is %d\n", x);
    exit(0);
  }

  x = 11;
  printf("(ex3fork) parent: my child is pid %d\n", pid);
  printf("(ex3fork) parent: x is %d\n", x);

  // Deliberately no wait(): init reaps an orphan if this parent exits first.
  return 0;
}
