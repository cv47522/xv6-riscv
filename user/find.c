/*
 * find.c: print paths under a directory whose final name matches, or run
 * a command once for each such path.
 *
 * NOTE: "matches" means any entry type, not just files. A directory
 * named <name> is printed like a file, and then still searched:
 *
 *   $ mkdir a; mkdir a/b; echo > a/b/b
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
 * docs/labs/05-util-exec.md explains the fork/exec/wait derivation.
 */

#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "kernel/stat.h"
#include "kernel/fs.h"
#include "kernel/param.h"
#include "user/user.h"

/*
 * Q: Why fork and exec here instead of once in main()?
 * A: main() runs once per find command; the contract is one command per
 *    match. A fork/exec in main() runs the command once, before or after
 *    the search, without any matched path. An exec in the searching
 *    process itself replaces find, so no later match is ever visited.
 *
 *   Process  Does                            Ends by
 *   -------  ------------------------------  ----------------------------
 *   parent   waits, then resumes the search  returning to find()'s loop
 *   child    builds argv, execs the command  the command's exit, or
 *                                            exit(1) if exec() fails
 *
 * Waiting before the next match keeps command output in search order and
 * reaps each child at once: unreaped children hold NPROC (64) slots until
 * fork() fails. A command's own exit status is not find's failure.
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
     * Q: Why a MAXARG array instead of char *child_argv[n] = {0}?
     * A: n is not a constant, so that is a variable-length array, and
     *    C99 (-std=gnu99) forbids initializing one. main() already bounded
     *    the words, path, and null pointer by MAXARG, which is constant.
     */
    char *child_argv[MAXARG];
    int n = 0;
    /*
     * Q: When is cmd[n] == 0?
     * A: One slot past the last command word. cmd is argv + 4, so cmd[n]
     *    is argv[4 + n], and kexec() in kernel/exec.c stores
     *    ustack[argc] = 0. For find . b -exec grep hello (argc 6):
     *
     *      n  cmd[n]   Same slot   Loop
     *      -  -------  ----------  -----------------------
     *      0  "grep"   argv[4]     copy
     *      1  "hello"  argv[5]     copy
     *      2  0        argv[6]     stop: n == argc - 4 == 2
     */
    while (cmd[n] != 0) {
      child_argv[n] = cmd[n];
      n++;
    }
    child_argv[n++] = path;
    child_argv[n] = 0; // sys_exec() stops only at a zero pointer, not "".

    exec(child_argv[0], child_argv);

    /*
     * Never return into find(): the child shares each open directory fd
     * and its read offset with the parent (filedup() in kfork(),
     * kernel/proc.c), so a second searcher would consume the parent's
     * remaining dirents and print them twice.
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

  struct dirent de; // dirlink() zero-pads names shorter than DIRSIZ.
  /*
   * No zero-initialization needed: strcpy() writes path and its '\0';
   * each copied name gets an explicit '\0'. This saves a 128-byte memset
   * per level. A prefix memcpy() needs separate termination (below).
   */
  char child_path[MAXPATH];
  char *child_name_ptr = child_path;
  int return_status = 0;

  /*
   * One check covers all entries: path is fixed and memcpy() always copies
   * DIRSIZ bytes. Budget '/' + the maximum name + '\0' before any write.
   *
   * Q: When does this reject a short child name that could fit?
   * A: MAXPATH = 128, DIRSIZ = 14. Counts include '/' and '\0':
   *
   *   Parent bytes  Name bytes  Actual total  Budget checked  Result
   *   ------------  ----------  ------------  --------------  --------
   *   112           14          128           128             accept
   *   113            1          116           129             reject
   *   125            1          128           141             reject
   *   126            1          129           142             reject
   *
   * The middle two names fit; the last exceeds MAXPATH even at one byte.
   * Accepting short names within 15 bytes of MAXPATH requires measuring
   * each name safely (de.name may lack '\0') and copying only that length,
   * rather than DIRSIZ bytes. The current check avoids that per-entry work.
   */
  /*
   * Q: Does each memcpy() grow child_path beyond the one-time size check?
   * A: No: child_name_ptr is set once, after "path/", before the loop.
   * memcpy() overwrites DIRSIZ bytes there; it never advances the pointer.
   * For path = "./x", every iteration writes the same byte range:
   *
   *   Entry name      Prefix [0..3]  Name region [4..17]  Terminator [18]
   *   --------------  -------------  -------------------  ---------------
   *   abc             ./x/           abc + 11 NUL bytes   '\0'
   *   fourteen_chars  ./x/           fourteen_chars       '\0'
   *
   * For abc, memcpy() copies the source padding too: child_path[4..6]
   * holds abc; child_path[7] (child_name_ptr[3]) is the first copied NUL.
   * dirlink()'s strncpy() in kernel/fs.c supplies the padding;
   * memcpy() adds none. The explicit NUL at child_path[18] handles
   * all 14 name bytes being nonzero.
   *
   * The name changes; the prefix, pointer, and write bounds do not.
   * Advancing child_name_ptr inside the loop would break this invariant.
   * Recursion uses another call's local buffer, leaving this one intact.
   * See the fixed_width_name_field.c example and
   * c-programming/notes/src/main/arrays_and_strings/strings/fixed_width_name_field.excalidraw
   */
  if (strlen(path) + 1 + DIRSIZ + 1 > sizeof(child_path)) {
    fprintf(2, "find: path too long: %s\n", path);
    close(fd);
    return -1;
  }

  /*
   * strcpy() terminates the prefix for strlen(child_path). Copying only
   * strlen(path) bytes with memcpy() leaves that scan reading uninitialized
   * stack bytes and may misplace '/'; zero the buffer first or reuse
   * strlen(path) as the offset to make that alternative safe.
   */
  strcpy(child_path, path);
  child_name_ptr += strlen(child_path);
  *child_name_ptr++ = '/';

  // Stops at EOF (0) or error (-1); dir sizes are multiples of 16.
  while (read(fd, &de, sizeof(de)) == sizeof(de)) {
    // Free slot: unlink() zeroes inum without compacting the directory.
    if (de.inum == 0)
      continue;

    /*
     * A DIRSIZ-long name has no '\0', so never strlen(de.name).
     * user/printf.c has no %.*s support; %s would hide the NUL padding.
     */
    // ------------ Uncomment For Debugging -------------
    // fprintf(2, "de.name bytes:");
    // for (int i = 0; i < DIRSIZ; i++) {
    //   if (de.name[i] == '\0')
    //     fprintf(2, " \\0");
    //   else
    //     fprintf(2, " %c", de.name[i]);
    // }
    // fprintf(2, "\n");
    // ------------------------------------
    memcpy(child_name_ptr, de.name, DIRSIZ);
    child_name_ptr[DIRSIZ] = '\0';

    /*
     * Never treat de.name as a C string: a DIRSIZ (14)-byte name lacks '\0'.
     * strcmp(de.name, ".") or ".." stops by index 2 on any full-width
     * name, but a user-supplied 14+ byte name could make it read past de.
     * child_name_ptr is terminated and safe for every comparison.
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
     * Match the bare name, print the full path: "./a/b" is not "b".
     * Any type matches, including directories (the grader checks this);
     * GNU `find . -name b` agrees. Its separate `-type f` filter is omitted.
     * The path locates the entry; matching directories are still searched.
     */
    if (strcmp(child_name_ptr, name) == 0) {
      /*
       * Q: Why not `else { return_status = run(...); }`?
       * A: return_status is sticky. Assigning run()'s 0 would erase a -1
       *    already recorded by an earlier stat() failure or failed run();
       *    only failures may write it, as in the descent check below.
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
   * Test argc before reading argv[3]. With argc == 3, argv[3] is the 0
   * from ustack[argc] in kernel/exec.c, and user text is linked at
   * address 0 (user/user.ld), so strcmp() reads code bytes instead of
   * faulting and silently rejects the plain form.
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
   * Q: Is argv + 4 the same as argv[4]? Why pass a pointer to a pointer
   *    instead of argv[4..argc-1]?
   * A: No. argv[4] is *(argv + 4), one string; argv + 4 is the address
   *    of that slot, so it reaches every later slot too:
   *
   *      Expression  Type     For find . b -exec grep hello
   *      ----------  -------  ---------------------------------
   *      argv[4]     char *   "grep" only
   *      argv + 4    char **  &argv[4]: "grep", "hello", then 0
   *
   *    C has no slice type: an array argument decays to a pointer to its
   *    first element, so argv + 4 is how a function receives
   *    argv[4..argc-1]. Its end needs no separate count because
   *    argv[argc] is 0 (kexec() in kernel/exec.c). Nothing is copied;
   *    run() copies into child_argv only because it appends the path.
   */
  /*
   * Q: How is argv[argc] known to be 0? Who implements it?
   * A: Each hop below needs a zero pointer from the one above it, and the
   *    kernel writes the final one that main() sees:
   *
   *      Step  Source                 What it does
   *      ----  ---------------------  ----------------------------------
   *      1     parseexec(), sh.c      sets cmd->argv[argc] = 0
   *      2     sys_exec(), sysfile.c  copies pointers until it reads 0;
   *                                   fails if none within MAXARG
   *      3     kexec(), exec.c        sets ustack[argc] = 0 and copies
   *                                   argc + 1 pointers to the stack
   *      4     kexec(), exec.c        sets a1 = sp, main()'s argv
   *
   *    Step 3 is the guarantee: kexec() rebuilds the vector from the
   *    strings, so main() always gets a 0 after the last pointer. Hosted
   *    ISO C requires the same, so this is not an xv6 quirk.
   */
  exit(find(argv[1], argv[2], argv + 4) < 0 ? 1 : 0);
}
