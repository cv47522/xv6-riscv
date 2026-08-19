// ex9ls.c: list the files in a directory, by reading the directory.
//
// Based on MIT 6.1810 Lecture 1's ex9.c, linked from the schedule on the
// course web site:
// https://pdos.csail.mit.edu/6.1810/2026/lec/l-overview/ex9.c.
// user/ls.c is the full version; this one keeps only the lecture's point.
//
// ---------------------------------------------------------------------------
//  RUNNING IT  (inside xv6, not on the host)
// ---------------------------------------------------------------------------
//
//    % make qemu
//    $ ex9ls
//    .
//    ..
//    README
//    cat
//    echo
//    ...
//    $ ex9ls | grep ex
//    $ mkdir d; ex9ls d
//
//  Compare with `ls`, which prints type, inode number, and size for each
//  entry.  This program prints names only -- the extra columns come from a
//  stat() per entry, which is a second idea and not this one.
//
// ---------------------------------------------------------------------------
//  THE POINT: A DIRECTORY IS AN ORDINARY FILE
// ---------------------------------------------------------------------------
//
//  There is no opendir(), no readdir(), and no directory-listing system
//  call.  A directory is a file whose CONTENTS are a plain array of fixed
//  size records, and you list it with the same open() and read() from
//  ex1copy.c and ex2create.c.  From kernel/fs.h:
//
//    #define DIRSIZ 14
//
//    struct dirent {
//      ushort inum;                 // which inode, or 0 for a free slot
//      char name[DIRSIZ];           // NOT necessarily NUL-terminated
//    };
//
//  16 bytes per entry -- 2 for inum, 14 for the name -- and a directory of
//  n entries is exactly 16n bytes long.  So the read loop below has no
//  parsing in it at all: each read() of sizeof(de) bytes lands one whole
//  record, and the file's length says when to stop.
//
//    open(".", O_RDONLY)
//      |
//      v
//    +----------+----------+----------+----------+----------+
//    | inum  1  | inum  1  | inum  2  | inum  0  | inum  3  |
//    | "."      | ".."     | "README" | <free>   | "cat"    |
//    +----------+----------+----------+----------+----------+
//     16 bytes    16 bytes   16 bytes   16 bytes   16 bytes
//                                       ^
//                                       skipped: inum 0 means the slot was
//                                       freed by unlink() and reused later
//
//  This is what "everything is a file" buys.  Rather than a second API for
//  directories, Unix reuses the descriptor interface and the byte-stream
//  read, and the only new thing to learn is the record layout.
//
// ---------------------------------------------------------------------------
//  THREE DETAILS THE LOOP HAS TO GET RIGHT
// ---------------------------------------------------------------------------
//
//   Detail                    Why, and what happens otherwise
//   ------------------------  -------------------------------------------
//   Skip entries with         Those are free slots, not files.  unlink()
//   inum == 0                 in kernel/fs.c blanks a dirent by zeroing
//                             it rather than compacting the directory, so
//                             the array is sparse.  Print them and you get
//                             blank lines and stale names.
//
//   Do not assume name is     name[] is `char name[DIRSIZ]` marked
//   NUL-terminated            __attribute__((nonstring)): a name of
//                             exactly 14 characters fills the field with
//                             no room for a terminator.  Passing it
//                             straight to printf("%s") would run off the
//                             end into the next record.  Copy into a
//                             DIRSIZ+1 buffer and terminate it yourself.
//
//   Read whole records        read() may return fewer bytes than asked
//                             for in general.  The `== sizeof(de)` test is
//                             both the loop condition and the guard: a
//                             short read means the directory ended, and a
//                             partial record is never processed.
//
// ---------------------------------------------------------------------------
//  "." AND ".."
// ---------------------------------------------------------------------------
//
//  Not shell syntax and not special-cased by this program: they are real
//  entries, written into every directory when it is created by create() in
//  kernel/sysfile.c.  "." links to the directory itself and ".." to its
//  parent, which is how a relative path is resolved at all -- namei() in
//  kernel/fs.c just walks entries by name.
//
//  So `open(".", O_RDONLY)` opens "the process's current directory",
//  tracked as p->cwd in kernel/proc.h and changed with chdir().  Because
//  the shell forks a child for each command, chdir() had to be built INTO
//  sh rather than shipped as a program: a `cd` that ran as its own process
//  would change that child's p->cwd and exit, leaving the shell where it
//  was.  See user/sh.c, and the xv6 book's note on the same point.
//
// ---------------------------------------------------------------------------
//  WHY THE PROGRAM CANNOT WRITE THE DIRECTORY BACK
// ---------------------------------------------------------------------------
//
//  Reading a directory is allowed; writing one is not.  sys_open() in
//  kernel/sysfile.c refuses a directory opened with anything but O_RDONLY,
//  and writei() in kernel/fs.c would refuse in any case.  Only the kernel
//  edits directory contents, through create(), sys_link(), and sys_unlink().
//
//  The reason is integrity: a directory entry is a reference to an inode,
//  and the inode's nlink count has to stay in step with it.  A user program
//  writing raw bytes could produce an entry pointing at a free inode, or an
//  inode with a link count nothing agrees on -- a corrupt file system with
//  no way back.  Early Unix did allow it, and this is one of the places
//  where later designs tightened the interface.
//
// ---------------------------------------------------------------------------
//  WHERE EACH PIECE LIVES IN THIS TREE
// ---------------------------------------------------------------------------
//
//   Idea                        File
//   --------------------------  -------------------------------------------
//   the open/read/fstat stubs   user/usys.S  (load a7, then `ecall`)
//   sys_open's directory check  kernel/sysfile.c
//   struct dirent, DIRSIZ       kernel/fs.h
//   struct stat, T_DIR/T_FILE   kernel/stat.h
//   path lookup by entry        kernel/fs.c  (namei(), dirlookup())
//   creating "." and ".."       kernel/sysfile.c  (create())
//   freeing an entry            kernel/fs.c  (sys_unlink() zeroes inum)
//   the current directory       kernel/proc.h  (p->cwd), sys_chdir()
//   the full listing program    user/ls.c
//   cd built into the shell     user/sh.c
//   this exercise, written up   docs/07-exercises.md
//   the long-form notes         docs/book/ch01-operating-system-interfaces.md
//
// ---------------------------------------------------------------------------
//  THREE THINGS WORTH KEEPING
// ---------------------------------------------------------------------------
//
//  * Listing a directory needs no new system call.  open() and read() are
//    enough, because a directory is a file with a known record layout.
//  * The layout leaks two obligations onto every reader: skip inum == 0,
//    and do not treat name[] as a C string.
//  * The kernel keeps the write side to itself, because directory entries
//    and inode link counts must stay consistent.

#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"
#include "user/user.h"

// One directory's names, printed one per line.
static int
list(const char *path)
{
  struct stat st;
  struct dirent de;

  // O_RDONLY is the only mode a directory can be opened in; see above.
  int fd = open(path, O_RDONLY);
  if (fd < 0) {
    fprintf(2, "ex9ls: cannot open %s\n", path);
    return -1;
  }

  // Ask what this descriptor actually names before reading it as records.
  // Without the check, a plain file would be read 16 bytes at a time and
  // its contents printed as if they were names -- the first two bytes of
  // every chunk silently taken for an inode number.
  if (fstat(fd, &st) < 0) {
    fprintf(2, "ex9ls: cannot stat %s\n", path);
    close(fd);
    return -1;
  }
  if (st.type != T_DIR) {
    fprintf(2, "ex9ls: %s is not a directory\n", path);
    close(fd);
    return -1;
  }

  // DIRSIZ + 1: room for the longest possible name plus the terminator that
  // the on-disk record has no space for.
  char name[DIRSIZ + 1];

  // One whole record per iteration.  A short read means the directory is
  // exhausted, so this doubles as the end condition.
  while (read(fd, &de, sizeof(de)) == sizeof(de)) {
    // A free slot, left behind by unlink().  The directory is sparse, so
    // this is normal rather than an error.
    if (de.inum == 0)
      continue;

    // memmove() rather than strcpy(): de.name may occupy all DIRSIZ bytes
    // with no NUL to stop at, so the length has to come from the layout,
    // not from scanning for a terminator.
    memmove(name, de.name, DIRSIZ);
    name[DIRSIZ] = '\0';

    printf("%s\n", name);
  }

  close(fd);
  return 0;
}

int
main(int argc, char *argv[])
{
  // No argument: list the current directory.  "." is an ordinary entry that
  // every directory contains, not a shell convention -- so the kernel
  // resolves it against p->cwd with no special case.
  if (argc < 2)
    return list(".") < 0 ? 1 : 0;

  // Each remaining argument is a directory to list.  argv[0] is skipped: it
  // is the program's own name by convention, as ex4exec.c describes.
  int bad = 0;
  for (int i = 1; i < argc; i++) {
    // A heading only when there is more than one, so single-directory
    // output stays clean enough to pipe into grep.
    if (argc > 2)
      printf("%s:\n", argv[i]);
    if (list(argv[i]) < 0)
      bad = 1;
  }

  return bad;
}
