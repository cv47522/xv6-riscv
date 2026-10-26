/*
 * ex5forkexec.c: run echo with the shell's fork/exec/wait pattern.
 *
 * Based on MIT 6.1810 Lecture 1's ex5.c:
 * https://pdos.csail.mit.edu/6.1810/2026/lec/l-overview/ex5.c.
 * Host references: man 2 fork; man 3 execv; man 2 wait.
 *
 * ---------------------------------------------------------------------------
 *  PREREQUISITES
 * ---------------------------------------------------------------------------
 *
 *   Kind  Read first
 *   ----  ------------------------------------------------------------------
 *   note  ../operating-system/The_Process_Abstraction.md  (sections 7.1-7.4)
 *   code  ../operating-system/codes/src/main/virtualization/cpu-process-api/
 *           fork_exec.c and fork_wait.c
 *
 * ---------------------------------------------------------------------------
 *  RUNNING IT  (inside xv6, not on the host)
 * ---------------------------------------------------------------------------
 *
 *    % make qemu
 *    $ ex5forkexec
 *    ex5forkexec: parent waiting for child <pid>
 *    THIS IS ECHO
 *    ex5forkexec: child <pid> exited with status 0
 *
 * ---------------------------------------------------------------------------
 *  THE SHELL'S CONTROL FLOW
 * ---------------------------------------------------------------------------
 *
 *    shell-like parent
 *          |
 *          v
 *       fork()                  child inherits a copy of the fd table
 *         |
 *         +----> child: exec("echo") -> writes THIS IS ECHO
 *         |
 *         +----> parent: printf("waiting for child N")  <-- races with child
 *                          |
 *                          v
 *                        wait(&status)                <-- ordering starts
 *                          |
 *                          v
 *                        report pid and status
 *
 * The shell must survive: direct exec() would replace it, ending prompts.
 * The fork-to-exec gap is where shells install redirection and pipes.
 *
 * ---------------------------------------------------------------------------
 *  WAIT AND REAPING
 * ---------------------------------------------------------------------------
 *
 *   Result       Meaning
 *   -----------  ----------------------------------------------------------
 *   return > 0   reaped child's pid; status is filled in
 *   return -1    no children, caller killed, or status copyout() failed
 *                (bad address; kernel/proc.c kwait())
 *   return 0     never: no nonblocking WNOHANG option
 *   status       raw 32-bit exit() value: 0 means success, nonzero failure
 *
 * The pid returns directly; status leaves through a pointer:
 *
 *    int status;               wait() writes it before we read it
 *    int pid = wait(&status);  pid comes back, status is filled in
 *
 * Check -1 before status: failure writes nothing, so its initial 0 here
 * would falsely suggest success. With wait(0) and only the pid printed,
 * -1 prints as -1; checking is optional (the prerequisite fork_exec.c).
 * Linux packs exit code and signal into status, requiring WIFEXITED() and
 * WEXITSTATUS(); xv6 has no signals, and kill() forces exit(-1).
 * An exited child stays a zombie until wait() collects status and frees
 * its slot; wait() blocks if needed before returning.
 *
 * ---------------------------------------------------------------------------
 *  WHERE EACH PIECE LIVES IN THIS TREE
 * ---------------------------------------------------------------------------
 *
 *   Idea                        File
 *   --------------------------  -------------------------------------------
 *   fork/exec/wait stubs        user/usys.S
 *   child creation              kernel/proc.c  (kfork())
 *   executable replacement      kernel/exec.c  (kexec())
 *   exit and reaping            kernel/proc.c  (kexit(), kwait())
 *   shell command execution     user/sh.c
 *   exercise details            docs/07-exercises.md
 */

#include "kernel/types.h"
#include "user/user.h"

int
main(void)
{
  int status = 0;
  int pid = fork();

  if (pid < 0) {
    fprintf(2, "ex5forkexec: fork failed\n");
    exit(1);
  }

  if (pid == 0) {
    char *argv[] = { "echo", "THIS", "IS", "ECHO", 0 };
    exec("echo", argv);

    // Only an exec failure leaves the child running this program.
    fprintf(2, "ex5forkexec: exec echo failed\n");
    exit(1);
  }

  printf("ex5forkexec: parent waiting for child %d\n", pid);

  int reaped_pid = wait(&status);
  if (reaped_pid < 0) {
    fprintf(2, "ex5forkexec: wait failed\n");
    exit(1);
  }

  printf("ex5forkexec: child %d exited with status %d\n", reaped_pid, status);
  exit(0);
}

/*
 * ---------------------------------------------------------------------------
 *  TWO RUNTIME SURPRISES
 * ---------------------------------------------------------------------------
 *
 *    $ ex5forkexec
 *    ex5forkTHIS IS ECHO
 *    exec: parent waiting for child 5
 *    ex5forkexec: child 5 exited with status 0
 *    $ ex5forkexec
 *    exec ex5forkexec failed
 *    $ ex5forkexec
 *    ex5forkexec: parenTHIS t ISw aECHO
 *    iting for child 8
 *    ex5forkexec: child 8 exited with status 0
 *
 * 1. Mid-word interleaving: the first printf() precedes wait() (see header
 *    flow). user/printf.c writes one byte per write(); kernel/uart.c's
 *    uartwrite() locks each write, not a whole printf(). CPUS=3 can mix
 *    output; the post-wait status cannot run early.
 * 2. `exec ex5forkexec failed` is from user/sh.c, not this program. Pasted
 *    bursts can overflow kernel/console.c's 128-byte input buffer:
 *    consoleintr() drops excess characters, potentially corrupting the name
 *    sh receives. Per-prompt runs avoid that input loss; this program's
 *    fork(), exec(), and wait() are not the failing operations.
 */
