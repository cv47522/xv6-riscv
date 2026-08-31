// ex5forkexec.c: run echo with the shell's fork/exec/wait pattern.
//
// Based on MIT 6.1810 Lecture 1's ex5.c:
// https://pdos.csail.mit.edu/6.1810/2026/lec/l-overview/ex5.c.
// Host references: man 2 fork; man 3 execv; man 2 wait.
//
// ---------------------------------------------------------------------------
//  PREREQUISITES
// ---------------------------------------------------------------------------
//
//   Kind  Read first
//   ----  ------------------------------------------------------------------
//   note  ../operating-system/The_Process_Abstraction.md  (sections 7.1-7.4)
//   code  ../operating-system/codes/src/main/virtualization/cpu-process-api/
//           fork_exec.c and fork_wait.c
//
// ---------------------------------------------------------------------------
//  RUNNING IT  (inside xv6, not on the host)
// ---------------------------------------------------------------------------
//
//    % make qemu
//    $ ex5forkexec
//    ex5forkexec: parent waiting for child <pid>
//    THIS IS ECHO
//    ex5forkexec: child <pid> exited with status 0
//
// ---------------------------------------------------------------------------
//  THE SHELL'S CONTROL FLOW
// ---------------------------------------------------------------------------
//
//    shell-like parent
//          |
//          v
//       fork()
//         |
//         +----> child ----> exec() ----> echo
//         |
//         +----> parent ---> wait(&status)  ---- blocks until child exits
//                              |
//                              v
//                         report pid and status
//
//  A shell cannot exec directly: it would become the command and never print
//  another prompt. The child is the process it can afford to replace.
//
// ---------------------------------------------------------------------------
//  wait(&status)
// ---------------------------------------------------------------------------
//
//   Result       Meaning
//   -----------  ----------------------------------------------------------
//   return > 0   pid of the child that was reaped
//   return -1    the caller has no children
//   status       the child's 32-bit exit value; 0 conventionally means success
//
//  The return value identifies the child, so its status leaves through a
//  pointer. Check for -1 first: on failure the kernel did not write status.
//
//    int status;               wait() writes it before we read it
//    int pid = wait(&status);  pid comes back, status is filled in
//
//  An exited child remains a zombie until wait() collects its 32-bit status
//  and frees its slot. By convention, 0 means success and nonzero failure.
//
// ---------------------------------------------------------------------------
//  WHERE EACH PIECE LIVES IN THIS TREE
// ---------------------------------------------------------------------------
//
//   Idea                        File
//   --------------------------  -------------------------------------------
//   fork/exec/wait stubs        user/usys.S
//   child creation              kernel/proc.c  (kfork())
//   executable replacement      kernel/exec.c  (kexec())
//   exit and reaping            kernel/proc.c  (kexit(), kwait())
//   shell command execution     user/sh.c
//   exercise details            docs/07-exercises.md
//
// ---------------------------------------------------------------------------
//  KEY POINTS
// ---------------------------------------------------------------------------
//
//  * fork() keeps the shell alive; exec() replaces only the child.
//  * wait() orders the parent after child exit and releases the zombie.
//  * The fork-to-exec gap is where shells later install redirection and pipes.

#include "kernel/types.h"
#include "user/user.h"

int
main(void)
{
  int status = 0;
  // The child inherits a copy of the parent's descriptor table.
  int pid = fork();

  if (pid < 0) {
    fprintf(2, "ex5forkexec: fork failed\n");
    return 1;
  }

  if (pid == 0) {
    char *argv[] = { "echo", "THIS", "IS", "ECHO", 0 };
    // Only the child execs; the parent must survive to wait and continue.
    exec("echo", argv);

    // Only an exec failure leaves the child running this program.
    fprintf(2, "ex5forkexec: exec echo failed\n");
    exit(1);
  }

  printf("ex5forkexec: parent waiting for child %d\n", pid);

  // wait() blocks if needed, returns a pid, and writes status separately.
  int reaped = wait(&status);
  if (reaped < 0) {
    fprintf(2, "ex5forkexec: wait failed\n");
    return 1;
  }

  printf("ex5forkexec: child %d exited with status %d\n", reaped, status);
  return 0;
}

// ---------------------------------------------------------------------------
//  TWO RUNTIME SURPRISES
// ---------------------------------------------------------------------------
//
//    $ ex5forkexec
//    ex5forkTHIS IS ECHO
//    exec: parent waiting for child 5
//    ex5forkexec: child 5 exited with status 0
//    $ ex5forkexec
//    exec ex5forkexec failed
//    $ ex5forkexec
//    ex5forkexec: parenTHIS t ISw aECHO
//    iting for child 8
//    ex5forkexec: child 8 exited with status 0
//
//  1. Output may interleave mid-word:
//
//    fork()
//      |
//      +-- child:  exec("echo") -> writes THIS IS ECHO
//      +-- parent: printf("waiting for child N")
//                  |
//                  v
//                  wait(&status)        <-- ordering starts here
//                  printf("child N exited ...")
//
//  The parent's first printf() races with the child because it precedes
//  wait(). printf() is not atomic, and uartputc() locks per character, so
//  output can mix with CPUS=3. The post-wait status line cannot run early.
//
//  2. `exec ex5forkexec failed` comes from user/sh.c, not this program.
//  A pasted burst can overflow kernel/console.c's 128-byte input buffer;
//  consoleintr() drops excess characters, and sh may receive a corrupt name.
//  Per-prompt runs avoid this input loss: fork(), exec(), and wait() are not
//  the failing operations.
