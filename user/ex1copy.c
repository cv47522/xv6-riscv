// ex1copy.c: copy standard input to standard output until EOF.
//
// Based on MIT 6.1810 Lecture 1's ex1.c:
// https://pdos.csail.mit.edu/6.1810/2026/lec/l-overview/ex1.c.
// Host references: man 2 read; man 2 write.
//
// ---------------------------------------------------------------------------
//  PREREQUISITES
// ---------------------------------------------------------------------------
//
//   Kind  Read first
//   ----  ------------------------------------------------------------------
//   note  ../operating-system/The_Process_Abstraction.md
//   code  ../operating-system/codes/src/main/virtualization/cpu-process-api/
//           dup_family/dup_fd_abstraction.c
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
//  REDIRECTION IS WIRING; read() MOVES BYTES
// ---------------------------------------------------------------------------
//
//   Typed       user/sh.c does                               Ends up at
//   ----------  -------------------------------------------  ------------
//   < README    close(0); open("README", O_RDONLY)            fd 0, input
//   > README    close(1); open("README",                      fd 1, output
//                 O_WRONLY|O_CREATE|O_TRUNC)
//
//  user/sh.c performs this after fork() and before exec(). open() takes the
//  lowest free slot, so the new file becomes fd 0 or 1. `> README` leaves
//  input on the keyboard and truncates README before this program starts;
//  it does not read the file. `< README > copy` safely combines both sides.
//
//  `<` runs once in the shell and chooses what fd 0 names. This program then
//  calls read() repeatedly to move bytes from that descriptor. The same loop
//  therefore handles a keyboard, file, or pipe. See ex2create.c,
//  ex6redirect.c, and ../operating-system/codes/src/main/virtualization/
//  cpu-process-api/redirect_family/redirect_vs_read.c.
//
// ---------------------------------------------------------------------------
//  THE DATA PATH
// ---------------------------------------------------------------------------
//
//    fd 0  <-- keyboard, pipe, or file (similar to stdin in Linux)
//      |
//      |   read(0, buf, sizeof(buf))        ---- ecall ---> kernel
//      |   returns n: >0 bytes, 0 at EOF, -1 on error
//      v
//    char buf[64]    untyped bytes; this program never looks inside
//      |
//      |   write(1, buf, n)                 ---- ecall ---> kernel
//      |   returns n, or -1 on error
//      v
//    fd 1  --> console, pipe, or file (similar to stdout in Linux)
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
//  An fd indexes p->ofile[NOFILE] in kernel/proc.h; xv6 allows 16 per process.
//
//   FD  Unix convention   Opened by             Used here for
//   --  ----------------  --------------------  ---------------------------
//    0  standard input    user/sh.c, pre-exec   everything we read
//    1  standard output   user/sh.c, pre-exec   everything we write
//    2  standard error    user/sh.c, pre-exec   diagnostics and the hint
//
//  fd 2 keeps diagnostics separate from copied output.
//
// ---------------------------------------------------------------------------
//  WHERE EACH PIECE LIVES IN THIS TREE
// ---------------------------------------------------------------------------
//
//   Idea                        File
//   --------------------------  -------------------------------------------
//   read/write syscall stubs    user/usys.S
//   system call dispatch        kernel/syscall.c
//   sys_read and sys_write      kernel/sysfile.c
//   per-process FD table        kernel/proc.h  (p->ofile[NOFILE])
//   open-file objects           kernel/file.c
//   console line buffering      kernel/console.c
//   descriptor plumbing         user/sh.c
//   exercise details            docs/07-exercises.md
//   long-form notes             docs/book/ch01-operating-system-interfaces.md
//
// ---------------------------------------------------------------------------
//  KEY POINTS
// ---------------------------------------------------------------------------
//
//  * C exposes addresses and maps predictably to machine code.
//  * read() and write() trap so the kernel validates unprivileged requests.
//  * Unix I/O moves untyped bytes; applications supply their meaning.

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(void)
{
  struct stat st;
  char buf[64];
  int n;

  // fstat() on fd 0 separates the three input cases:
  //
  //   fd 0 is        fstat(0, &st)   st.type     hint?
  //   -------------  --------------  ----------  -----
  //   the console    returns 0       T_DEVICE    yes
  //   a file         returns 0       T_FILE      no
  //   a pipe         returns -1      --          no
  //
  // The hint goes to fd 2, so it cannot contaminate redirected output.
  if (fstat(0, &st) == 0 && st.type == T_DEVICE) {
    fprintf(2, "ex1copy: copying standard input to standard output.\n");
    fprintf(2, "ex1copy: type a line and press Enter. Ctrl-d on an empty "
               "line finishes.\n");
  }

  // kernel/console.c ends a line on Enter or Ctrl-d. consoleread() omits the
  // Ctrl-d byte: at an empty position read() returns 0; after partial input,
  // one read returns the bytes and the next returns 0.
  while ((n = read(0, buf, sizeof(buf))) > 0) {
    // A short write would silently lose part of the byte stream.
    if (write(1, buf, n) != n) {
      fprintf(2, "ex1copy: write error\n");
      return 1;
    }
  }
  if (n < 0) {
    fprintf(2, "ex1copy: read error\n");
    return 1;
  }

  // user/ulib.c calls exit(main(...)), so return 0 equals exit(0). xv6 is
  // freestanding; user/user.h has no host EXIT_SUCCESS.
  return 0;
}
