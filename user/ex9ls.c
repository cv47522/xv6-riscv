// ex9ls.c: list directory entries by reading the directory.
//
// Based on MIT 6.1810 Lecture 1's ex9.c:
// https://pdos.csail.mit.edu/6.1810/2026/lec/l-overview/ex9.c.
// Host contrast: man 3 opendir; man 3 readdir; man 2 getdents.
//
// ---------------------------------------------------------------------------
//  PREREQUISITES
// ---------------------------------------------------------------------------
//
//  <OS> means the sibling repository at ../operating-system.
//
//   Kind  Read first
//   ----  ------------------------------------------------------------------
//   note  <OS>/The_Process_Abstraction.md  (FD terminology)
//   code  <OS>/codes/src/main/virtualization/cpu-process-api/
//           pipe_family/pipe_three_closes.c  (opendir/readdir example)
//
// ---------------------------------------------------------------------------
//  RUNNING IT  (inside xv6, not on the host)
// ---------------------------------------------------------------------------
//
//    % make qemu
//    $ ex9ls               list the current directory
//    $ ex9ls . /           list two named directories with headings
//    $ ex9ls README        reject a path that is not a directory
//
// ---------------------------------------------------------------------------
//  A DIRECTORY IS A FILE OF FIXED-SIZE RECORDS
// ---------------------------------------------------------------------------
//
//  xv6 needs no opendir() or readdir(). open() and read() expose the directory
//  as an array of records defined in kernel/fs.h:
//
//    struct dirent {
//      ushort inum;          inode number; 0 means an unused slot
//      char name[DIRSIZ];    exactly 14 bytes; may have no trailing NUL
//    };
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
//   Rule                       Why
//   -------------------------  ---------------------------------------------
//   read sizeof(struct dirent) consume exactly one on-disk record
//   skip inum == 0             unlink leaves reusable holes in the array
//   copy exactly DIRSIZ bytes  a full-length name has no NUL terminator
//   append a NUL in name[]     printf("%s") requires a C string
//
//  "." and ".." are real directory entries. With no argument, "." resolves
//  against p->cwd in kernel/proc.h, the process's current directory.
//
// ---------------------------------------------------------------------------
//  WHERE EACH PIECE LIVES IN THIS TREE
// ---------------------------------------------------------------------------
//
//   Idea                        File
//   --------------------------  -------------------------------------------
//   struct dirent and DIRSIZ    kernel/fs.h
//   struct stat and T_DIR       kernel/stat.h
//   directory reads             kernel/fs.c  (readi())
//   path lookup                 kernel/fs.c  (namei(), dirlookup())
//   "." and ".." creation       kernel/sysfile.c  (create())
//   full ls implementation      user/ls.c
//   exercise details            docs/07-exercises.md
//
// ---------------------------------------------------------------------------
//  THREE THINGS WORTH KEEPING
// ---------------------------------------------------------------------------
//
//  * Listing needs no new xv6 syscall: directories use open(), read(), and
//    known record layout.
//  * On-disk names are fixed-width byte fields, not automatically C strings.
//  * The kernel owns directory writes so dirents and inode links stay valid.

#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"
#include "user/user.h"

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

  // Do not interpret an ordinary file as an array of directory records.
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

  // One extra byte holds the NUL missing from a full DIRSIZ on-disk name.
  char name[DIRSIZ + 1];

  while (read(fd, &de, sizeof(de)) == sizeof(de)) {
    if (de.inum == 0) // A zero inode marks a free directory slot.
      continue;

    // memmove uses the record width; strcpy could scan beyond de.name.
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
  // "." is a real entry naming this process's current directory.
  if (argc < 2)
    return list(".") < 0 ? 1 : 0;

  int bad = 0;
  for (int i = 1; i < argc; i++) {
    if (argc > 2)
      printf("%s:\n", argv[i]);
    if (list(argv[i]) < 0)
      bad = 1;
  }

  return bad;
}
