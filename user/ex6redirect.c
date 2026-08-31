// ex6redirect.c: implement echo output redirection with file descriptors.
//
// Based on MIT 6.1810 Lecture 1's ex6.c:
// https://pdos.csail.mit.edu/6.1810/2026/lec/l-overview/ex6.c.
// Host references: man 2 fork; man 2 open; man 3 execv.
//
// ---------------------------------------------------------------------------
//  PREREQUISITES
// ---------------------------------------------------------------------------
//
//  <OS> means the sibling repository at ../operating-system.
//
//   Kind  Read first
//   ----  ------------------------------------------------------------------
//   note  <OS>/The_Process_Abstraction.md  (section 7.4)
//   code  <OS>/codes/src/main/virtualization/cpu-process-api/fork_redirect.c
//
// ---------------------------------------------------------------------------
//  RUNNING IT  (inside xv6, not on the host)
// ---------------------------------------------------------------------------
//
//    % make qemu
//    $ ex6redirect
//    $ cat ex6.out
//    ex6redirect's redirected echo
//
// ---------------------------------------------------------------------------
//  HOW close(1) + open() IMPLEMENTS >
// ---------------------------------------------------------------------------
//
//   Child FD table        after close(1)       after open("ex6.out", ...)
//   -------------------   -------------------  -----------------------------
//   0 -> console input    0 -> console input   0 -> console input
//   1 -> console output   1 -> empty           1 -> ex6.out
//   2 -> console errors   2 -> console errors  2 -> console errors
//
//  fdalloc() always picks the lowest free slot. Once close(1) frees slot 1,
//  open() has no other answer. exec() then preserves the descriptor table, so
//  echo's ordinary write to fd 1 lands in ex6.out.
//
//   Operation      Why it is needed
//   -------------  --------------------------------------------------------
//   fork()         lets the child modify its table without breaking the shell
//   close/open     installs the output file at the conventional stdout slot
//   exec()         replaces the child but keeps the new descriptor mapping
//
// ---------------------------------------------------------------------------
//  WHERE EACH PIECE LIVES IN THIS TREE
// ---------------------------------------------------------------------------
//
//   Idea                        File
//   --------------------------  -------------------------------------------
//   lowest-free allocation      kernel/sysfile.c  (fdalloc())
//   descriptor close/open       kernel/sysfile.c
//   fork copying FD entries     kernel/proc.c  (kfork(), filedup())
//   exec preserving FDs         kernel/exec.c
//   shell redirection           user/sh.c  (runcmd(), REDIR)
//   exercise details            docs/07-exercises.md
//
// ---------------------------------------------------------------------------
//  THREE THINGS WORTH KEEPING
// ---------------------------------------------------------------------------
//
//  * The child changes fd 1 because commands already treat it as stdout.
//  * fork() confines that change to the child; the parent's output stays on
//    the console.
//  * fd indirection lets the shell own redirection while echo remains unaware.

#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "user/user.h"

int
main(void)
{
  int pid = fork(); /* fork() copies the same fd table from parent process to child process */
  if (pid < 0) {
    fprintf(2, "ex6redirect: fork failed\n");
    return 1;
  }

  if (pid == 0) {
    // Free stdout so fdalloc() installs the new file in exactly slot 1.
    close(1);

    if (open("ex6.out", O_WRONLY | O_CREATE | O_TRUNC) != 1) { /* 0,2 are taken, so open() takes 1 */
      fprintf(2, "ex6redirect: cannot redirect to ex6.out\n");
      exit(1);
    }

    char *argv[] = { "echo", "ex6redirect's", "redirected", "echo", 0 };
    exec("echo", argv);

    // fd 2 still names the console, so exec failures remain visible.
    fprintf(2, "ex6redirect: exec echo failed\n");
    exit(1);
  }

  printf("ex6redirect: parent waiting for child %d\n", pid);

  // A null status pointer says completion matters but the exit value does not.
  wait((int *)0);

  printf("ex6redirect: child %d finished; try `cat ex6.out`\n", pid);
  return 0;
}
