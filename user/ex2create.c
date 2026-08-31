// ex2create.c: create a file on disk and write six bytes to it.
//
// Based on MIT 6.1810 Lecture 1's ex2.c:
// https://pdos.csail.mit.edu/6.1810/2026/lec/l-overview/ex2.c.
// Host references: man 2 open; man 2 write; man 2 close.
//
// ---------------------------------------------------------------------------
//  PREREQUISITES
// ---------------------------------------------------------------------------
//
//  <OS> means the sibling repository at ../operating-system.
//
//   Kind  Read first
//   ----  ------------------------------------------------------------------
//   note  <OS>/Introduction_to_Operating_Systems.md  (Persistence)
//   code  <OS>/codes/src/main/intro/persistence.c
//
// ---------------------------------------------------------------------------
//  RUNNING IT  (inside xv6, not on the host)
// ---------------------------------------------------------------------------
//
//    % make qemu
//    $ ex2create            open() normally returns fd 3
//    $ cat ex2.out          prints the six bytes written below
//    $ ex2create            O_TRUNC keeps the file at six bytes
//
//  make qemu rebuilds fs.img; make qemu-fs reuses the existing image.
//  See docs/02-build-boot-and-usage.md.
//
// ---------------------------------------------------------------------------
//  HOW open() MAKES A DESCRIPTOR
// ---------------------------------------------------------------------------
//
//  fdalloc() in kernel/sysfile.c returns the lowest empty p->ofile[] slot.
//
//   Slot  Holds before main()                 Set up by
//   ----  ----------------------------------  -------------------------------
//      0  console input                       user/init.c
//      1  console output                      dup(0) in user/init.c
//      2  console diagnostics                 dup(0) in user/init.c
//      3  empty: this open() takes it          fdalloc()
//
//  The number is local to this process. Another process can use fd 3 for a
//  different file because it has a different p->ofile[] table.
//
// ---------------------------------------------------------------------------
//  open(path, flags)
// ---------------------------------------------------------------------------
//
//   Flag        Effect
//   ----------  ------------------------------------------------------------
//   O_RDONLY    read only; value 0 and therefore the default
//   O_WRONLY    write only
//   O_RDWR      read and write
//   O_CREATE    create the file if it is absent
//   O_TRUNC     reset an existing file to length zero
//
//   Return   Means
//   -------  ----------------------------------------------------------
//    >= 0    the new descriptor, always the lowest free slot
//      -1    failed: no such file without O_CREATE, a directory opened
//            for writing, the process table slot full, or out of inodes
//
//  xv6 open() has no mode argument: xv6 has no users, groups, or permission
//  bits. O_CREATE is also spelled differently from host Unix O_CREAT.
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
// ---------------------------------------------------------------------------
//  WHERE EACH PIECE LIVES IN THIS TREE
// ---------------------------------------------------------------------------
//
//   Idea                        File
//   --------------------------  -------------------------------------------
//   open/write/close stubs      user/usys.S
//   sys_open and fdalloc        kernel/sysfile.c
//   pathname lookup             kernel/fs.c
//   open-file objects           kernel/file.c
//   O_* flags                   kernel/fcntl.h
//   per-process FD table        kernel/proc.h  (p->ofile[NOFILE])
//   exercise details            docs/07-exercises.md
//
// ---------------------------------------------------------------------------
//  THREE THINGS WORTH KEEPING
// ---------------------------------------------------------------------------
//
//  * open() makes a per-process descriptor, not a global file name.
//  * The lowest-free rule yields fd 3 here and fd 1 after close(1), which is
//    the mechanism used by ex6redirect.c.
//  * Check every return. An ignored open() or short write moves the visible
//    symptom away from the call that failed.

#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "user/user.h"

int
main(void)
{
  // Create if absent; truncate first so repeated runs do not append.
  int fd = open("ex2.out", O_WRONLY | O_CREATE | O_TRUNC); /* 0,1,2 are taken, so this is 3 */
  if (fd < 0) {
    fprintf(2, "ex2create: cannot create ex2.out\n");
    return 1;
  }

  printf("ex2create: open() returned fd %d\n", fd);

  // write() reports bytes transferred, which may be fewer than requested.
  int n = write(fd, "Hello\n", 6);
  if (n != 6) {
    fprintf(2, "ex2create: short write (%d of 6 bytes)\n", n);
    close(fd);
    return 1;
  }

  close(fd);
  return 0;
}
