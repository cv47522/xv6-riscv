# Lab util: Deriving `find`

---

[toc]

---

MIT's [2026 util lab](https://pdos.csail.mit.edu/6.1810/2026/labs/util.html#find) asks for a directory-tree search in `user/find.c`. This note connects that contract to the existing directory reader in `user/ls.c`; it stops before the new program's implementation.

> [!IMPORTANT]
> This is a derivation guide, not a solution. Its C quotation is an unchanged function from `user/ls.c`, and the decisions for `user/find.c` remain questions.

## What the exercise asks for

| Contract        | Required behavior                                                                                        |
| --------------- | -------------------------------------------------------------------------------------------------------- |
| **Input**       | A starting directory and a name to find, as in `find . b`.                                               |
| **Search**      | Visit names throughout that directory tree, including names of directories.                              |
| **Match**       | Compare the entry's name, not the whole path, with the requested name.                                   |
| **Output**      | Print the path of each match, including its starting-path prefix, as in `./a/b`.                         |
| **Termination** | Do not descend through the special `.` and `..` entries.                                                 |
| **Build**       | Put the source directly in `user/find.c` and register `$U/_find` in `UPROGS`; this tree has neither yet. |

The 2026 handout gives no difficulty label for this exercise. The relevant tests in [`grade-lab-util`](../../grade-lab-util) total 40 points. The regex checks inspect console lines; they are narrower than a full specification of ordering, errors, or every pathname shape.

| Test                                                       | Points | Exact checks                                                                                                                                                                                                                                    | What they leave open                                                                                                        |
| ---------------------------------------------------------- | -----: | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | --------------------------------------------------------------------------------------------------------------------------- |
| **`find, in current directory`**                           |     10 | Creates a random file in `.`; requires a line beginning with the regex `./<name>` and forbids lines beginning `./README` or `README`.                                                                                                           | `.` is an unescaped regex wildcard, and the expected name is not end-anchored; this does not verify a unique or exact line. |
| **`find, in sub-directory`**                               |     10 | Creates the same random basename in `.` and a new subdirectory; searches from that subdirectory; requires a line beginning `<dir>/<name>` and forbids a line beginning `./<name>`.                                                              | It checks the requested start point, not all possible sibling paths or output order.                                        |
| **`find, recursive`**                                      |     10 | Places one matching basename at three paths, including a grandchild; requires a line beginning with each expected path.                                                                                                                         | It does not count duplicates or reject extra lines.                                                                         |
| **`find, missing target, no duplicates, directory match`** |     10 | Counts one output line beginning `./` and containing the present random name, zero containing an absent random name, and one containing a matching directory name; then requires `DONE` and forbids lines beginning `path too long` or `panic`. | The regexes are not end-anchored; the test does not define diagnostics for bad arguments or failed I/O.                     |

The grader's [`assert_lines_match()`](../../gradelib.py) applies `re.match()` to each line, while the corner test uses `re.findall()` on the whole transcript. Thus a passing test proves its stated observations, not an unstated choice about traversal order or error messages.

## Prerequisites

| When                | Read                                                                                                                                                  | Ownership                                                                                                                                                              |
| ------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **Read first**      | [`02-build-boot-and-usage.md`](../02-build-boot-and-usage.md#adding-and-running-a-user-exercise)                                                      | User-program location, arguments, xv6 C style, `UPROGS`, and image creation.                                                                                           |
| **Read first**      | [`C_String.md`](../../../c-programming/C_String.md)                                                                                                   | The sibling study note introduces terminated strings and comparison; [`user/ulib.c`](../../user/ulib.c) supplies this tree's actual `strcmp`, `strlen`, and `memmove`. |
| **Read first**      | [`07-exercises.md`'s `ex9ls`](../07-exercises.md#ex9ls--a-directory-is-a-file) and [`user/ex9ls.c`](../../user/ex9ls.c)                               | A smaller directory reader before the fuller `user/ls.c` walkthrough below.                                                                                            |
| **Use when needed** | [`05-syscall-reference.md`](../05-syscall-reference.md#files-and-descriptors) and its [path limits](../05-syscall-reference.md#directories-and-paths) | `open`, `read`, `fstat`, `stat`, `close`, and pathname limits.                                                                                                         |
| **Use when needed** | [`book/ch10-file-system.md`](../book/ch10-file-system.md) and the book's [`fs.tex`](../../../xv6-riscv-book/fs.tex)                                   | The xv6 directory and pathname mechanisms; the chapter note is still a placeholder, so the narrow record-format explanation below is held here temporarily.            |
| **Use when needed** | [`03-lab-workflow.md`](../03-lab-workflow.md) and [`06-build-artifacts.md`](../06-build-artifacts.md)                                                 | Lab grading and generated system-call stubs; neither mechanism needs repeating here.                                                                                   |

> [!NOTE]
> The sibling C note teaches hosted C concepts, not xv6's callable API. [`user/user.h`](../../user/user.h) is the complete user interface here; `opendir()` and `readdir()` are not available.

## The code to read first

| File                                                                          | Contributes                                                                                                    | Does not contribute                                                    |
| ----------------------------------------------------------------------------- | -------------------------------------------------------------------------------------------------------------- | ---------------------------------------------------------------------- |
| [`user/ls.c`](../../user/ls.c)                                                | `ls()` opens a path, checks its type, reads directory records, builds a child path, and closes the descriptor. | No recursive descent and no requested-name filter.                     |
| [`user/ex9ls.c`](../../user/ex9ls.c)                                          | `list()` exposes one fixed-size record at a time without `ls()`'s metadata formatting.                         | No full paths for entries and no recursive descent.                    |
| [`kernel/fs.h`](../../kernel/fs.h) and [`kernel/stat.h`](../../kernel/stat.h) | `struct dirent`, `DIRSIZ`, `struct stat`, and `T_DIR`.                                                         | No user-level traversal policy.                                        |
| [`user/ulib.c`](../../user/ulib.c)                                            | `stat()` delegates to `open`, `fstat`, and `close`; `strcmp()` compares terminated strings.                    | No host C library.                                                     |
| [`kernel/fs.c`](../../kernel/fs.c)                                            | `namex()` resolves path components, and `dirlookup()` resolves one name in a directory.                        | No `find` operation; the new program must choose which paths to visit. |

## `ls()` line by line

This is the complete, unchanged `ls()` function from [`user/ls.c`](../../user/ls.c). It is existing source, not a template for the target file.

```c
void
ls(char *path)
{
  char buf[512], *p;
  int fd;
  struct dirent de;
  struct stat st;

  if ((fd = open(path, O_RDONLY)) < 0) {
    fprintf(2, "ls: cannot open %s\n", path);
    return;
  }

  if (fstat(fd, &st) < 0) {
    fprintf(2, "ls: cannot stat %s\n", path);
    close(fd);
    return;
  }

  switch (st.type) {
  case T_DEVICE:
  case T_FILE:
    printf("%s %d %d %d\n", fmtname(path), st.type, st.ino, (int)st.size);
    break;

  case T_DIR:
    if (strlen(path) + 1 + DIRSIZ + 1 > sizeof buf) {
      printf("ls: path too long\n");
      break;
    }
    strcpy(buf, path);
    p = buf + strlen(buf);
    *p++ = '/';
    while (read(fd, &de, sizeof(de)) == sizeof(de)) {
      if (de.inum == 0)
        continue;
      memmove(p, de.name, DIRSIZ);
      p[DIRSIZ] = 0;
      if (stat(buf, &st) < 0) {
        printf("ls: cannot stat %s\n", buf);
        continue;
      }
      printf("%s %d %d %d\n", fmtname(buf), st.type, st.ino, (int)st.size);
    }
    break;
  }
  close(fd);
}
```

| Branch or expression           | Consequence in this source                                                                                 |
| ------------------------------ | ---------------------------------------------------------------------------------------------------------- |
| **`open` and `fstat` failure** | The open failure returns without a descriptor; the `fstat` failure closes the descriptor before returning. |
| **`T_FILE` or `T_DEVICE`**     | `ls` prints metadata for that path rather than reading it as directory records.                            |
| **`T_DIR` and buffer check**   | The check budgets a slash, a full `DIRSIZ` component, and a terminator in its 512-byte local buffer.       |
| **Fixed-size `read`**          | A full `struct dirent` is one record; a short read ends the loop. `de.inum == 0` is an unused slot.        |
| **`memmove` plus terminator**  | A 14-byte name need not contain `\0`, so `strcpy` from `de.name` would read beyond its field.              |
| **`stat(buf, &st)`**           | The constructed path yields the child's type; on failure, `ls` reports it and continues.                   |
| **Final `close`**              | It runs after both the file/device and directory branches, including the too-long-path break.              |

`ls()` does not filter `.` and `..` because displaying them is useful. Their presence becomes consequential only when a tree traversal follows directory entries into further directories.

## The concept underneath

| Constant or record  | Value in this tree                    | Source                                   | Why it matters                                                                                                         |
| ------------------- | ------------------------------------- | ---------------------------------------- | ---------------------------------------------------------------------------------------------------------------------- |
| **`DIRSIZ`**        | 14 bytes per name                     | [`kernel/fs.h`](../../kernel/fs.h)       | A full-width entry name has no spare byte for `\0`.                                                                    |
| **`struct dirent`** | 2-byte inode number plus 14-byte name | [`kernel/fs.h`](../../kernel/fs.h)       | Directory `read()` exposes 16-byte slots, including reusable holes with `inum == 0`.                                   |
| **`T_DIR`**         | 1                                     | [`kernel/stat.h`](../../kernel/stat.h)   | Only this type contains directory entries to inspect.                                                                  |
| **`MAXPATH`**       | 128 bytes                             | [`kernel/param.h`](../../kernel/param.h) | The kernel copies pathname arguments into bounded buffers; `ls()`'s 512-byte local buffer does not enlarge that limit. |

> [!IMPORTANT]
> A directory entry names an inode inside one parent. A displayed path is a separate string assembled from the starting path and successive names; `struct dirent` does not store that path.

The book's [`fs.tex`](../../../xv6-riscv-book/fs.tex) and this tree's [`kernel/fs.c`](../../kernel/fs.c) describe `namex()` resolving paths from `/` or the process's current directory, then looking up each component. The current directory aliases `.` and `..` are real entries created by [`create()` in `kernel/sysfile.c`](../../kernel/sysfile.c). They are harmless to list, but following them repeatedly would revisit the same directories. The missing-target test and duplicate count expose that failure without proving a general cycle-detection strategy.

`strcmp()` in [`user/ulib.c`](../../user/ulib.c) compares terminated strings; `==` compares pointer values. The 14-byte record field therefore needs a safe conversion before string comparison. `ls()` shows one such boundary. Its `stat()` call is a user-library wrapper, not a separate kernel primitive, and it closes the temporary descriptor it opens.

## Deriving it

| Contract for `user/find.c`                 | Source-backed question to settle                                                                                                                                                                                                                                 |
| ------------------------------------------ | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **Interpret the two operands**             | How does [`user/echo.c`](../../user/echo.c) or [`02-build-boot-and-usage.md`](../02-build-boot-and-usage.md#how-command-words-reach-main-via-argcargv) distinguish `argv[0]` from the user's path and name? What is a sensible diagnostic for a missing operand? |
| **Inspect only directory records**         | Which [`ls()`](../../user/ls.c) branch and `struct stat.type` value establish that `read()` will return directory entries?                                                                                                                                       |
| **Compare the requested basename exactly** | What does [`strcmp()`](../../user/ulib.c) require of a full-width `de.name`, and why is comparing a full path against the basename different?                                                                                                                    |
| **Report every matching name once**        | How do the four [`grade-lab-util`](../../grade-lab-util) find tests distinguish a path, a directory match, a missing match, and duplicate visits?                                                                                                                |
| **Descend without revisiting ancestors**   | Which real entries in [`kernel/sysfile.c`](../../kernel/sysfile.c) and [`user/ls.c`](../../user/ls.c) must a recursive reader recognize, and why?                                                                                                                |
| **Respect path and resource limits**       | What do [`ls()`'s buffer check](../../user/ls.c), `MAXPATH`, and `NOFILE` in [`kernel/param.h`](../../kernel/param.h) imply for long paths and open descriptors?                                                                                                 |

| Trap                                                            | Observable consequence                                                                   | Source to inspect                                                                                             |
| --------------------------------------------------------------- | ---------------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------- |
| **Treat `de.name` as an ordinary C string**                     | A 14-byte name can make `strcmp` or printing read beyond the field.                      | [`kernel/fs.h`](../../kernel/fs.h), [`user/ls.c`](../../user/ls.c)                                            |
| **Descend into `.` or `..`**                                    | The same entry can be reported repeatedly, or traversal may reach a path-length failure. | [`kernel/sysfile.c`](../../kernel/sysfile.c), corner test in [`grade-lab-util`](../../grade-lab-util)         |
| **Print only the basename**                                     | The current-directory and nested-path checks cannot recognize the full path.             | [`grade-lab-util`](../../grade-lab-util)                                                                      |
| **Treat a failed `open`, `fstat`, or `stat` as valid metadata** | An invalid descriptor or uninitialized type can misdirect the search.                    | [`user/ls.c`](../../user/ls.c), [`05-syscall-reference.md`](../05-syscall-reference.md#files-and-descriptors) |
| **Keep every ancestor descriptor open**                         | Deep searches can exhaust the per-process descriptor table.                              | `NOFILE` in [`kernel/param.h`](../../kernel/param.h), [`kernel/proc.h`](../../kernel/proc.h)                  |

## Verifying it

1. Build and boot with `make qemu` after registering `$U/_find` in the Makefile; [`02-build-boot-and-usage.md`](../02-build-boot-and-usage.md#adding-and-running-a-user-exercise) owns that build path.
2. In the guest, create the handout's `b`, `a/b`, and `a/aa/b` layout, then run `find . b`; also try an absent name, a directory name, and a search starting at `a`.
3. From the host, run `./grade-lab-util find` to select the four tests whose titles contain `find`. [`03-lab-workflow.md`](../03-lab-workflow.md) owns transcript and grading mechanics.

```bash
make qemu
```

```text
find . b
find a b
```

```bash
./grade-lab-util find
```

`make qemu` rebuilds the filesystem image, so guest files from the previous image disappear; `make qemu-fs` boots the existing image when preserving them matters. The commands above are checks to run after implementing the exercise, not a claimed transcript from this unfinished tree.

## Questions

1. Why does [`ls()`](../../user/ls.c) call `fstat(fd, &st)` before treating reads as `struct dirent` records?
2. What are the bytes of a full-length directory name in [`kernel/fs.h`](../../kernel/fs.h), and where does `ls()` add the terminator needed by C string functions?
3. Why does [`user/ls.c`](../../user/ls.c) tolerate `.` and `..`, while a traversal cannot follow them as ordinary children?
4. If `open(path, O_RDONLY)` fails, which branch in [`ls()`](../../user/ls.c) runs, and which descriptor cleanup is unnecessary?
5. If `stat(buf, &st)` fails for one entry, what does [`ls()`](../../user/ls.c) do with the rest of that directory?
6. What does the corner test in [`grade-lab-util`](../../grade-lab-util) prove about missing names, duplicate output, and directories that match?
7. Why can a 512-byte buffer in [`ls()`](../../user/ls.c) still be rejected by `MAXPATH` in [`kernel/param.h`](../../kernel/param.h)?
