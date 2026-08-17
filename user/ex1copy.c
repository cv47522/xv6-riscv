// ex1copy.c: copy standard input to standard output until EOF.
//
// Based on MIT 6.1810 Lecture 1's ex1.c, linked from the schedule on the
// course web site:
// https://pdos.csail.mit.edu/6.1810/2026/lec/l-overview/ex1.c.
// The host manual pages for the same two calls are `man 2 read` and
// `man 2 write`.
//
// ---------------------------------------------------------------------------
//  RUNNING IT  (inside xv6, not on the host)
// ---------------------------------------------------------------------------
//
//    % make qemu
//    $ ex1copy              interactive; Ctrl-d on an empty line ends it
//    $ echo hi | ex1copy    the pipe supplies EOF when echo exits
//    $ ex1copy < README     the file supplies EOF at end of file
//
// ---------------------------------------------------------------------------
//  THE DATA PATH
// ---------------------------------------------------------------------------
//
//    fd 0  <-- keyboard, pipe, or file
//      |
//      |   read(0, buf, sizeof(buf))        ---- ecall ---> kernel
//      |   returns n: >0 bytes, 0 at EOF, -1 on error
//      v
//    char buf[64]    untyped bytes; this program never looks inside
//      |
//      |   write(1, buf, n)                 ---- ecall ---> kernel
//      |   returns n, or -1 on error
//      v
//    fd 1  --> console, pipe, or file
//
// ---------------------------------------------------------------------------
//  read(fd, addr, n)  AND  write(fd, addr, n)
// ---------------------------------------------------------------------------
//
//   Argument  Is                          Rule
//   --------  --------------------------  ----------------------------------
//   fd        which open file to touch    a small int; must already be open
//   addr      memory to read into, or     &buf[0] here; the kernel copies
//             to write from               across the user/kernel boundary
//   n         the MAXIMUM byte count      may transfer fewer, never more
//
//   Return    Means                       What this program does
//   --------  --------------------------  ----------------------------------
//    > 0      bytes actually transferred  copy exactly that many, not 64
//      0      EOF: no more will arrive    leave the loop and return 0
//     -1      the call failed             report on fd 2 and return 1
//
// ---------------------------------------------------------------------------
//  FILE DESCRIPTORS
// ---------------------------------------------------------------------------
//
//  An FD is a small integer the kernel uses to find one already-open file.
//  It indexes p->ofile[NOFILE] in kernel/proc.h, so one process can hold up
//  to NOFILE (16) of them at once.
//
//   FD  Unix convention   Opened by             Used here for
//   --  ----------------  --------------------  ---------------------------
//    0  standard input    user/sh.c, pre-exec   everything we read
//    1  standard output   user/sh.c, pre-exec   everything we write
//    2  standard error    user/sh.c, pre-exec   diagnostics and the hint
//
//  The convention is the whole trick.  Because this program only ever names
//  0 and 1, it never has to know whether the other end is a keyboard, a
//  file, or half of a pipe: sh wires the descriptors up before exec'ing us,
//  and all three command lines above reach identical code.
//
// ---------------------------------------------------------------------------
//  WHERE EACH PIECE LIVES IN THIS TREE
// ---------------------------------------------------------------------------
//
//   Idea                        File
//   --------------------------  -------------------------------------------
//   the read/write stubs        user/usys.S  (load a7, then `ecall`)
//   system call dispatch        kernel/syscall.c
//   sys_read and sys_write      kernel/sysfile.c
//   per-process FD table        kernel/proc.h  (p->ofile[NOFILE])
//   one open file               kernel/file.c  (struct file, filestat())
//   console line buffering      kernel/console.c
//   descriptor plumbing         user/sh.c  (redirection and pipelines)
//   the long-form notes         docs/book/ch01-operating-system-interfaces.md
//
// ---------------------------------------------------------------------------
//  THREE THINGS WORTH KEEPING
// ---------------------------------------------------------------------------
//
//  * C is the language here because kernels are written in C: it compiles to
//    predictable machine code and needs no runtime beneath it, which is what
//    lets kernel/ and user/ share headers such as kernel/types.h.
//  * read() and write() look exactly like function calls in C source, but
//    they trap into the kernel instead of running in this process.  That
//    trap is what preserves user/kernel isolation: the kernel decides what
//    the call may touch, and the caller never gains privilege.
//  * Unix I/O is untyped 8-bit bytes.  There are no lines, records, or
//    character encodings at this layer.  Whether the bytes are C source, a
//    database row, or a JPEG is the application's problem, which is exactly
//    why this program never inspects buf.
//
//  Still open: how do you make a *new* file descriptor?  open(), pipe(), and
//  dup(), all declared in user/user.h.  user/sh.c builds every redirection
//  and pipeline out of those three plus close().

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(void)
{
  struct stat st;
  char buf[64];
  int n;

  // A filter prints no prompt, so an interactive run is indistinguishable
  // from a hung program until you know to start typing.  Say so -- but only
  // when standard input really is the console, and only on fd 2, so the hint
  // can never contaminate the copied data on fd 1.
  //
  // fstat() on fd 0 separates the three cases by itself:
  //
  //   fd 0 is        fstat(0, &st)   st.type     hint?
  //   -------------  --------------  ----------  -----
  //   the console    returns 0       T_DEVICE    yes
  //   a file         returns 0       T_FILE      no
  //   a pipe         returns -1      --          no
  //
  // The pipe case falls out of kernel/file.c: filestat() serves FD_INODE and
  // FD_DEVICE and rejects FD_PIPE outright.
  if (fstat(0, &st) == 0 && st.type == T_DEVICE) {
    fprintf(2, "ex1copy: copying standard input to standard output.\n");
    fprintf(2, "ex1copy: type a line and press Enter. Ctrl-d on an empty "
               "line finishes.\n");
  }

  // Console input is line-buffered by kernel/console.c: keystrokes are held
  // until Enter, and then one read() returns the whole line.  Once the
  // buffered input is consumed the next read() waits for more, and Ctrl-d at
  // an empty input position returns zero for EOF, which ends this loop.
  while ((n = read(0, buf, sizeof(buf))) > 0) {
    if (write(1, buf, n) != n) {
      fprintf(2, "ex1copy: write error\n");
      return 1;
    }
  }
  if (n < 0) {
    fprintf(2, "ex1copy: read error\n");
    return 1;
  }

  // Returning is exactly equivalent to calling exit() here: start() in
  // user/ulib.c is the real entry point, and it does `exit(main(argc, argv))`.
  // There is no EXIT_SUCCESS to return -- xv6 is freestanding (-nostdlib in
  // the Makefile) and user/user.h is the entire C library, so the value is a
  // plain int: 0 for success, non-zero for failure.
  return 0;
}
