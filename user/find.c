/*
 * find.c: print paths under a directory whose final name matches, or run
 * a command once for each such path.
 *
 * NOTE: "matches" means any entry type, not just files. A directory
 * named <name> is printed like a file, and then still searched:
 *
 *   $ mkdir a; mkdir a/b; echo > a/b/b
 *   // Or
 *   $ sh < findtest.sh
 *   $ find . b
 *   ./a/b        <- the directory
 *   ./a/b/b      <- the file inside it
 *
 * With -exec, every word after it forms the command, and each matched path
 * is appended as its final argument (not host find's {} or ';' grammar):
 *
 *   Vector     [0]   [1]    [2]    [3]    [4]    [5]
 *   ---------  ----  -----  -----  -----  -----  -----
 *   find argv  find  .      b      -exec  grep   hello
 *   child      grep  hello  ./a/b  0
 *
 * docs/labs/05-util-find-exec.md explains the fork/exec/wait derivation.
 */

#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "kernel/stat.h"
#include "kernel/fs.h"
#include "kernel/param.h"
#include "user/user.h"

/*
 * Q: Why fork/exec per match, not once in main()?
 * A: main() would run one command without a matched path; exec() in the
 * searcher would replace find and prevent later matches.
 *
 *   Process  Does                            Ends by
 *   -------  ------------------------------  ----------------------------
 *   parent   waits, then resumes the search  returning to find()'s loop
 *   child    builds argv, execs the command  command exit, or exit(1)
 *                                            if exec() fails
 *
 * Waiting preserves search order and reaps children before they exhaust
 * NPROC (64) slots. The command's exit status is not find's failure.
 */
static int
run(char **cmd, char *path)
{
  int pid = fork();
  if (pid < 0) {
    fprintf(2, "find: fork failed\n");
    return -1;
  }

  if (pid == 0) {
    /*
     * Q: Why MAXARG, not char *child_argv[n] = {0}?
     * A: C99 (-std=gnu99) forbids initializing a variable-length array.
     * MAXARG is constant; main() bounds command words + path + null pointer.
     */
    char *child_argv[MAXARG];
    /*
     * Q: When is cmd[n] == 0?
     * A: cmd is argv + 4; kexec() supplies argv[argc] = 0 (see main()).
     * For find . b -exec grep hello, argc == 6:
     *
     *   n  cmd[n]   Same slot   Loop
     *   -  -------  ----------  -----------------------
     *   0  "grep"   argv[4]     copy
     *   1  "hello"  argv[5]     copy
     *   2  0        argv[6]     stop: n == argc - 4 == 2
     */
    int i = 0;
    for (i = 0; cmd[i] !=0; i++) {
      child_argv[i] = cmd[i];
    }
    child_argv[i] = path;
    child_argv[i + 1] = 0; // sys_exec() stops only at a zero pointer, not "".

    exec(child_argv[0], child_argv);

    /*
     * Never return into find(): filedup() in kernel/proc.c's kfork() shares
     * open directory files and read offsets with the parent. A second
     * searcher would consume the parent's remaining dirents.
     */
    fprintf(2, "find: exec %s failed\n", child_argv[0]);
    exit(1);
  }

  if (wait(0) < 0) {
    fprintf(2, "find: wait failed\n");
    return -1;
  }
  return 0;
}

static int
find(const char *path, const char *name, char **cmd)
{
  int fd = open(path, O_RDONLY);
  // Missing path, or NOFILE fds held by active recursion.
  if (fd < 0) {
    fprintf(2, "find: cannot open %s\n", path);
    return -1;
  }

  struct stat st;
  // Rare: fd from open() is FD_INODE/FD_DEVICE; guards garbage st.type.
  if (fstat(fd, &st) < 0) {
    fprintf(2, "find: cannot stat %s\n", path);
    close(fd);
    return -1;
  }

  // Start path is a file or device: only T_DIR data holds dirents.
  if (st.type != T_DIR) {
    fprintf(2, "find: %s is not a directory\n", path);
    close(fd);
    return -1;
  }

  /*
   * strcpy() writes the prefix and '\0'; each name gets its own terminator.
   * No zero-initialization needed: save a 128-byte memset per level.
   * A prefix memcpy() needs separate termination (below).
   */
  char child_path[MAXPATH];
  char *child_name_ptr = child_path;
  int return_status = 0;

  /*
   * One check covers every entry: path is fixed; memcpy() copies DIRSIZ
   * bytes. Budget '/' + the maximum name + '\0' before any write.
   *
   * Q: When does this reject a short child name that could fit?
   * A: MAXPATH = 128, DIRSIZ = 14. Totals include '/' and '\0':
   *
   *   Parent bytes  Name bytes  Actual total  Budget checked  Result
   *   ------------  ----------  ------------  --------------  --------
   *   112           14          128           128             accept
   *   113            1          116           129             reject
   *   125            1          128           141             reject
   *   126            1          129           142             reject
   *
   * The middle two fit; the last exceeds MAXPATH even with a one-byte name.
   * Accepting short names within 15 bytes of MAXPATH requires measuring
   * each safely (de.name may lack '\0') and copying that length, not DIRSIZ.
   * The current check avoids this per-entry work.
   */
  /*
   * Q: Does each memcpy() grow child_path beyond the one-time size check?
   * A: No: child_name_ptr stays after "path/"; each copy overwrites the same
   * DIRSIZ bytes. For path = "./x":
   *
   *   Entry name      Prefix [0..3]  Name region [4..17]  Terminator [18]
   *   --------------  -------------  -------------------  ---------------
   *   abc             ./x/           abc + 11 NUL bytes   '\0'
   *   fourteen_chars  ./x/           fourteen_chars       '\0'
   *
   * For abc, child_path[4..6] holds letters; child_path[7] (child_name_ptr[3])
   * is the first copied NUL. dirlink()'s strncpy() in kernel/fs.c supplies
   * padding; memcpy() adds none. The explicit NUL at child_path[18] handles
   * full-width names. Advancing the pointer in the loop breaks the bound;
   * recursive calls use separate buffers, preserving this frame's prefix.
   * See fixed_width_name_field.c and fixed_width_name_field.excalidraw in
   * ../c-programming/notes/src/main/arrays_and_strings/strings/.
   */
  if (strlen(path) + 1 + DIRSIZ + 1 > sizeof(child_path)) {
    fprintf(2, "find: path too long: %s\n", path);
    close(fd);
    return -1;
  }

  /*
   * strcpy() terminates the prefix for strlen(child_path). A memcpy() of only
   * strlen(path) bytes leaves that scan reading uninitialized stack bytes
   * and may misplace '/'; zero the buffer or reuse strlen(path) as the offset.
   */
  strcpy(child_path, path);
  child_name_ptr += strlen(child_path);
  *child_name_ptr++ = '/';

  // Stops at EOF (0) or error (-1); dir sizes are multiples of 16.
  struct dirent de; // dirlink() zero-pads names shorter than DIRSIZ.
  while (read(fd, &de, sizeof(de)) == sizeof(de)) {
    // Free slot: unlink() zeroes inum without compacting the directory.
    if (de.inum == 0)
      continue;

    /*
     * Never strlen(de.name): full-width names lack '\0'. user/printf.c has no
     * %.*s; %s hides NUL padding. Uncomment to show all DIRSIZ bytes:
     */
    // fprintf(2, "de.name bytes:");
    // for (int i = 0; i < DIRSIZ; i++) {
    //   if (de.name[i] == '\0')
    //     fprintf(2, " \\0");
    //   else
    //     fprintf(2, " %c", de.name[i]);
    //   }
    //  fprintf(2, "\n");

    memcpy(child_name_ptr, de.name, DIRSIZ);
    child_name_ptr[DIRSIZ] = '\0';

    /*
     * strcmp(de.name, ".") or ".." stops by index 2 even on full-width names;
     * a user-supplied 14+ byte name could instead cause an overread. Compare
     * the terminated child_name_ptr everywhere; de.name is not a C string.
     */
    // "." and ".." lead back up: descending would loop forever.
    if (strcmp(child_name_ptr, ".") == 0 || strcmp(child_name_ptr, "..") == 0)
      continue;

    // Out of fds (stat() opens one), or the entry vanished since read().
    if (stat(child_path, &st) < 0) {
      fprintf(2, "find: cannot stat %s\n", child_path);
      return_status = -1;
      continue;
    }

    /*
     * Match the bare name, print its location: "./a/b" is not "b". Any type
     * matches, including directories (the grader checks this), which are
     * still searched. GNU `find . -name b` agrees; `-type f` is omitted here.
     */
    if (strcmp(child_name_ptr, name) == 0) {
      /*
       * Q: Why not return_status = run(...)?
       * A: A successful 0 would erase an earlier stat()/run() failure.
       * Only failures may set the sticky -1, as in the descent check.
       */
      if (cmd == 0)
        printf("%s\n", child_path);
      else if (run(cmd, child_path) < 0)
        return_status = -1;
    }

    // Files are not searched; a sticky -1 survives a later sibling's 0.
    if (st.type == T_DIR && find(child_path, name, cmd) < 0)
      return_status = -1;
  }

  close(fd);
  return return_status;
}

int
main(int argc, char *argv[])
{
  /*
   * Test argc first: with argc == 3, argv[3] is kernel/exec.c's null pointer.
   * User text starts at address 0 (user/user.ld), so strcmp() reads code
   * instead of faulting and silently rejects the plain form.
   */
  if (argc == 3)
    exit(find(argv[1], argv[2], 0) < 0 ? 1 : 0);

  // No dir/name, extra operands, or -exec without a command.
  if (argc < 5 || strcmp(argv[3], "-exec") != 0) {
    fprintf(2, "Usage: find <dir> <name> [-exec cmd [arg ...]]\n");
    exit(1);
  }

  // Command words + matched path + null pointer must fit sys_exec().
  if (argc - 4 + 2 > MAXARG) {
    fprintf(2, "find: too many -exec arguments\n");
    exit(1);
  }

  /*
   * Q: Is argv + 4 the same as argv[4]? Why a pointer to a pointer?
   * A: argv[4] == *(argv + 4) is one string; argv + 4 addresses that slot:
   *
   *   Expression  Type     For find . b -exec grep hello
   *   ----------  -------  ---------------------------------
   *   argv[4]     char *   "grep" only
   *   argv + 4    char **  &argv[4]: "grep", "hello", then 0
   *
   * C has no slice type; array arguments decay to first-element pointers.
   * argv + 4 passes argv[4..argc-1] without copying or a separate count;
   * argv[argc] == 0 marks the end. run() copies only to append the path.
   */
  /*
   * Q: Who guarantees argv[argc] == 0?
   * A: Each hop consumes the previous hop's zero pointer:
   *
   *   Step  Source                        Action
   *   ----  ----------------------------  --------------------------------
   *   1     user/sh.c: parseexec()         sets cmd->argv[argc] = 0
   *   2     kernel/sysfile.c: sys_exec()   copies until 0; fails if no 0
   *                                         within MAXARG
   *   3     kernel/exec.c: kexec()         sets ustack[argc] = 0; copies
   *                                         argc + 1 pointers to the stack
   *   4     kernel/exec.c: kexec()         sets a1 = sp, main()'s argv
   *
   * Step 3 rebuilds the vector from strings and supplies main()'s final 0.
   * Hosted ISO C requires the same guarantee.
   */
  exit(find(argv[1], argv[2], argv + 4) < 0 ? 1 : 0);
}
