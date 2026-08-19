// ex8pipefork.c: two processes talking over a pipe.
//
// Based on MIT 6.1810 Lecture 1's ex8.c, linked from the schedule on the
// course web site:
// https://pdos.csail.mit.edu/6.1810/2026/lec/l-overview/ex8.c.
// This is the pipe half of what the shell does for `ls | grep x`.
//
// ---------------------------------------------------------------------------
//  RUNNING IT  (inside xv6, not on the host)
// ---------------------------------------------------------------------------
//
//    % make qemu
//    $ ex8pipefork
//    ex8pipefork: parent read 17 bytes: hello from child
//    $
//
//  The bytes crossed a process boundary.  The child cannot see the parent's
//  variables and the parent cannot see the child's -- fork() gave each its
//  own memory -- so the pipe is the only channel between them.
//
// ---------------------------------------------------------------------------
//  WHY pipe() COMES BEFORE fork()
// ---------------------------------------------------------------------------
//
//  Because fork() is the only way to hand a pipe to another process.  A
//  pipe has no name in the file system, so a process cannot open() one that
//  somebody else created.  The only route is inheritance: make the pipe
//  first, then fork, and the child's copied descriptor table already points
//  at the same kernel buffer.
//
//    1. pipe(fds)                    2. fork()
//
//    parent                          parent            child
//    +-----------+                   +-----------+     +-----------+
//    | 3 -> read |                   | 3 -> read |     | 3 -> read |
//    | 4 -> write|                   | 4 -> write|     | 4 -> write|
//    +-----------+                   +-----------+     +-----------+
//          |                               \               /
//          v                                \             /
//     [ 512-byte buffer ]                    [ same buffer ]
//
//  Reversing the order would give each process a private, unconnected pipe
//  -- two buffers, no communication.
//
//  After the fork there are FOUR descriptors on one pipe: two read ends and
//  two write ends.  That is one more of each than the design needs, and
//  the extras are not harmless.
//
// ---------------------------------------------------------------------------
//  WHY EACH SIDE CLOSES THE END IT DOES NOT USE
// ---------------------------------------------------------------------------
//
//  read() on a pipe returns 0 -- EOF -- only when the buffer is empty AND
//  every write-end descriptor anywhere has been closed.  piperead() in
//  kernel/pipe.c sleeps while `pi->nread == pi->nwrite && pi->writeopen`,
//  and pipeclose() clears writeopen only when the LAST writer goes away.
//
//  So the parent holding its own copy of the write end is enough to keep
//  the pipe open forever, even after the child exits.  A reader that waits
//  for EOF would then sleep for good, waiting on a writer that is itself:
//
//    parent forgets close(fds[1])
//      -> child exits, its write end closes
//      -> writeopen is still 1, because the parent still holds one
//      -> parent's read() sees an empty buffer and sleeps
//      -> nothing will ever write, and nothing will ever close
//
//  This program reads a fixed byte count and so would survive the mistake,
//  but the closes are here anyway, because the habit is the lesson: in a
//  real pipeline the reader always waits for EOF.  Ctrl-p dumps the process
//  table if you want to see what the hang looks like.
//
//  A mirror-image rule holds on the other side.  If every read end closes
//  while a writer is still writing, pipewrite() sets the writer's
//  p->killed and write() returns -1 -- the xv6 stand-in for SIGPIPE, and
//  what `ex1copy < README | echo done` exercises in grade-ex1copy.
//
// ---------------------------------------------------------------------------
//  THE FULL SEQUENCE
// ---------------------------------------------------------------------------
//
//    parent                               child
//    ------                               -----
//    pipe(fds)
//    fork()          ------------------>  (has both ends too)
//    close(fds[1])   drop write end       close(fds[0])   drop read end
//    read(fds[0])    sleeps, empty        write(fds[1], msg)
//                    <-------------------- bytes land in the buffer
//    read() returns  n bytes              close(fds[1])   -> EOF for parent
//    close(fds[0])                        exit(0)
//    wait(0)         reap the child
//
//  This is one direction only.  A pipe is not bidirectional -- writing to
//  fds[0] fails -- so two-way conversation needs two pipes, which is
//  exactly the chapter 1 exercise in the xv6 book: ping-pong a byte between
//  two processes over a pair of pipes.
//
// ---------------------------------------------------------------------------
//  HOW THE SHELL GOES FURTHER
// ---------------------------------------------------------------------------
//
//  `ls | grep x` needs one more step than this file shows.  Here the parent
//  reads descriptor 3 knowingly; grep must read descriptor 0, because that
//  is all any program knows.  The shell bridges the gap with dup():
//
//    close(0);          give up the console
//    dup(p[0]);         the pipe's read end lands in slot 0
//    close(p[0]);       drop the now-redundant original
//    close(p[1]);       and the write end, so EOF can happen
//    exec("grep", argv);
//
//  Same lowest-free-descriptor rule as ex6redirect.c, with dup() in place
//  of open() because the target is an existing descriptor rather than a
//  file.  See user/sh.c, runcmd(), case PIPE.
//
// ---------------------------------------------------------------------------
//  WHERE EACH PIECE LIVES IN THIS TREE
// ---------------------------------------------------------------------------
//
//   Idea                        File
//   --------------------------  -------------------------------------------
//   the pipe/fork/dup stubs     user/usys.S  (load a7, then `ecall`)
//   sys_pipe, sys_dup           kernel/sysfile.c
//   piperead, pipewrite,        kernel/pipe.c  (writeopen, readopen)
//     pipeclose, PIPESIZE
//   fork copying the FD table   kernel/proc.c  (kfork(), filedup())
//   sleep and wakeup            kernel/proc.c
//   pipelines in the shell      user/sh.c  (runcmd(), case PIPE)
//   the SIGPIPE stand-in        kernel/pipe.c  (p->killed in pipewrite())
//   this exercise, written up   docs/07-exercises.md
//   the long-form notes         docs/book/ch01-operating-system-interfaces.md
//
// ---------------------------------------------------------------------------
//  THREE THINGS WORTH KEEPING
// ---------------------------------------------------------------------------
//
//  * A pipe has no name, so fork() is the only way to share one.  Create it
//    before forking or the two processes get unrelated pipes.
//  * Closing unused ends is protocol, not hygiene: EOF is defined as "the
//    last write end closed", so a stray descriptor suppresses it.
//  * A pipe carries bytes in one direction between two schedulable
//    processes.  Two directions need two pipes.

#include "kernel/types.h"
#include "user/user.h"

int
main(void)
{
  int fds[2];
  char buf[100];

  // Before the fork, so that the child inherits descriptors onto the same
  // kernel buffer.  After the fork it would be two separate pipes.
  if (pipe(fds) < 0) {
    fprintf(2, "ex8pipefork: pipe failed\n");
    return 1;
  }

  int pid = fork();
  if (pid < 0) {
    fprintf(2, "ex8pipefork: fork failed\n");
    return 1;
  }

  if (pid == 0) {
    // ------------------------------------------------------------------
    //  Child: the writer.
    // ------------------------------------------------------------------

    // This process will never read the pipe, and a read end left open here
    // would keep the pipe alive past the point where the parent should see
    // it end.
    close(fds[0]);

    char msg[64];
    // No snprintf in xv6 -- user/user.h has printf and fprintf only, both
    // of which write to a descriptor.  So the message is assembled by hand.
    // strcpy() and strlen() come from user/ulib.c.
    strcpy(msg, "hello from child\n");
    int len = strlen(msg);

    if (write(fds[1], msg, len) != len) {
      fprintf(2, "ex8pipefork: child write failed\n");
      exit(1);
    }

    // The close that makes EOF possible.  With the parent's copy already
    // gone, this drops writeopen to 0 and any blocked read() returns 0.
    close(fds[1]);
    exit(0);
  }

  // --------------------------------------------------------------------
  //  Parent: the reader.
  // --------------------------------------------------------------------

  // The parent never writes.  Dropping this copy is what lets the child's
  // close(fds[1]) actually signal end-of-file rather than leaving one
  // writer standing.
  close(fds[1]);

  // Sleeps in piperead() until the child writes.  The bytes may arrive in
  // more than one read() on a busy system -- a pipe is a byte stream with
  // no message boundaries, so a loop is the honest way to drain it.
  int n = read(fds[0], buf, sizeof(buf));
  if (n < 0) {
    fprintf(2, "ex8pipefork: read failed\n");
    return 1;
  }

  printf("ex8pipefork: parent read %d bytes: ", n);
  if (write(1, buf, n) != n) {
    fprintf(2, "ex8pipefork: write to stdout failed\n");
    return 1;
  }

  close(fds[0]);

  // Reap the child, so it does not linger as a ZOMBIE and so the shell's
  // prompt cannot appear between our output and the child's exit.
  wait((int *)0);

  return 0;
}
