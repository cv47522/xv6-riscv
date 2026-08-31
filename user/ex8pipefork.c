// ex8pipefork.c: pass bytes from a child to its parent through a pipe.
//
// Based on MIT 6.1810 Lecture 1's ex8.c:
// https://pdos.csail.mit.edu/6.1810/2026/lec/l-overview/ex8.c.
// Host references: man 2 pipe; man 2 fork; man 7 pipe.
//
// ---------------------------------------------------------------------------
//  PREREQUISITES
// ---------------------------------------------------------------------------
//
//   Kind  Read first
//   ----  ------------------------------------------------------------------
//   note  ../operating-system/The_Process_Abstraction.md  (sections 7.6, 7.8)
//   code  ../operating-system/codes/src/main/virtualization/cpu-process-api/
//           pipe_family/pipe_three_closes.c
//   code  ../operating-system/codes/src/main/virtualization/cpu-process-api/
//           homework/
//           08_parent_child_print_pipe.c
//
// ---------------------------------------------------------------------------
//  RUNNING IT  (inside xv6, not on the host)
// ---------------------------------------------------------------------------
//
//    % make qemu
//    $ ex8pipefork
//    ex8pipefork: parent read 17 bytes: hello from child
//
// ---------------------------------------------------------------------------
//  CREATE THE PIPE BEFORE fork()
// ---------------------------------------------------------------------------
//
//  A pipe has no name, so inheritance shares it. Creating it before fork()
//  makes both descriptor tables refer to one kernel buffer.
//
//   Process  Read-end copy   Write-end copy   Role after unused-end close
//   -------  --------------  ---------------  -----------------------------
//   parent   fds[0]          fds[1]           keeps fds[0], reads
//   child    fds[0]          fds[1]           keeps fds[1], writes
//
//    child                        shared pipe                    parent
//      |                              |                           |
//      | close(fds[0])               |            close(fds[1]) |
//      | write(fds[1], msg) -------->|------------------------->| read(fds[0])
//      | close(fds[1])               |                           |
//      v                              v                           v
//    exits                    last writer closes              wait() reaps
//
// ---------------------------------------------------------------------------
//  WHY UNUSED ENDS MUST CLOSE
// ---------------------------------------------------------------------------
//
//  piperead() treats an empty buffer in two different ways:
//
//   Writers left  Result
//   ------------  ----------------------------------------------------------
//   one or more   sleep: a holder could still write later
//   none          return 0: EOF
//
//  A reader retaining a write-end copy can prevent its own EOF. This fixed
//  read does not need EOF, but streaming pipelines do.
//
// ---------------------------------------------------------------------------
//  WHERE EACH PIECE LIVES IN THIS TREE
// ---------------------------------------------------------------------------
//
//   Idea                        File
//   --------------------------  -------------------------------------------
//   pipe/fork/read/write        user/usys.S
//   inherited FD references    kernel/proc.c  (kfork(), filedup())
//   buffer, EOF, wakeups        kernel/pipe.c
//   descriptor close           kernel/sysfile.c
//   shell pipelines             user/sh.c  (runcmd(), PIPE)
//   exercise details            docs/07-exercises.md
//
// ---------------------------------------------------------------------------
//  KEY POINTS
// ---------------------------------------------------------------------------
//
//  * pipe() before fork() gives isolated processes one controlled channel.
//  * fork() makes four descriptor references onto the one shared buffer.
//  * Close unused ends so reference counts can express end-of-stream.

#include "kernel/types.h"
#include "user/user.h"

int
main(void)
{
  int fds[2];
  char buf[100];

  // With fds 0-2 taken, pipe() normally returns read fd 3 and write fd 4.
  if (pipe(fds) < 0) {
    fprintf(2, "ex8pipefork: pipe failed\n");
    return 1;
  }

  // Both processes inherit references to this same pipe.
  int pid = fork();
  if (pid < 0) {
    fprintf(2, "ex8pipefork: fork failed\n");
    return 1;
  }

  // The roles may swap if their close() calls swap too. Here the child writes,
  // and the surviving parent reads, reports the result, and reaps the child.
  if (pid == 0) {
    // The child writes only; it must not retain an unused read reference.
    close(fds[0]);

    char msg[64];
    strcpy(msg, "hello from child\n");
    int len = strlen(msg);

    // fds[1] is the pipe's write end.
    if (write(fds[1], msg, len) != len) {
      fprintf(2, "ex8pipefork: child write failed\n");
      exit(1);
    }

    // With the parent's writer closed too, this makes EOF reachable.
    close(fds[1]);
    exit(0);
  }

  // The parent reads only; retaining this writer could postpone EOF.
  close(fds[1]);

  int n = read(fds[0], buf, sizeof(buf));
  if (n < 0) {
    fprintf(2, "ex8pipefork: read failed\n");
    return 1;
  }

  printf("ex8pipefork: parent read %d bytes: ", n);
  // fd 1 remains the console.
  if (write(1, buf, n) != n) {
    fprintf(2, "ex8pipefork: write to stdout failed\n");
    return 1;
  }

  close(fds[0]);

  // Read before wait(). A write over PIPESIZE (512 in kernel/pipe.c) blocks
  // until a reader drains the buffer; waiting first could deadlock:
  //
  //    child:  write(fds[1], ...)  -- asleep in pipewrite(), buffer full
  //    parent: wait()              -- asleep until the child exits
  //            (nobody reads, so neither wakes)
  wait((int *)0);
  return 0;
}
