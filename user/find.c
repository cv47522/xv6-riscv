/*
 * find.c: print every path under a directory whose last name matches.
 *
 * MIT 6.1810 util lab, exercise "find". The note that derives it, with
 * the grader's assertions and traps, is docs/labs/04-util-find.md.
 *
 * ---------------------------------------------------------------------------
 *  RUNNING IT  (inside xv6, not on the host)
 * ---------------------------------------------------------------------------
 *
 *    $ echo > b; mkdir a; echo > a/b; mkdir a/aa; echo > a/aa/b
 *    $ find . b
 *    ./b
 *    ./a/b
 *    ./a/aa/b
 *
 * ---------------------------------------------------------------------------
 *  ONE CALL OF find(path, name)
 * ---------------------------------------------------------------------------
 *
 *   open(path) + fstat()          must be a T_DIR, else report and return
 *      |
 *      v
 *   child_path = path + "/"       child_name points just past the '/'
 *      |
 *      v
 *   read() one 16-byte dirent  <-------------------------------+
 *      |   (no more records: close(fd), return)                |
 *      v                                                       |
 *   copy de.name to child_name    child_path is "path/child"   |
 *      |                                                       |
 *      v                                                       |
 *   apply the entry table below, maybe find(child_path)  ------+
 *
 * The record holds only the child's name, so child_path is the one place
 * the full path exists. Each record overwrites only the bytes from
 * child_name on; the prefix written once per directory stays valid for
 * every child.
 *
 *   Entry               Print it?               Descend into it?
 *   ------------------  ----------------------  ------------------------
 *   inum == 0           no: unused slot         no
 *   "." or ".."         no                      no: they lead back (loop)
 *   name matches        yes, print child_path   if it is a T_DIR
 *   name differs        no                      if it is a T_DIR
 *
 * The match test compares child_name with the target, never child_path:
 * "./a/b" is not "b". A matching directory is printed and still searched,
 * which is what the grader's directory-match case checks.
 *
 * ---------------------------------------------------------------------------
 *  RESOURCES THE RECURSION HOLDS
 * ---------------------------------------------------------------------------
 *
 * A call keeps its fd and its child_path until every nested call returns,
 * because it still has records left to read and xv6 has no lseek().
 *
 *   Per active level   Limit                          Where
 *   -----------------  -----------------------------  --------------------
 *   1 fd               NOFILE 16, minus fds 0-2       kernel/param.h
 *   child_path[]       user stack, USERSTACK pages    kernel/param.h
 *   (path length)      open()/stat() need < MAXPATH   argstr() in sysfile.c
 *
 * Sizing child_path at MAXPATH, not ls.c's 512, keeps each frame small and
 * matches the longest path the kernel would accept anyway.
 *
 * Figure: docs/labs/fig/util-find-recursion-state.svg (editable source:
 * the .excalidraw beside it) shows the stack frames, each child_path and
 * child_name, and p->ofile[] at the deepest point of "find . b". The
 * note's "Solution walkthrough" section pairs it with a control-flow
 * diagram of one call.
 */

#include "kernel/types.h"
#include "kernel/param.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "kernel/fs.h"
#include "user/user.h"

static int
find(const char *path, const char *name)
{
  // Directories can only be opened O_RDONLY.
  int fd = open(path, O_RDONLY);
  if (fd < 0) {
    fprintf(2, "find: cannot open %s\n", path);
    /*
     * Q: Why is there no close(fd) when open() fails?
     * A: A failed open() allocated no descriptor, so there is nothing to
     * release. fd holds -1, and close(-1) would itself fail: argfd() in
     * kernel/sysfile.c rejects fd < 0 before touching p->ofile[].
     */
    return -1;
  }

  struct stat st;
  /*
   * Q: What does dirent stand for, and what does one record hold?
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
  /*
   * ls.c's buf and p are one path and a cursor into it:
   *
   *   Name        Was  Holds
   *   ----------  ---  -----------------------------------------------
   *   child_path  buf  "path/" + the current record's name + '\0',
   *                    the string passed to printf(), stat(), find()
   *   child_name  p    &child_path[strlen(path) + 1]: where that name
   *                    starts, and the string strcmp() tests
   *
   * child_name owns no memory; it aliases the tail of child_path. Writing
   * through it rewrites the path, which is why one memmove() per record
   * updates both strings at once.
   */
  char child_path[MAXPATH];
  char *child_name;
  int return_status = 0;

  /*
   * Q: What does checking fstat()'s return value protect against?
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
   * Only the starting path can reach the reject rows: the loop below
   * calls find() for T_DIR children only.
   */
  if (fstat(fd, &st) < 0) {
    fprintf(2, "find: cannot stat path %s (FD %d)\n", path, fd);
    close(fd);
    return -1;
  }
  /*
   * Q: Why reject a T_FILE start path instead of matching its name?
   * A: It could be done, and GNU find does test its starting points. Two
   * reasons it is a usage error here instead:
   *
   *   1. The contract is "find <dir> <name>"; no grader test passes a
   *      file, and the handout's examples all start at a directory.
   *   2. A fair test needs the start path's last component: "a/b" must
   *      match "b", and "./a/" must match "a". That is a basename parser
   *      with trailing-'/' cases, extra code for an untested input.
   *
   * Children never need this: a child's name arrives on its own in the
   * record, so the loop tests it before deciding whether to descend.
   */
  if (st.type != T_DIR) {
    fprintf(2, "find: %s is not a directory\n", path);
    close(fd);
    return -1;
  }

  /*
   * Q: Why budget exactly DIRSIZ bytes for the name?
   * A: DIRSIZ is the on-disk name width, so it is the longest name any
   * record can carry. Budgeting a full DIRSIZ here means no child name
   * can overflow child_path, whatever its length.
   */
  if (strlen(path) + 1 + DIRSIZ + 1 > sizeof(child_path)) {
    fprintf(2, "find: path too long: %s\n", path);
    close(fd);
    return -1;
  }
  strcpy(child_path, path); // strcpy() adds '\0'
  /*
   * Q: Can writes through child_name run past the end of child_path?
   * A: It cannot get there: the length check above runs before any
   * write. With L = strlen(path), the highest byte touched is the
   * '\0' written after the name:
   *
   *   Byte written  Index               Why it fits in [MAXPATH]
   *   ------------  ------------------  ---------------------------
   *   '/'           L                   the check above guarantees
   *   name bytes    L+1 .. L+DIRSIZ     L + 1 + DIRSIZ + 1 <= 128,
   *   name's '\0'   L+1+DIRSIZ          so this index is <= 127
   *
   * The table assumes char-sized steps. Pointer arithmetic scales by
   * the pointed-to size, so ++ and [DIRSIZ] on a pointer to a wider type
   * move further than one byte each and escape the bound.
   */
  child_name = child_path + strlen(child_path);
  *child_name++ = '/';

  /*
   * Q: Why read one dirent per call, and how does the loop stop at EOF?
   * A: A directory holds one record per entry, and its size is not known
   * up front, so the loop asks for one 16-byte record at a time:
   *
   *   read() returns  Means                      Loop
   *   --------------  -------------------------  --------------------
   *   16              one whole dirent arrived   handle it, read next
   *   0               EOF: past the last record  stops
   *   -1              the read failed            stops
   *
   * readi() in kernel/fs.c stops at the inode's size, and a directory's
   * size is a multiple of 16, so a short positive count does not occur.
   * Reading many records per call would save system calls but needs a
   * bigger buffer in every recursion frame; ls.c makes the same choice.
   */
  while (read(fd, &de, sizeof(de)) == sizeof(de)) {
    /*
     * Q: What is inum, and why skip records whose inum is 0?
     * A: The inode number: the index of the child's on-disk inode, which
     * holds its type, size, and data blocks. sys_unlink() in
     * kernel/sysfile.c zeroes a removed entry in place instead of
     * compacting the directory, so 0 marks a hole that names no file.
     * Printing it would emit an empty name; stat()ing it would fail.
     */
    if (de.inum == 0)
      continue;

    /*
     * Q: Could memcpy() replace memmove(), and does either zero-pad a
     * name shorter than DIRSIZ?
     * A: memcpy() would work: de.name and child_path do not overlap.
     * In user/ulib.c, memcpy() even delegates to memmove(); using
     * memmove() here follows user/ls.c.
     *
     * Both copy exactly DIRSIZ bytes, including any zeros already in the
     * record; neither adds padding nor stops at '\0'. Short names are
     * zero-padded by dirlink()'s strncpy() in kernel/fs.c, not this copy.
     * A full-width name has no '\0', so strlen(de.name) or strcpy() could
     * read past the field. The explicit terminator below makes child_name
     * a valid C string for names of either length.
     */
    memmove(child_name, de.name, DIRSIZ);
    child_name[DIRSIZ] = '\0';

    // Skipped before the match test too, so "find . .." prints nothing.
    if (strcmp(child_name, ".") == 0 || strcmp(child_name, "..") == 0)
      continue;

    if (strcmp(child_name, name) == 0)
      printf("%s\n", child_path);

    /*
     * Q: Why stat() each child when fstat() already ran? How do the two
     * calls differ?
     * A: The fstat() above described this directory, through fd. The
     * child's type is still unknown, and the record carries no type: an
     * inode number and a name only.
     *
     *   Call           Takes           Describes        Opens an fd?
     *   -------------  --------------  ---------------  ----------------
     *   fstat(fd, st)  an open fd      what fd names    no, needs one
     *   stat(path, st) a path string   what path names  briefly, inside
     *
     * stat() in user/ulib.c is open() + fstat() + close(), so it costs
     * one extra fd slot for an instant: frame 3's red slot in the figure.
     */
    if (stat(child_path, &st) < 0) {
      fprintf(2, "find: cannot stat path %s\n", child_path);
      return_status = -1;
      continue;
    }
    /*
     * Q: How many directory fds are open at once during recursion?
     * A: One per call still in progress. This call still holds fd while
     * the nested find() opens its own, and each call closes exactly the
     * one it opened before returning. The nested call copies child_path
     * into its own frame, so the prefix here is intact when it returns.
     */
    if (st.type == T_DIR && find(child_path, name) < 0)
      return_status = -1;
  }

  close(fd);
  return return_status;
}

int
main(int argc, char *argv[])
{
  /*
   * Q: Why require exactly two operands, unlike Ubuntu's `find` and `find .`?
   * A: GNU find with no -name test prints every path; it does not search
   * for "". The old one- and two-argument branches called find(dir, ""),
   * and no entry has an empty name, so they would print nothing and exit
   * 0, which looks like success. Matching GNU would need a separate "no
   * filter" mode, and lab 05 (docs/labs/05-util-exec.md) already extends
   * these arguments with -exec, so the usage stays the handout's exact
   * "find <dir> <name>" until then.
   */
  if (argc != 3) {
    fprintf(2, "Usage: find <dir> <name>\n");
    exit(1);
  }
  // A failure in one subtree is reported but does not stop the others.
  exit(find(argv[1], argv[2]) < 0 ? 1 : 0);
}
