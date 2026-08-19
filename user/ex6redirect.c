// ex6redirect.c: redirect a command's standard output to a file.
//
// Based on MIT 6.1810 Lecture 1's ex6.c, linked from the schedule on the
// course web site:
// https://pdos.csail.mit.edu/6.1810/2026/lec/l-overview/ex6.c.
// This is what the shell does for `echo hello > out`.
//
// ---------------------------------------------------------------------------
//  RUNNING IT  (inside xv6, not on the host)
// ---------------------------------------------------------------------------
//
//    % make qemu
//    $ ex6redirect
//    ex6redirect: parent waiting for child 4
//    ex6redirect: child 4 exited with status 0
//    $ cat ex6.out
//    ex6redirect's redirected echo
//
//  echo's output never reaches the console.  Nothing about echo changed --
//  user/echo.c still writes to descriptor 1 and has no idea where it goes.
//  The child rewired descriptor 1 before exec'ing it.
//
//  Compare with the shell doing the same thing itself:
//
//    $ echo hello > ex6.out
//
//  Same mechanism, same three system calls, in user/sh.c instead of here.
//
// ---------------------------------------------------------------------------
//  THE TRICK, IN TWO CALLS
// ---------------------------------------------------------------------------
//
//    child's FD table       close(1)              open("ex6.out", ...)
//    +--------------+       +--------------+      +--------------------+
//    | 0 -> console |       | 0 -> console |      | 0 -> console       |
//    | 1 -> console |  ==>  | 1 <empty>    |  ==> | 1 -> ex6.out       |
//    | 2 -> console |       | 2 -> console |      | 2 -> console       |
//    +--------------+       +--------------+      +--------------------+
//                                  ^                       ^
//                    slot 1 is now the lowest    fdalloc() must pick it
//                    free slot in the table      -- there is no choice
//
//  The guarantee is fdalloc() in kernel/sysfile.c, which scans p->ofile[]
//  from index 0 and returns the FIRST empty slot.  Because 0 is still taken
//  and 1 has just been vacated, the open() below CANNOT return anything but
//  1.  Nothing names the number 1 twice by coincidence -- close(1) makes
//  the open() deterministic.
//
//  Then exec() preserves the descriptor table, so echo inherits the rewired
//  fd 1 and writes into the file while believing it writes to standard
//  output.  Three properties combine, and removing any one breaks it:
//
//    fork()   gives a process whose descriptors we may safely damage
//    close+open   puts the file where the program will look for fd 1
//    exec()   keeps the table while replacing the program
//
//  Descriptor 2 is untouched, which is why an error from echo would still
//  reach the terminal.  `>` redirects only the descriptor you name -- the
//  reason ex1copy.c puts its hint on 2 rather than 1.
//
// ---------------------------------------------------------------------------
//  WHY THE PARENT IS UNAFFECTED
// ---------------------------------------------------------------------------
//
//  close(1) runs in the child, and the child's descriptor table is a COPY
//  made by fork().  Closing a slot there marks p->ofile[1] = 0 in one
//  struct proc only.  The parent's fd 1 still points at the console, which
//  is why its two messages appear on screen even though the child's output
//  went to a file.
//
//  This is precisely why the shell must fork before redirecting.  A shell
//  that closed its own fd 1 would lose the terminal for good, and every
//  later prompt would land in whatever file the last command mentioned.
//
// ---------------------------------------------------------------------------
//  WHY wait() GETS A NULL POINTER HERE
// ---------------------------------------------------------------------------
//
//  wait() writes the child's exit status through the pointer it is given,
//  and passing 0 tells the kernel to skip that store.  From kwait() in
//  kernel/proc.c:
//
//    if (addr != 0 && copyout(...) < 0) { ... }
//
//  So `wait(0)` is not a trick or a placeholder -- it is the documented way
//  to say "I do not want the status".  ex5forkexec.c wanted it and passed
//  &status; this program only needs to know the child has finished, so that
//  `cat ex6.out` afterwards sees a complete file.
//
//  The `(int *)` cast is style, not necessity.  A bare 0 is a valid null
//  pointer constant in C and compiles identically; the xv6 book writes
//  `wait((int *) 0)` to make the argument's type visible at the call site,
//  and user/sh.c writes plain `wait(0)`.  Both appear in this tree.  This
//  file keeps the cast because a lone `0` in an argument list reads like a
//  status VALUE rather than a null pointer.
//
// ---------------------------------------------------------------------------
//  WHERE EACH PIECE LIVES IN THIS TREE
// ---------------------------------------------------------------------------
//
//   Idea                        File
//   --------------------------  -------------------------------------------
//   the syscall stubs           user/usys.S  (load a7, then `ecall`)
//   fdalloc()'s lowest-free rule  kernel/sysfile.c
//   sys_close, sys_open         kernel/sysfile.c
//   exec preserving the table   kernel/exec.c  (p->ofile[] left alone)
//   the O_* flag values         kernel/fcntl.h
//   the same code in the shell  user/sh.c  (runcmd(), case REDIR)
//   a program that just uses 1  user/echo.c
//   this exercise, written up   docs/07-exercises.md
//   the long-form notes         docs/book/ch01-operating-system-interfaces.md
//
// ---------------------------------------------------------------------------
//  THREE THINGS WORTH KEEPING
// ---------------------------------------------------------------------------
//
//  * Redirection is not a feature of any program.  echo contains no code
//    for it; only sh does, and sh implements it entirely with close(),
//    open(), and the lowest-free-descriptor rule.
//  * Descriptors are indirection.  A program names 0, 1, and 2 and stays
//    ignorant of what they reach, which is what lets one binary work at a
//    terminal, in a pipeline, and against a file.
//  * The fork/exec split is what makes the window exist.  Redirection lives
//    in the few instructions between them.

#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "user/user.h"

int
main(void)
{
  int pid = fork();
  if (pid < 0) {
    fprintf(2, "ex6redirect: fork failed\n");
    return 1;
  }

  if (pid == 0) {
    // The child, and only the child.  The parent's descriptor 1 is a
    // separate entry in a separate table and is not touched by any of this.

    // Give up the console on descriptor 1.  After this, slot 1 is the
    // lowest free slot in this process's table.
    close(1);

    // fdalloc() therefore has to put the file in slot 1.  The return value
    // is not stored because it is not in doubt -- but it is still worth
    // checking that the open succeeded at all, since a failure here would
    // leave the child with NO descriptor 1 and echo's writes would silently
    // vanish into a closed slot.
    if (open("ex6.out", O_WRONLY | O_CREATE | O_TRUNC) != 1) {
      // fd 2, which close(1) did not disturb, so this is still visible.
      fprintf(2, "ex6redirect: cannot redirect to ex6.out\n");
      exit(1);
    }

    // echo knows nothing about any of the above.  It writes descriptor 1,
    // exactly as it would at a terminal, and exec() carries the rewired
    // table into it.
    char *argv[] = { "echo", "ex6redirect's", "redirected", "echo", 0 };
    exec("echo", argv);

    fprintf(2, "ex6redirect: exec echo failed\n");
    exit(1);
  }

  // The parent, still holding the console on descriptor 1.
  printf("ex6redirect: parent waiting for child %d\n", pid);

  // A null pointer: the status is genuinely not wanted here.  What matters
  // is the ordering -- returning only after the child is gone means the
  // file is complete and closed by the time the shell prompts again.
  wait((int *)0);

  printf("ex6redirect: child %d finished; try `cat ex6.out`\n", pid);
  return 0;
}
