/*
 * find.c: print paths under a directory whose final name matches.
 *
 * MIT 6.1810 util lab, "find". Derivation, grader checks, and traps:
 * docs/labs/04-util-find.md.
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
 *   child_path = path + "/"       child_name_ptr points after '/'
 *      |
 *      v
 *   read() one 16-byte dirent  <-----------------------------+
 *      |   (otherwise: close(fd), return)                    |
 *      v                                                     |
 *   copy de.name; add '\0'       child_path is "path/child"  |
 *      |                                                     |
 *      v                                                     |
 *   apply table; maybe find(child_path, name)  --------------+
 *
 *   Entry               Print it?               Descend into it?
 *   ------------------  ----------------------  ------------------------
 *   inum == 0           no: unused slot         no
 *   "." or ".."         no                      no: they lead back (loop)
 *   name matches        yes, print child_path   if it is a T_DIR
 *   name differs        no                      if it is a T_DIR
 *
 * Match child_name_ptr, print child_path: "./a/b" is not "b". Matching
 * directories are still searched, as the grader's directory-match case
 * requires.
 *
 * ---------------------------------------------------------------------------
 *  RESOURCES THE RECURSION HOLDS
 * ---------------------------------------------------------------------------
 *
 * Each call keeps fd and child_path across nested calls to resume reading
 * its directory afterward; xv6 has no lseek() to reopen and resume.
 *
 *   Per active level   Limit                          Where
 *   -----------------  -----------------------------  --------------------
 *   1 fd               NOFILE 16, minus fds 0-2       kernel/param.h
 *   child_path[]       user stack, USERSTACK pages    kernel/param.h
 *   path length        open()/stat() need < MAXPATH    kernel/sysfile.c
 *
 * MAXPATH, rather than user/ls.c's 512, saves stack space without losing
 * any kernel-accepted path length; sys_open() uses argstr() to enforce it.
 *
 * docs/labs/fig/util-find-recursion-state.svg shows frames, path buffers,
 * name pointers, and p->ofile[] at the deepest point of "find . b". Its
 * editable .excalidraw is beside it; the note's "Solution walkthrough"
 * adds a control-flow diagram of one call.
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
    /*
     * Q: When would open() fail here?
     * A: sys_open() in kernel/sysfile.c returns -1 for these causes:
     *
     *   Cause                              Where it is rejected
     *   ---------------------------------  -------------------------------
     *   path has no '\0' within MAXPATH    argstr() copying the path
     *   path does not exist                namei() returns 0
     *   a middle component is not a dir    namei() -> dirlookup() fails
     *   a T_DIR opened for writing         omode != O_RDONLY check
     *   this process already has NOFILE    fdalloc() finds no free slot
     *   all NFILE system-wide files used   filealloc(): no free struct file
     *   invalid device major               T_DEVICE major outside [0, NDEV)
     *
     * Recursion can exhaust NOFILE: each active call holds a directory fd.
     */
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
   * A: "directory entry" in kernel/fs.h. Directory data is an array of
   * these 16-byte records; read() returns their raw bytes:
   *
   *   Field   Bytes  Holds
   *   ------  -----  ---------------------------------------------------
   *   inum        2  inode number of the child; 0 marks an unused slot
   *   name       14  child's name in this parent; no '\0' if 14 long
   *
   * No path is stored: the caller adds the parent prefix (see user/ls.c).
   */
  /*
   * Q: Is each row of `ls -l` a directory entry?
   *
   *   $ ls -l                        (Linux host, not xv6)
   *   total 4.1M
   *   -rw-r--r-- 1 wahsieh wahsieh  16K Oct 22 18:23 AGENTS.md
   *   -rw-r--r-- 1 wahsieh wahsieh 2.4K Aug 14 14:24 README
   *   drwxr-xr-x 2 wahsieh wahsieh 4.0K Oct 19 17:25 conf
   *
   * A: Each file row represents a directory entry: its name comes from the entry,
   * as does the inode number shown by `ls -i`. stat() supplies inode
   * metadata: mode, link count, owner, size, and time. find() below also
   * uses stat() per entry, but only needs the type. "total" is not an
   * entry; Linux ls hides dot-prefixed names unless given -a. xv6's
   * `ls .` shows all occupied entries, including "." and "..":
   *
   *   Name    Type  Inum  Size     Source
   *   ------  ----  ----  -------  ---------------------------------
   *   .       1     1     1024     de.name, then stat(): T_DIR == 1
   *   ..      1     1     1024     the root is its own parent
   *   README  2     2     2441     T_FILE == 2
   */
  struct dirent de;
  /*
   * user/ls.c calls the path buffer buf and its name pointer p:
   *
   *   Name            ls.c  Holds / used by
   *   --------------  ----  -------------------------------------------
   *   child_path      buf   "path/name\0": printf(), stat(), find()
   *   child_name_ptr  p     &child_path[strlen(path) + 1]: strcmp()
   *
   * child_name_ptr owns no buffer: it points into child_path. Each copy
   * updates both strings while preserving the parent's "path/" prefix.
   */
  char child_path[MAXPATH];
  char *child_name_ptr = child_path;
  int return_status = 0;

  /*
   * Q: What does checking fstat()'s return value protect against?
   * A: Reading uninitialized st.type after failure would use stack garbage.
   *
   *   path names     fstat(fd, &st)  st.type   next step here
   *   -------------  --------------  --------  ------------------------
   *   a directory    returns 0       T_DIR     read dirent records
   *   a file         returns 0       T_FILE    reject: bytes, not slots
   *   the console    returns 0       T_DEVICE  reject: bytes, not slots
   *   (copy failed)  returns -1      garbage   report, close, return
   *
   * The loop calls find() only for T_DIR children, so normally only the
   * starting path reaches the file/device rejection rows.
   *
   * Q: When would fstat() fail here?
   * A: Rarely: open() yields FD_INODE or FD_DEVICE, and &st is valid stack
   * memory. filestat() in kernel/file.c fails for other fd types (e.g., a
   * pipe from pipe(), never open()) or a failed copyout() to &st. sys_fstat()
   * in kernel/sysfile.c also rejects invalid fds; ours came from open().
   */
  if (fstat(fd, &st) < 0) {
    fprintf(2, "find: cannot stat path %s (FD %d)\n", path, fd);
    close(fd);
    return -1;
  }

  /*
   * Q: Why reject a T_FILE start path instead of matching its name?
   * A: GNU find tests starting points; this program requires a directory:
   *
   *   1. The contract is "find <dir> <name>". Handout examples start at a
   *      directory; no grader test passes a file.
   *   2. Matching a start path needs a basename parser, including trailing
   *      '/': "a/b" must match "b", and "./a/" must match "a". That adds
   *      code for untested input.
   *
   * Child names arrive separately in dirent, so the loop can match each
   * before deciding whether to descend.
   */
  /*
   * Q: Why can't the start path be a T_DEVICE?
   * A: Only T_DIR data holds dirent records. fileread() in kernel/file.c
   * routes FD_DEVICE to devsw[major].read. For the console, consoleread()
   * in kernel/console.c can wait for a typed line, then returns keystrokes,
   * not inum/name records. T_FILE holds file contents, not records either.
   */
  if (st.type != T_DIR) {
    fprintf(2, "find: %s is not a directory\n", path);
    close(fd);
    return -1;
  }

  /*
   * Q: Why not reject path "." or ".." here, before the loop?
   * A: "find . b" must search the current directory, not return empty.
   * The loop risk comes from following child records named "." or ".."
   * back into this directory or its parent. The loop skips them before
   * recursion; rejecting path here would only reject the user's start.
   */

  /*
   * Q: Why budget exactly DIRSIZ bytes for the name?
   * A: DIRSIZ is the on-disk maximum name length. Reserving '/' + DIRSIZ +
   * '\0' after the prefix fits every possible child name in child_path.
   */
  // '/' + up to DIRSIZ name bytes + '\0' must fit after the prefix.
  if (strlen(path) + 1 + DIRSIZ + 1 > sizeof(child_path)) {
    fprintf(2, "find: path too long: %s\n", path);
    close(fd);
    return -1;
  }

  strcpy(child_path, path); // strcpy() adds '\0'
  /*
   * Q: Can writes through child_name_ptr run past the end of child_path?
   * A: No: the check precedes all writes. For L = strlen(path):
   *
   *   Byte written  Index               Why it fits in [MAXPATH]
   *   ------------  ------------------  ---------------------------
   *   '/'           L                   the check above guarantees
   *   name bytes    L+1 .. L+DIRSIZ     L + 1 + DIRSIZ + 1 <= 128,
   *   name's '\0'   L+1+DIRSIZ          so this index is <= 127
   *
   * strcpy() first copies path and its '\0'; '/' replaces that terminator.
   * These indices assume char-sized steps: ++ and [DIRSIZ] scale by the
   * pointed-to type, so a wider type would invalidate this bound.
   */
  child_name_ptr += strlen(child_path);
  *child_name_ptr++ = '/';

  /*
   * Q: Why read one dirent per call, and how does the loop stop at EOF?
   * A: Like user/ls.c, this loop reads one 16-byte record at a time without
   * needing the entry count:
   *
   *   read() returns  Means                      Loop
   *   --------------  -------------------------  --------------------
   *   16              one whole dirent arrived   handle it, read next
   *   0               EOF: past the last record  stops
   *   -1              the read failed            stops
   *
   * readi() in kernel/fs.c stops at the inode size. Directory sizes are
   * multiples of 16, so normal reads do not return short positive counts.
   * Batching records saves system calls but enlarges every stack frame.
   * This loop also stops on read errors without marking return_status.
   */
  while (read(fd, &de, sizeof(de)) == sizeof(de)) {
    /*
     * Q: What is inum, and why skip records whose inum is 0?
     * A: The index of the child's on-disk inode (type, size, data blocks).
     * 0 marks an unused slot, not a file; its name prints as an empty string.
     * Its empty name makes child_path just "path/", so stat() would name
     * the parent, not a child.
     *
     * Q: When is de.inum 0?
     * A: sys_unlink() in kernel/sysfile.c zeroes removed entries without
     * compacting the directory; dirlink() in kernel/fs.c reuses the holes.
     * "rm a/b" followed by "find a b" walks over one. mkfs/mkfs.c also
     * pads the root directory with zeroed slots.
     */
    if (de.inum == 0)
      continue;

    /*
     * Q: Could memcpy() replace memmove(), and does either zero-pad a
     * name shorter than DIRSIZ?
     * A: memcpy() works: de.name and child_path do not overlap. In
     * user/ulib.c it calls memmove(); this code follows user/ls.c.
     *
     * Both copy exactly DIRSIZ bytes, including existing zeros; neither
     * pads nor stops at '\0'. dirlink()'s strncpy() in kernel/fs.c pads
     * short names. A full-width name lacks '\0', so strlen(de.name) or
     * strcpy() could overread. The terminator below makes child_name_ptr
     * a C string for either length.
     */
    memmove(child_name_ptr, de.name, DIRSIZ);
    child_name_ptr[DIRSIZ] = '\0';

    /*
     * Q: Why compare child_name_ptr, not path or child_path, with dots?
     * A: Only the bare name identifies the "." and ".." records:
     *
     *   String          Example  Equals "."?  Changes per record?
     *   --------------  -------  -----------  -------------------
     *   path            "."      start only   no: the parent
     *   child_path      "./."    never        yes
     *   child_name_ptr  "."      for "."      yes: this record
     *
     * path tests the same parent every time; child_path includes "path/".
     * Skip dots before matching too, so "find . .." prints nothing.
     */
    if (strcmp(child_name_ptr, ".") == 0 || strcmp(child_name_ptr, "..") == 0)
      continue;

    if (strcmp(child_name_ptr, name) == 0)
      printf("%s\n", child_path);

    /*
     * Q: Why stat() each child when fstat() already ran? How do the two
     * calls differ?
     * A: fstat() described the parent directory. The record has only the
     * child's name and inode number; stat() supplies its type.
     *
     *   Call           Takes           Describes        Opens an fd?
     *   -------------  --------------  ---------------  ----------------
     *   fstat(fd, st)  an open fd      what fd names    no, needs one
     *   stat(path, st) a path string   what path names  briefly, inside
     *
     * stat() in user/ulib.c is open() + fstat() + close(): one temporary
     * fd, shown as frame 3's red slot in the header's figure.
     */
    if (stat(child_path, &st) < 0) {
      fprintf(2, "find: cannot stat path %s\n", child_path);
      return_status = -1;
      continue; // returning here would leak fd and skip the siblings
    }
    /*
     * Q: How many directory fds are open at once during recursion?
     * A: One per active call, plus stat()'s temporary fd (see header limits).
     * Each call opens and closes its own fd. A nested call copies the path
     * into its own frame, leaving this frame's prefix intact on return.
     */
    if (st.type == T_DIR && find(child_path, name) < 0) {
      // return find(...) would skip later siblings and leak fd.
      return_status = -1;
    }
  }

  close(fd);
  return return_status;
}

int
main(int argc, char *argv[])
{
  /*
   * Q: Why require exactly two operands, unlike Ubuntu's `find` and `find .`?
   * A: Bare GNU find prints every path. find(dir, "") instead matches no
   * entry and exits 0 with no output. GNU behavior needs a separate "no
   * filter" mode; keep the handout's "find <dir> <name>" until lab 05 adds
   * -exec (docs/labs/05-util-exec.md).
   */
  if (argc != 3) {
    fprintf(2, "Usage: find <dir> <name>\n");
    exit(1);
  }
  // A failure in one subtree is reported but does not stop the others.
  exit(find(argv[1], argv[2]) < 0 ? 1 : 0);
}
