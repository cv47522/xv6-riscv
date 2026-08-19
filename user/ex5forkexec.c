// ex5forkexec.c: fork() a child, exec() a program in it, wait() for it.
//
// Based on MIT 6.1810 Lecture 1's ex5.c, linked from the schedule on the
// course web site:
// https://pdos.csail.mit.edu/6.1810/2026/lec/l-overview/ex5.c.
// The host manual page for the third call is `man 2 wait`.
//
// ---------------------------------------------------------------------------
//  RUNNING IT  (inside xv6, not on the host)
// ---------------------------------------------------------------------------
//
//    % make qemu
//    $ ex5forkexec
//    ex5forkexec: parent waiting for child 4
//    THIS IS ECHO
//    ex5forkexec: child 4 exited with status 0
//    $
//
//  Unlike ex3fork.c, the order here is fixed after the first line: wait()
//  does not return until the child is gone, so the exit message can never
//  print before the child's output.  The first two lines can still swap,
//  because the child may reach echo before the parent reaches its printf.
//
//  This is the complete shape of what user/sh.c does for EVERY command you
//  type.  Everything after this exercise is a variation on it.
//
// ---------------------------------------------------------------------------
//  THE SHAPE
// ---------------------------------------------------------------------------
//
//    parent                          child
//    ------                          -----
//    pid = fork()  ---------------->  pid == 0
//    pid > 0                          |
//    |                                exec("echo", argv)
//    wait(&status)   (asleep)         |  becomes a different program
//    |                                echo prints, then exit(0)
//    |     <-------------------------- exit status travels back
//    wait() returns the child's pid
//    status now holds 0
//
//  The gap between fork() and exec() is the whole reason Unix keeps them
//  separate.  Code that runs in the child, after the fork but before the
//  exec, can change the child's world -- and ONLY the child's -- without
//  the parent or the exec'd program knowing.  ex6redirect.c puts a
//  close()/open() pair in exactly that gap.  As the xv6 book puts it, a
//  hypothetical combined forkexec() would have to take redirection
//  instructions as arguments, or make every program implement its own.
//
// ---------------------------------------------------------------------------
//  wait(status)
// ---------------------------------------------------------------------------
//
//   Argument  Is                          Rule
//   --------  --------------------------  ----------------------------------
//   status    where to store the child's  an OUT parameter: the kernel
//             exit status                 writes through the pointer.  Pass
//                                         0 to discard it
//
//   Return    Means
//   -------   ---------------------------------------------------------------
//    > 0      the pid of a child that exited.  With several children you
//             learn WHICH one this way -- wait() reaps any of them, not a
//             chosen one, and xv6 has no waitpid()
//      -1     the caller has no children at all; nothing to wait for
//
//  wait() blocks only if no child has exited YET.  A child that finished
//  earlier is already sitting in the process table as a ZOMBIE -- exited,
//  but retained so its status can still be collected -- and wait() returns
//  from it immediately.  kexit() in kernel/proc.c sets p->state = ZOMBIE
//  and wakes the parent; kwait() finds it, copies p->xstate out, and calls
//  freeproc() to release the slot.  A parent that never wait()s is what
//  user/zombie.c demonstrates.
//
// ---------------------------------------------------------------------------
//  WHY `status` HAS TO BE A SEPARATE VARIABLE
// ---------------------------------------------------------------------------
//
//  Because wait()'s return value is already spoken for: it carries the pid.
//  One integer cannot carry both the identity of the child and the value it
//  exited with, so the status leaves through a pointer instead.
//
//    int status;               declared, deliberately uninitialised --
//                              wait() writes it before we read it
//    int pid = wait(&status);  pid comes back, status is filled in
//
//  There are two failure modes worth naming.  Passing an uninitialised
//  `status` BY VALUE would be meaningless; and reading `status` after
//  wait() returned -1 reads a variable the kernel never wrote, which is why
//  the check below happens first.  Initialising it to 0 anyway is cheap
//  insurance and is what this file does.
//
//  The exit status is a convention, not a rule the kernel enforces:
//
//    0        success
//    non-0    failure; 1 is the usual "something went wrong"
//
//  It is how a child reports an outcome to a parent that cannot see its
//  memory.  32 bits is the entire channel.  user/sh.c ignores the value,
//  but the grading scripts and any script-like caller do not.
//
// ---------------------------------------------------------------------------
//  A NOTE ON WASTE, AND THE COPY-ON-WRITE LAB
// ---------------------------------------------------------------------------
//
//  fork() copies the parent's whole address space, and exec() then throws
//  that copy away a few instructions later.  Every command the shell runs
//  pays for it.  Real kernels avoid the copy with copy-on-write: fork()
//  maps the parent's pages into the child read-only and shares them, and
//  only a page that is actually written gets duplicated -- so pages that
//  exec() is about to discard are never copied at all.  That is the cow
//  lab; see docs/03-lab-workflow.md.
//
// ---------------------------------------------------------------------------
//  WHERE EACH PIECE LIVES IN THIS TREE
// ---------------------------------------------------------------------------
//
//   Idea                        File
//   --------------------------  -------------------------------------------
//   the fork/exec/wait stubs    user/usys.S  (load a7, then `ecall`)
//   sys_fork, sys_wait, sys_exit  kernel/sysproc.c
//   kwait(), kexit(), ZOMBIE    kernel/proc.c
//   kexec(), ELF loading        kernel/exec.c
//   a parent that never waits   user/zombie.c
//   the shell doing all three   user/sh.c  (main(), runcmd())
//   this exercise, written up   docs/07-exercises.md
//   the long-form notes         docs/book/ch01-operating-system-interfaces.md
//
// ---------------------------------------------------------------------------
//  THREE THINGS WORTH KEEPING
// ---------------------------------------------------------------------------
//
//  * fork + exec + wait is the shell's entire execution model.  Read
//    user/sh.c after this file and it will look familiar.
//  * The window between fork() and exec() is where redirection and
//    pipelines are built.  Combining the two calls would close it.
//  * wait() returns a pid and delivers the status through a pointer,
//    because one return value cannot carry two answers.

#include "kernel/types.h"
#include "user/user.h"

int
main(void)
{
  // Initialised even though wait() will overwrite it: if wait() fails there
  // is nothing to overwrite it with, and a defined 0 beats whatever was on
  // the stack.
  int status = 0;

  int pid = fork();
  if (pid < 0) {
    fprintf(2, "ex5forkexec: fork failed\n");
    return 1;
  }

  if (pid == 0) {
    // The child.  Everything from here is disposable -- this process exists
    // only to become echo.
    char *argv[] = { "echo", "THIS", "IS", "ECHO", 0 };
    exec("echo", argv);

    // Only on failure; a successful exec() never comes back here.
    fprintf(2, "ex5forkexec: exec echo failed\n");

    // exit(), not return: this is the child, and returning would fall into
    // the parent's code below and run a second wait() from the wrong
    // process.  Non-zero so the parent's status report shows the failure.
    exit(1);
  }

  // The parent.
  printf("ex5forkexec: parent waiting for child %d\n", pid);

  // Sleeps in kwait() until the child exits, then fills in `status` and
  // returns the pid.  With one child that pid necessarily equals `pid`
  // above; with several it is how you tell which one finished.
  int reaped = wait(&status);
  if (reaped < 0) {
    // Only when the caller has no children -- impossible here, since fork()
    // just succeeded, but the check documents what -1 would mean.
    fprintf(2, "ex5forkexec: wait failed\n");
    return 1;
  }

  printf("ex5forkexec: child %d exited with status %d\n", reaped, status);
  return 0;
}
