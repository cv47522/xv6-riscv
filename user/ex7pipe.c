// ex7pipe.c: create a pipe and send bytes through it, within one process.
//
// Based on MIT 6.1810 Lecture 1's ex7.c, linked from the schedule on the
// course web site:
// https://pdos.csail.mit.edu/6.1810/2026/lec/l-overview/ex7.c.
// The host manual page for the same call is `man 2 pipe`.
//
// ---------------------------------------------------------------------------
//  RUNNING IT  (inside xv6, not on the host)
// ---------------------------------------------------------------------------
//
//    % make qemu
//    $ ex7pipe
//    ex7pipe: pipe() gave read fd 3, write fd 4
//    xyz
//    $
//
//  One process, writing to itself.  That is not what pipes are for -- the
//  point of a pipe is two processes, and ex8pipefork.c adds the fork().
//  This exercise exists to show the pipe alone, with the process question
//  held aside.
//
// ---------------------------------------------------------------------------
//  WHAT pipe() MAKES
// ---------------------------------------------------------------------------
//
//    pipe(fds)
//      |
//      +--> fds[0] ---> [ read end  ] ---+
//      |                                 |
//      |                                 +--> kernel buffer, 512 bytes
//      |                                 |    (struct pipe, kernel/pipe.c)
//      +--> fds[1] ---> [ write end ] ---+
//
//  Two descriptors, one object.  The buffer is a fixed-size circular queue
//  in kernel memory -- `char data[PIPESIZE]` with PIPESIZE 512 in
//  kernel/pipe.c -- not a file, and nothing about it touches the disk.
//  Nothing survives the last close: when both ends are gone, pipeclose()
//  frees the buffer.  There is no name for it in the file system, which is
//  why a pipe can only be passed to a child through fork().
//
//   fds[]   End      Direction        Notes
//   ------  -------  ---------------  -------------------------------------
//   fds[0]  read     out of the pipe  read() blocks while the buffer is
//                                     empty and a writer still exists
//   fds[1]  write    into the pipe    write() blocks while the buffer is
//                                     full and a reader still exists
//
//  The numbers are 3 and 4 on a normal run for the same reason ex2create.c
//  saw 3: fdalloc() hands out the lowest free slots, and 0, 1, and 2 are
//  already the console.
//
// ---------------------------------------------------------------------------
//  READ AND WRITE ARE THE SAME CALLS AS EVER
// ---------------------------------------------------------------------------
//
//  Nothing below uses a pipe-specific call.  read() and write() are the
//  ones from ex1copy.c, and they behave the same way: write() appends,
//  read() consumes, and the return value is a count that may be smaller
//  than asked for.  fileread() and filewrite() in kernel/file.c branch on
//  f->type and forward to piperead()/pipewrite() -- the caller never sees
//  the difference.
//
//  That uniformity is the abstraction doing its job.  `ls | grep x` works
//  because grep reads descriptor 0 with no idea a pipe is behind it.
//
// ---------------------------------------------------------------------------
//  WHY THIS ONE-PROCESS VERSION IS A TRAP
// ---------------------------------------------------------------------------
//
//  It works only because "xyz\n" is four bytes and the buffer holds 512.
//  Write more than PIPESIZE with nobody draining the other end and the
//  program hangs forever, on itself:
//
//    char big[1024];
//    pipe(fds);
//    write(fds[1], big, sizeof(big));   // fills 512 bytes, then sleeps
//                                       // waiting for a reader that is
//                                       // this same process, now asleep
//
//  pipewrite() in kernel/pipe.c sleeps at `pi->nwrite == pi->nread +
//  PIPESIZE` until a reader advances nread.  With one process there is no
//  other thread of control to do that, so the sleep never ends.  A pipe is
//  a rendezvous between two schedulable things; using one alone works
//  purely by staying under the buffer size.
//
//  Ctrl-p at the console dumps the process table (procdump() in
//  kernel/proc.c) and shows the stuck process in state `sleep`.
//
// ---------------------------------------------------------------------------
//  HOW EOF WORKS ON A PIPE
// ---------------------------------------------------------------------------
//
//  A pipe has no end-of-file marker in the data.  read() returns 0 only
//  when the buffer is empty AND every write-end descriptor has been closed
//  -- pipe->writeopen going to 0 in pipeclose().  Until then an empty pipe
//  means "wait", not "finished".
//
//  So closing the write end is not tidiness, it is how the reader is told
//  the stream is over.  A forgotten close(fds[1]) is the classic pipeline
//  hang, and it is why ex8pipefork.c closes ends it does not use.  This
//  program dodges the issue by reading a known byte count and never asking
//  for EOF at all.
//
// ---------------------------------------------------------------------------
//  WHERE EACH PIECE LIVES IN THIS TREE
// ---------------------------------------------------------------------------
//
//   Idea                        File
//   --------------------------  -------------------------------------------
//   the pipe stub               user/usys.S  (load a7, then `ecall`)
//   sys_pipe                    kernel/sysfile.c
//   the buffer and PIPESIZE     kernel/pipe.c  (struct pipe, 512 bytes)
//   pipealloc, piperead,        kernel/pipe.c
//     pipewrite, pipeclose
//   sleep/wakeup underneath     kernel/proc.c
//   read/write dispatch on type  kernel/file.c  (fileread(), filewrite())
//   pipelines in the shell      user/sh.c  (runcmd(), case PIPE)
//   this exercise, written up   docs/07-exercises.md
//   the long-form notes         docs/book/ch01-operating-system-interfaces.md
//
// ---------------------------------------------------------------------------
//  THREE THINGS WORTH KEEPING
// ---------------------------------------------------------------------------
//
//  * A pipe is a kernel buffer with two descriptors on it -- one to put
//    bytes in, one to take them out.  It has no name and no disk presence.
//  * read() and write() do not change.  A pipe is a new kind of thing
//    behind a descriptor, not a new interface in front of one.
//  * An empty pipe means "wait" until the last write end is closed, and
//    only then "EOF".  Closing unused ends is part of the protocol.

#include "kernel/types.h"
#include "user/user.h"

int
main(void)
{
  // Two descriptors come back through this array: fds[0] to read, fds[1] to
  // write.  Like wait()'s status, it is an OUT parameter -- pipe()'s return
  // value is only a success flag, so the descriptors need somewhere to go.
  int fds[2];
  char buf[100];

  if (pipe(fds) < 0) {
    // Fails when the process is out of descriptor slots (NOFILE is 16) or
    // the kernel is out of memory for the buffer.
    fprintf(2, "ex7pipe: pipe failed\n");
    return 1;
  }

  printf("ex7pipe: pipe() gave read fd %d, write fd %d\n", fds[0], fds[1]);

  // Four bytes, safely under PIPESIZE (512), so this returns immediately
  // rather than sleeping for a reader.  See the trap section above: the
  // margin is what makes a single-process pipe work at all.
  int written = write(fds[1], "xyz\n", 4);
  if (written != 4) {
    fprintf(2, "ex7pipe: write failed\n");
    return 1;
  }

  // Reads what is in the buffer now.  This does not block, because the four
  // bytes are already there -- had the buffer been empty, read() would have
  // slept, and with no other process nothing would ever wake it.
  int n = read(fds[0], buf, sizeof(buf));
  if (n < 0) {
    fprintf(2, "ex7pipe: read failed\n");
    return 1;
  }

  // Straight out to the console: same write(), different kind of
  // destination behind the descriptor.
  if (write(1, buf, n) != n) {
    fprintf(2, "ex7pipe: write to stdout failed\n");
    return 1;
  }

  // Closing both ends releases the kernel buffer through pipeclose().
  // exit() would do it, but a program that made a pipe per iteration would
  // exhaust NOFILE (16) without these.
  close(fds[0]);
  close(fds[1]);

  return 0;
}
