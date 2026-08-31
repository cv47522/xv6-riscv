// ex7pipe.c: send bytes through a pipe within one process.
//
// Based on MIT 6.1810 Lecture 1's ex7.c:
// https://pdos.csail.mit.edu/6.1810/2026/lec/l-overview/ex7.c.
// Host references: man 2 pipe; man 7 pipe.
//
// ---------------------------------------------------------------------------
//  PREREQUISITES
// ---------------------------------------------------------------------------
//
//  <OS> means the sibling repository at ../operating-system.
//
//   Kind  Read first
//   ----  ------------------------------------------------------------------
//   note  <OS>/The_Process_Abstraction.md  (section 7.6)
//   code  <OS>/codes/src/main/virtualization/cpu-process-api/
//           pipe_family/pipe_basic.c
//
// ---------------------------------------------------------------------------
//  RUNNING IT  (inside xv6, not on the host)
// ---------------------------------------------------------------------------
//
//    % make qemu
//    $ ex7pipe
//    ex7pipe: pipe() gave read fd 3, write fd 4
//    xyz
//
// ---------------------------------------------------------------------------
//  WHAT pipe(fds) CREATES
// ---------------------------------------------------------------------------
//
//    fds[1]: write end
//          |
//          |  write appends bytes
//          v
//    +-----------------------------+
//    | kernel/pipe.c buffer        |  PIPESIZE = 512 bytes
//    +-----------------------------+
//          |
//          |  read removes bytes
//          v
//    fds[0]: read end
//
//   End     Direction            Blocks when
//   ------  -------------------  -------------------------------------------
//   fds[0]  out of the buffer    empty while any write end remains open
//   fds[1]  into the buffer      full while any read end remains open
//
//  The pipe has no pathname and touches no disk. read() returns 0 only when
//  the buffer is empty and every write-end descriptor has closed.
//
//  This one-process demo writes before it reads and must stay below PIPESIZE.
//  A larger write can fill the buffer and sleep while its only reader is the
//  same sleeping process.
//
// ---------------------------------------------------------------------------
//  WHERE EACH PIECE LIVES IN THIS TREE
// ---------------------------------------------------------------------------
//
//   Idea                        File
//   --------------------------  -------------------------------------------
//   pipe/read/write stubs       user/usys.S
//   descriptor allocation       kernel/sysfile.c  (sys_pipe())
//   buffer and blocking rules   kernel/pipe.c
//   open-file dispatch          kernel/file.c
//   pipe buffer size            kernel/pipe.c  (PIPESIZE)
//   exercise details            docs/07-exercises.md
//
// ---------------------------------------------------------------------------
//  THREE THINGS WORTH KEEPING
// ---------------------------------------------------------------------------
//
//  * pipe() returns two descriptors onto one bounded kernel byte buffer.
//  * The usual read()/write() interface hides whether an FD names a file,
//    device, or pipe.
//  * Empty is not EOF while a writer exists; closing descriptors is protocol.

#include "kernel/types.h"
#include "user/user.h"

int
main(void)
{
  int fds[2];
  char buf[100];

  if (pipe(fds) < 0) { /* 0,1,2 are taken, so pipe() takes 3 & 4 */
    fprintf(2, "ex7pipe: pipe failed\n");
    return 1;
  }

  printf("ex7pipe: pipe() gave read fd %d, write fd %d\n", fds[0], fds[1]);

  int written = write(fds[1], "xyz\n", 4); // Write to FD 4 (pipe write end fds[1])
  if (written != 4) {
    fprintf(2, "ex7pipe: write failed\n");
    return 1;
  }

  int n = read(fds[0], buf, sizeof(buf));
  if (n < 0) {
    fprintf(2, "ex7pipe: read failed\n");
    return 1;
  }

  // Preserve the exact byte count; a pipe does not add a string terminator.
  if (write(1, buf, n) != n) { // Write to FD 1 (console)
    fprintf(2, "ex7pipe: write to stdout failed\n");
    return 1;
  }

  close(fds[0]);
  close(fds[1]);
  return 0;
}
