#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "kernel/fs.h"
#include "user/user.h"

static int
file_is_found(const char *str1, const char *str2)
{
  if (strcmp(str1, str2) == 0) return 1;
  return 0;
}

static int
find(const char *path, const char *name)
{
  // Directories can only be opened O_RDONLY.
  int fd = open(path, O_RDONLY);
  if (fd < 0) {
    fprintf(2, "find: cannot open %s\n", path);
    /*
     * Q: Why not close(fd)?
     * A: A failed open() allocated no descriptor, so there is nothing to
     * release. fd holds -1, and close(-1) would itself fail: argfd() in
     * kernel/sysfile.c rejects fd < 0 before touching p->ofile[].
     */
    return -1;
  }

  struct stat st;
  /*
   * Q: dirent's fullname & usage?
   * A: "directory entry", defined in kernel/fs.h. A directory's data is an
   * array of these 16-byte records, and read() on a directory fd returns
   * them verbatim:
   *
   *   Field   Bytes  Holds
   *   ------  -----  ---------------------------------------------------
   *   inum        2  inode number of the child; 0 marks an unused slot
   *   name       14  child's name in this parent; no '\0' if 14 long
   *
   * A record names one child inside one parent. It holds no path: the
   * caller supplies the parent prefix, as ls() does in user/ls.c.
   */
  struct dirent de;
  char buf[512];
  int *path_ptr;
  /*
   * Q: Why use DIRSIZ not random size?
   * A: DIRSIZ is the on-disk name width, so it is the longest name any
   * record can carry. A smaller buffer overflows on a 14-byte name; a
   * larger one wastes space and hides the rule that the copy below must
   * move exactly DIRSIZ bytes and then add the missing '\0'.
   */
  char filename[DIRSIZ + 1];


  /*
   * Q: What's fstat(fd, &st) check for?
   * A: st is uninitialized until fstat() fills it, so reading st.type
   * after a failed call branches on stack garbage. filestat() in
   * kernel/file.c returns -1 for an fd that is not an inode or device
   * (a pipe), or when copyout() to &st fails:
   *
   *   path names     fstat(fd, &st)  st.type   next step here
   *   -------------  --------------  --------  ------------------------
   *   a directory    returns 0       T_DIR     read dirent records
   *   a file         returns 0       T_FILE    reject: bytes, not slots
   *   the console    returns 0       T_DEVICE  reject: bytes, not slots
   *   (copy failed)  returns -1      garbage   report, close, return
   *
   * After a successful open() of a path the -1 row is rare, but the check
   * is what makes the st.type test below meaningful.
   */
  if (fstat(fd, &st) < 0) {
    fprintf(2, "find: cannot stat %s\n", path);
    close(fd);
    return -1;
  }

  switch (st.type) {
    case T_DEVICE:
    case T_FILE:
      file_is_found(path, name);
      break;

    case T_DIR:
      if (strlen(path) + 1 + DIRSIZ + 1 > sizeof(buf)) {
        fprintf(2, "find: path too long\n");
        break;
      }
      strcpy(buf, path); // strcpy() adds '\0'
      /*
       * Q: What if path_ptr past the end of buf?
       * A: It cannot get there: the length check above runs before any
       * write. With L = strlen(path), the highest byte touched is the
       * '\0' written after the name:
       *
       *   Byte written  Index               Why it fits in buf[512]
       *   ------------  ------------------  ---------------------------
       *   '/'           L                   the check above guarantees
       *   name bytes    L+1 .. L+DIRSIZ     L + 1 + DIRSIZ + 1 <= 512,
       *   name's '\0'   L+1+DIRSIZ          so this index is <= 511
       *
       * The table assumes char-sized steps. Pointer arithmetic scales by
       * the pointed-to size, so ++ and [DIRSIZ] on a pointer to a wider type
       * move further than one byte each and escape the bound.
       */
      path_ptr = buf + strlen(buf);
      *(path_ptr++) = '/';

      // Recursively open dirs & call file_is_found(subpath, name)
      while (read(fd, &de, sizeof(de)) == sizeof(de)) {
        /*
        * Q: What's inum? Why do we care?
        * A: The inode number: the index of the child's on-disk inode, which
        * holds its type, size, and data blocks. sys_unlink() in
        * kernel/sysfile.c zeroes a removed entry in place instead of
        * compacting the directory, so 0 marks a hole that names no file.
        * Printing it would emit an empty name; stat()ing it would fail.
        */
        if (de.inum == 0)
          continue;

        // memmove uses the record width; strcpy could scan beyond de.name.
        memmove(path_ptr, de.name, DIRSIZ);
        path_ptr[DIRSIZ] = '\0';
        // memmove(filename, de.name, DIRSIZ);
        // filename[DIRSIZ] = '\0';
        if (stat(buf, &st) < 0) {
          fprintf("find: cannot stat %s\n", buf);
          continue;
        }
        printf("%s\n", path_ptr);
      }
      break;
  }
  /*
   * Q: Aren't there many dirs opened as FDs not just one?
   * A: Each call opens exactly one, so each call closes exactly one. The
   * total open at once is the number of calls still in progress. A call
   * that must keep reading its own directory after a nested call returns
   * holds its fd across that nested call; xv6 has no lseek() to reopen
   * and resume. NOFILE in kernel/param.h caps p->ofile[] at 16 slots,
   * 3 of them taken by fds 0-2, so call depth is the resource to watch.
   */
  close(fd);
  return 0;
}

int
main(int argc, char *argv[])
{
  if (argc == 1) {
    exit(find(".", "") < 0 ? 1 : 0);
  }

  if (argc == 2) {
    exit(find(argv[1], "") < 0 ? 1 : 0);
  }

  if (argc > 3) {
    fprintf(2, "Usage: find [dir] [name]");
    exit(1);
  }

  exit(find(argv[1], argv[2]) < 0 ? 1 : 0);
}

