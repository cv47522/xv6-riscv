// ex2create.c: create a file on disk and write to it.
//
// Based on MIT 6.1810 Lecture 1's ex2.c, linked from the schedule on the
// course web site:
// https://pdos.csail.mit.edu/6.1810/2026/lec/l-overview/ex2.c.
// The host manual pages for the same calls are `man 2 open` and
// `man 2 close`.
//
// ---------------------------------------------------------------------------
//  RUNNING IT  (inside xv6, not on the host)
// ---------------------------------------------------------------------------
//
//    % make qemu
//    $ ex2create            prints the descriptor open() handed back
//    $ cat ex2.out          the six bytes are on disk
//    $ ls | grep ex2.out    one row: name, type 2 (T_FILE), inode, size 6
//
//  Run it twice.  The second run prints the *same* descriptor number and
//  leaves the same six bytes, because O_TRUNC resets the length to zero
//  before the write.  Drop O_TRUNC and the file would keep growing.
//
//  `make qemu` rebuilds fs.img from scratch, so ex2.out does not survive it.
//  `make qemu-fs` boots the existing image and keeps it.  See
//  docs/02-build-boot-and-usage.md.
//
// ---------------------------------------------------------------------------
//  THE ANSWER TO LECTURE 1's OPEN QUESTION
// ---------------------------------------------------------------------------
//
//  ex1copy ended on "how do you make a *new* file descriptor?".  open() is
//  the first of the three answers; pipe() is ex7pipe.c and dup() is used by
//  user/sh.c.  All three go through fdalloc() in kernel/sysfile.c, which
//  scans p->ofile[] from index 0 and returns the first empty slot:
//
//    static int
//    fdalloc(struct file *f)
//    {
//      for (fd = 0; fd < NOFILE; fd++)
//        if (p->ofile[fd] == 0) { p->ofile[fd] = f; return fd; }
//      return -1;
//    }
//
//  That "lowest unused" rule is not a detail.  It is the whole mechanism
//  behind shell redirection: close(1) frees slot 1, so the next open() is
//  *guaranteed* to land there.  ex6redirect.c does exactly that.
//
// ---------------------------------------------------------------------------
//  WHY THE DESCRIPTOR IS 3
// ---------------------------------------------------------------------------
//
//  Nothing in this program chooses 3.  It falls out of who was already in
//  the table when main() started:
//
//    slot  holds                       put there by
//    ----  --------------------------  ------------------------------------
//     0    the console, for reading    user/init.c open()s it, sh inherits
//     1    the console, for writing    dup(0) in user/init.c
//     2    the console, for writing    dup(0) in user/init.c
//     3    <empty>  <-- fdalloc()      this program's open()
//
//  A descriptor is a per-process index, not a global name.  Two processes
//  can both hold descriptor 3 pointing at completely different files, and
//  neither can see the other's table -- it is p->ofile[] in kernel/proc.h,
//  private to one struct proc.  This is why "FD 3" is meaningless on its
//  own and only means something together with a process.
//
// ---------------------------------------------------------------------------
//  open(path, flags)
// ---------------------------------------------------------------------------
//
//   Flag        Value   Effect                     Defined in
//   ----------  ------  -------------------------  ----------------------
//   O_RDONLY    0x000   read only (the default)    kernel/fcntl.h
//   O_WRONLY    0x001   write only
//   O_RDWR      0x002   both
//   O_CREATE    0x200   make it if absent          handled by create()
//   O_TRUNC     0x400   reset an existing file
//                       to length zero
//
//   Return   Means
//   -------  ----------------------------------------------------------
//    >= 0    the new descriptor, always the lowest free slot
//      -1    failed: no such file without O_CREATE, a directory opened
//            for writing, the process table slot full, or out of inodes
//
//  Note what is *missing* compared to Linux: there is no third `mode`
//  argument.  xv6 has no users, no groups, and no permission bits, so
//  there is nothing to pass.  sys_open() in kernel/sysfile.c takes exactly
//  two arguments.  A host `open("f", O_CREAT|O_WRONLY, 0644)` has no xv6
//  equivalent, and O_CREATE is spelled without the missing E on purpose.
//
// ---------------------------------------------------------------------------
//  WHAT open() DOES INSIDE THE KERNEL
// ---------------------------------------------------------------------------
//
//    open("ex2.out", O_WRONLY|O_CREATE|O_TRUNC)
//      |
//      v
//    sys_open()        kernel/sysfile.c   argstr() copies the path in
//      |
//      +-- create()    kernel/sysfile.c   O_CREATE and absent: allocate an
//      |                                  inode, link it into the directory
//      +-- namei()     kernel/fs.c        otherwise: walk the path to it
//      |
//      +-- filealloc() kernel/file.c      one struct file, refcount 1
//      +-- fdalloc()   kernel/sysfile.c   lowest free p->ofile[] slot
//      |
//      v
//    the small int this program prints
//
//  Two levels of indirection, and they matter later: the descriptor points
//  at a struct file (which holds the offset), and the struct file points at
//  an inode (which holds the data).  fork() copies the first level and
//  shares the second, which is why a parent and child writing the same
//  descriptor do not overwrite each other.  See ex5forkexec.c.
//
// ---------------------------------------------------------------------------
//  ERROR HANDLING, AND WHY THIS FILE DIFFERS FROM THE LECTURE'S
// ---------------------------------------------------------------------------
//
//  The lecture slide says, of its own examples: "these examples ignore
//  errors -- don't be this sloppy!"  The official ex2.c does ignore them.
//  This version does not, and the difference is worth reading:
//
//    Ignored                    What you see when it fails
//    -------------------------  ------------------------------------------
//    open() returning -1        write(-1, ...) silently fails, the program
//                               reports success, and `cat ex2.out` says
//                               "cat: cannot open ex2.out" -- three steps
//                               from the actual fault
//    write() returning < 6      a short or empty file, no message at all
//
//  Neither failure announces itself.  That is the argument for checking:
//  the cost is four lines, and the alternative is debugging a symptom that
//  appears in a different program.
//
// ---------------------------------------------------------------------------
//  WHERE EACH PIECE LIVES IN THIS TREE
// ---------------------------------------------------------------------------
//
//   Idea                        File
//   --------------------------  -------------------------------------------
//   the open/write/close stubs  user/usys.S  (load a7, then `ecall`)
//   sys_open, sys_close         kernel/sysfile.c
//   create() and namei()        kernel/sysfile.c, kernel/fs.c
//   the O_* flag values         kernel/fcntl.h
//   per-process FD table        kernel/proc.h  (p->ofile[NOFILE])
//   one open file               kernel/file.c  (struct file)
//   on-disk inode and dirent    kernel/fs.h
//   this exercise, written up   docs/07-exercises.md
//   the full syscall list       docs/05-syscall-reference.md
//
// ---------------------------------------------------------------------------
//  THREE THINGS WORTH KEEPING
// ---------------------------------------------------------------------------
//
//  * open() is how a descriptor comes into existence, and fdalloc()'s
//    lowest-unused rule is what makes the number predictable enough to
//    build redirection on.
//  * The descriptor namespace is per process.  Descriptor 3 here and
//    descriptor 3 in the shell that launched us are unrelated integers
//    indexing two different p->ofile[] arrays.
//  * Checking return values is not ceremony.  Both calls below can fail,
//    and neither failure is visible at the point where it happens.

#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "user/user.h"

int
main(void)
{
  // O_WRONLY: we only write.  O_CREATE: make the file if it is not there.
  // O_TRUNC: if it *is* there, reset it to length zero first, so a second
  // run leaves six bytes rather than twelve.  There is no mode argument to
  // pass -- xv6 has no permission bits.
  int fd = open("ex2.out", O_WRONLY | O_CREATE | O_TRUNC);
  if (fd < 0) {
    // fd 2 rather than 1: a diagnostic is about the run, not part of its
    // output, and it must survive `ex2create > log`.  fprintf() rather than
    // printf() because printf() in user/printf.c hardwires fd 1.
    fprintf(2, "ex2create: cannot create ex2.out\n");
    return 1;
  }

  // The number below is the whole observable point of the exercise: it is 3
  // on a normal run, because fdalloc() returned the lowest free slot and
  // 0, 1, and 2 were taken before main() started.
  printf("ex2create: open() returned fd %d\n", fd);

  // Six bytes, not seven: strlen("Hello\n") is 6 and the trailing NUL that
  // C puts in the string literal is a C convention, not a file-format one.
  // Unix files have an explicit length, so nothing needs to mark the end --
  // writing the NUL would put a stray 0x00 byte in the file and make
  // `ls` report a size of 7.
  int n = write(fd, "Hello\n", 6);
  if (n != 6) {
    fprintf(2, "ex2create: short write (%d of 6 bytes)\n", n);
    close(fd);
    return 1;
  }

  // Closing is not strictly required -- exit() releases every descriptor
  // through fileclose() in kernel/file.c.  Do it anyway: it is the habit
  // that matters once a program opens files in a loop, where NOFILE (16)
  // is reached quickly, and it is the point at which a failure is still
  // attributable to this program rather than to whatever runs next.
  close(fd);

  return 0;
}
