/*
 * find_basic.c: print paths under a directory whose final name matches.
 *
 * NOTE: "matches" means any entry type, not just files. A directory
 * named <name> is printed like a file, and then still searched.
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
 * Match child_name_ptr, print child_path: "./a/b" is not "b". The "name
 * matches" row has no type condition: a matching directory prints just
 * like a file, then is still searched, as the grader's directory-match
 * case requires.
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
     * sys_open() in kernel/sysfile.c returns -1 for these causes:
     *
     *   Cause                              Rejection point
     *   ---------------------------------  --------------------------------
     *   no '\0' within MAXPATH             argstr() copying the path
     *   nonexistent path                   namei() returns 0
     *   non-directory middle component     namei() -> dirlookup() fails
     *   T_DIR opened for writing           omode != O_RDONLY
     *   NOFILE fds in this process          fdalloc(): no free slot
     *   NFILE system-wide files in use      filealloc(): no free struct file
     *   invalid device major               T_DEVICE major outside [0, NDEV)
     *
     * Each active recursive call holds a directory fd and can exhaust NOFILE.
     */
    fprintf(2, "find_basic: cannot open %s\n", path);
    /*
     * Failed open() leaves no fd to release. close(-1) would fail too:
     * argfd() in kernel/sysfile.c rejects it before touching p->ofile[].
     */
    return -1;
  }

  struct stat st;
  /*
   * Check before reading st.type; failure leaves it uninitialized:
   *
   *   path names     fstat(fd, &st)  st.type   Next step
   *   -------------  --------------  --------  --------------------------
   *   a directory    0               T_DIR     read dirent records
   *   a file         0               T_FILE    reject: contents, not slots
   *   the console    0               T_DEVICE  reject: keystrokes, not slots
   *   (copy failed)  -1              garbage   report, close, return
   *
   * Failure is rare: open() yields FD_INODE/FD_DEVICE and &st is valid stack
   * memory. filestat() in kernel/file.c rejects other fd types (e.g., pipes
   * from pipe(), never open()) or failed copyout(); sys_fstat() in
   * kernel/sysfile.c rejects invalid fds. Normally only the starting path
   * can reach the file/device rows: recursion is guarded by T_DIR.
   */
  if (fstat(fd, &st) < 0) {
    fprintf(2, "find_basic: cannot stat path %s (FD %d)\n", path, fd);
    close(fd);
    return -1;
  }

  /*
   * Unlike GNU find, this program requires a directory starting point:
   *
   * 1. The contract is "find <dir> <name>"; handout examples start at a
   *    directory, and no grader test passes a file.
   * 2. Matching starting points needs an otherwise untested basename parser,
   *    including trailing '/': "a/b" must match "b", "./a/" must match "a".
   *
   * Children already have separate dirent names to match before descending.
   */
  /*
   * Only T_DIR data holds dirents; T_FILE holds contents. For FD_DEVICE,
   * fileread() in kernel/file.c calls devsw[major].read; consoleread() in
   * kernel/console.c may wait for a typed line and returns keystrokes,
   * not inum/name records.
   */
  if (st.type != T_DIR) {
    fprintf(2, "find_basic: %s is not a directory\n", path);
    close(fd);
    return -1;
  }

  /*
   * Accept "." and ".." as starting paths: "find . b" must search here.
   * Only child records with those names loop back into the current/parent
   * directory; skip them inside the loop, before matching and recursion.
   */

  /*
   * dirent = "directory entry" (kernel/fs.h). read() returns raw 16-byte
   * records from the directory's array:
   *
   *   Field   Bytes  Holds
   *   ------  -----  ---------------------------------------------------
   *   inum        2  child's inode number; 0 marks an unused slot
   *   name       14  child's name in this parent; no '\0' if 14 long
   *
   * No path is stored; add the parent prefix as in user/ls.c.
   */
  /*
   * Each file row in Linux `ls -l` represents a directory entry:
   *
   *   $ ls -l                        (Linux host, not xv6)
   *   total 4.1M
   *   -rw-r--r-- 1 wahsieh wahsieh  16K Oct 22 18:23 AGENTS.md
   *   -rw-r--r-- 1 wahsieh wahsieh 2.4K Aug 14 14:24 README
   *   drwxr-xr-x 2 wahsieh wahsieh 4.0K Oct 19 17:25 conf
   *
   * The entry supplies the name and `ls -i` inode number; stat() supplies
   * mode, link count, owner, size, and time. find() needs only the type.
   * "total" is not an entry. Linux hides dot-prefixed names without -a;
   * xv6's `ls .` shows all occupied entries, including "." and "..":
   *
   *   Name    Type  Inum  Size     Source
   *   ------  ----  ----  -------  ---------------------------------
   *   .       1     1     1024     de.name, then stat(): T_DIR == 1
   *   ..      1     1     1024     the root is its own parent
   *   README  2     2     2441     T_FILE == 2
   */
  struct dirent de; // Fixed-size name[] in struct dirent automatically pads shorter names w/ zeros
  /*
   *   Name            user/ls.c  Holds / used by
   *   --------------  ---------  -----------------------------------------
   *   child_path      buf        "path/name\0": printf(), stat(), find()
   *   child_name_ptr  p          &child_path[strlen(path) + 1]: strcmp()
   *
   * The pointer owns no buffer: it aliases child_path. Copying each name
   * updates both strings while preserving the parent's "path/" prefix.
   */
  char child_path[MAXPATH];
  char *child_name_ptr = child_path;
  int return_status = 0;
  /*
   * DIRSIZ is the on-disk maximum name length; reserve '/' + DIRSIZ + '\0'
   * after the prefix so every possible child name fits.
   */
  if (strlen(path) + 1 + DIRSIZ + 1 > sizeof(child_path)) {
    fprintf(2, "find_basic: path too long: %s\n", path);
    close(fd);
    return -1;
  }

  strcpy(child_path, path);
  /*
   * The check precedes all writes. For L = strlen(path):
   *
   *   Byte written  Index               Bound in child_path[MAXPATH]
   *   ------------  ------------------  ---------------------------
   *   '/'           L                   L + 1 + DIRSIZ + 1 <= 128
   *   name bytes    L+1 .. L+DIRSIZ      (guaranteed above)
   *   name's '\0'   L+1+DIRSIZ          last index <= 127
   *
   * strcpy() copies path and '\0'; '/' replaces that terminator. The bound
   * assumes char-sized steps: ++ and [DIRSIZ] scale by the pointed-to type;
   * a wider type would invalidate it.
   */
  child_name_ptr += strlen(child_path);
  *child_name_ptr++ = '/';

  /*
   * Like user/ls.c, read one 16-byte dirent per call; no entry count needed:
   *
   *   read() returns  Means                      Loop
   *   --------------  -------------------------  --------------------
   *   16              one whole dirent arrived   handle it, read next
   *   0               EOF: past the last record  stops
   *   -1              read failed                stops; status unchanged
   *
   * readi() in kernel/fs.c stops at inode size. Directory sizes are multiples
   * of 16, so normal reads are not short positive counts. Batching saves
   * system calls but enlarges every stack frame.
   */
  while (read(fd, &de, sizeof(de)) == sizeof(de)) {
    /*
     * inum indexes the child's on-disk inode (type, size, data blocks).
     * 0 means unused: its empty name would make stat("path/") name the parent.
     * sys_unlink() in kernel/sysfile.c zeroes slots without compacting;
     * dirlink() in kernel/fs.c reuses them. "rm a/b; find a b" encounters one;
     * mkfs/mkfs.c also pads the root directory with zeroed slots.
     */
    if (de.inum == 0)
      continue;

    /*
     * A DIRSIZ-long name has no '\0', so never strlen(de.name).
     * user/printf.c has no %.*s support; %s would hide the NUL padding.
     */
    // ------------ Uncomment For Debugging -------------
    fprintf(2, "de.name bytes:");
    for (int i = 0; i < DIRSIZ; i++) {
      if (de.name[i] == '\0')
        fprintf(2, " \\0");
      else
        fprintf(2, " %c", de.name[i]);
    }
    fprintf(2, "\n");
    // ------------------------------------
    memcpy(child_name_ptr, de.name, DIRSIZ);
    child_name_ptr[DIRSIZ] = '\0';

    /*
     * memcpy() would work: these buffers do not overlap. In user/ulib.c it
     * calls memmove(); this code follows user/ls.c. Both copy exactly DIRSIZ
     * bytes, including existing zeros; neither pads nor stops at '\0'.
     * dirlink()'s strncpy() in kernel/fs.c pads short names; full-width names
     * lack '\0', so strlen(de.name) or strcpy() could overread. Terminating
     * child_name_ptr makes either length a C string.
     */
    memmove(child_name_ptr, de.name, DIRSIZ);
    child_name_ptr[DIRSIZ] = '\0';

    /*
     * Only the bare name identifies child "." and ".." records:
     *
     *   String          Example  Equals "."?  Changes per record?
     *   --------------  -------  -----------  -------------------
     *   path            "."      start only   no: same parent
     *   child_path      "./."    never        yes: includes "path/"
     *   child_name_ptr  "."      for "."      yes: bare name
     *
     * Skip dots before matching too, so "find . .." prints nothing.
     */
    if (strcmp(child_name_ptr, ".") == 0 || strcmp(child_name_ptr, "..") == 0)
      continue;

    /*
     * fstat() described the parent; dirent has no child type, so stat() it:
     *
     *   Call            Takes        Describes        Opens an fd?
     *   --------------  -----------  ---------------  ----------------
     *   fstat(fd, st)   an open fd   what fd names    no, needs one
     *   stat(path, st)  path string  what path names  briefly, inside
     *
     * stat() in user/ulib.c is open() + fstat() + close(): one temporary fd
     * (frame 3's red slot in the header's linked figure).
     */
    if (stat(child_path, &st) < 0) {
      fprintf(2, "find_basic: cannot stat path %s\n", child_path);
      return_status = -1;
      continue; // returning here would leak fd and skip the siblings
    }

    /*
     * Why `if (st.type != T_DIR && strcmp(name, child_path))` is wrong:
     *
     *   Part              Problem                       Symptom
     *   ----------------  ----------------------------  ---------------------
     *   strcmp() != 0     0 means match                 prints differing files
     *   child_path        "./a/b" never equals "b"      with == 0, prints none
     *   st.type != T_DIR  skips matching directories    fails directory match
     *
     * Each breaks a grader case. Name decides printing; type only decides
     * whether to descend.
     */
    if (strcmp(child_name_ptr, name) == 0)
      printf("%s\n", child_path);

    /*
     * Each call owns one directory fd and a path copy in its stack frame;
     * returning preserves the caller's prefix. See header limits; stat()
     * briefly needs one more fd.
     *
     *   Change            Effect
     *   ----------------  ------------------------------------------------
     *   no T_DIR guard    reopens each file, reports "is not a directory" on
     *                     fd 2, returns -1; a clean search exits 1
     *   assign recursive  a later sibling's 0 overwrites an earlier -1;
     *   result directly   a failed subtree can exit 0
     *
     * The grader's result regexes anchor on "./", while diagnostics start
     * with "find_basic:", so tests may miss the wrong exit status and extra
     * open()/fstat() per file. Set only -1 to keep failures sticky.
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
   * Bare GNU find prints every path; find(dir, "") matches nothing and exits
   * 0. GNU behavior needs a separate "no filter" mode. Require the handout's
   * two operands until lab 05 adds -exec (docs/labs/05-util-find-exec.md).
   */
  if (argc != 3) {
    fprintf(2, "Usage: find_basic <dir> <name>\n");
    exit(1);
  }
  // A failure in one subtree is reported but does not stop the others.
  exit(find(argv[1], argv[2]) < 0 ? 1 : 0);
}
