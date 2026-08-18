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
//  Type each of those out.  user/sh.c has no line editing and no command
//  history, so the arrow keys never arrive as keys: the terminal sends the
//  raw bytes of an ANSI escape sequence (Up is ESC [ A), and sh folds them
//  into the word being typed.  Recalling `echo hi` with Up and appending
//  ` | ex1copy` therefore asks sh to exec "\033[Aecho", which fails -- and
//  because the terminal *acts* on the escape rather than printing it, the
//  diagnostic renders as `exec echo failed`, naming a program that `ls`
//  shows is plainly there.  Ctrl-u erases the line and does work; it is
//  handled by consoleintr() in kernel/console.c, alongside Ctrl-h.
//
//  One more console edge: Ctrl-d at the `$` prompt is EOF for sh itself, so
//  an extra one after this program exits ends the shell, and init starts a
//  fresh one -- the reason a stray `init: starting sh` appears mid-session.
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
//     -1      the call failed             report on fd 2
//                                         (similar to stderr in Linux) and
//                                         return 1
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
//   this exercise, written up   docs/07-exercises.md
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
  //
  // Why descriptor 0 and not 1?  The hint answers one question -- is a human
  // about to type at me? -- and that is a property of the input side alone.
  // The two sides come apart in both directions: `ex1copy > out` leaves fd 1
  // on a file while somebody still waits at the keyboard, and
  // `echo hi | ex1copy` leaves fd 1 on the console with nobody typing.
  // fstat() takes a descriptor rather than a path, so it reports on whatever
  // sh wired to 0 before exec'ing us, which is exactly the thing in doubt.
  //
  // Why T_DEVICE means "the console" here, with no display case to add:
  // T_DEVICE is the inode type mknod() stamps (sys_mknod() in
  // kernel/sysfile.c), and the only mknod() call in this whole tree is
  // `mknod("console", CONSOLE, 0)` in user/init.c -- so `console` is the only
  // device file that exists.  Keyboard and display are not two devices
  // either: one inode with major number CONSOLE (1) covers both directions,
  // because consoleinit() registers devsw[CONSOLE].read = consoleread and
  // devsw[CONSOLE].write = consolewrite over the same UART.  NDEV (10) is
  // only the size of devsw[]; nothing else ever claims a major number.  On
  // Linux this same test would pass for every character device -- /dev/null,
  // a disk, any tty -- which is why portable filters ask isatty() instead.
  if (fstat(0, &st) == 0 && st.type == T_DEVICE) {
    // fd 2 is better read as the out-of-band descriptor than the error one:
    // it is what stays pointed at the terminal when `>` moves fd 1 elsewhere.
    // Anything *about* the run rather than *part* of it belongs there --
    // errors, progress, prompts, and this hint.  The filter rule forces it:
    // `ex1copy > out` and `ex1copy | wc` must see the input bytes and nothing
    // else, and sh redirects only the descriptor you name.
    //
    // fprintf() rather than printf() because printf() in user/printf.c is
    // vprintf(1, ...) with fd 1 hardwired, while fprintf() takes the
    // descriptor.  xv6 has no FILE, no stderr stream, and no fdopen() --
    // user/user.h declares exactly these two functions -- so fprintf(2, ...)
    // is the only way to reach fd 2.
    fprintf(2, "ex1copy: copying standard input to standard output.\n");
    fprintf(2, "ex1copy: type a line and press Enter. Ctrl-d on an empty "
               "line finishes.\n");
  }

  // Console input is line-buffered by kernel/console.c: keystrokes are held
  // until Enter, and then one read() returns the whole line.  Once the
  // buffered input is consumed the next read() waits for more, and Ctrl-d at
  // an empty input position returns zero for EOF, which ends this loop.
  //
  // Nothing below implements Ctrl-d, and this program never names it: read()
  // returns 0 and the `> 0` test does the rest.  Two functions in
  // kernel/console.c produce that zero.
  //
  //   consoleintr()  accepts C('D') -- the macro is ((x) - '@'), so byte 0x04
  //                  -- as a line terminator alongside '\n', advancing cons.w
  //                  and waking a sleeping reader though no newline arrived.
  //   consoleread()  pulls that byte back out, breaks without copying it, and
  //                  returns target - n.  At an empty position nothing has
  //                  been copied yet, n is still target, and the result is 0.
  //
  // That is also the whole content of "on an empty line": type `abc` and then
  // Ctrl-d, and consoleread() takes its `if (n < target) cons.r--` branch,
  // pushing the Ctrl-d back into the buffer so this read returns three bytes
  // and only the *next* one returns 0.  A partial line needs two presses.
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
