# Lab util: Deriving `find`

---

[toc]

---

MIT's [2026 util lab](https://pdos.csail.mit.edu/6.1810/2026/labs/util.html#find) asks for a directory-tree search in `user/find.c`. This note connects that contract to the existing directory reader in `user/ls.c`; it stops before the new program's implementation.

> [!IMPORTANT]
> This is a derivation guide, not a solution. Its C quotation is an unchanged function from `user/ls.c`, and the decisions for `user/find.c` remain questions. The one exception is the final [Solution walkthrough](#solution-walkthrough), a marked spoiler; stop before it if you are still deriving.

## What the exercise asks for

### Contract

| Contract        | Required behavior                                                                                        |
| --------------- | -------------------------------------------------------------------------------------------------------- |
| **Input**       | A starting directory and a name to find, as in `find . b`.                                               |
| **Search**      | Visit names throughout that directory tree, including names of directories.                              |
| **Match**       | Compare the entry's name, not the whole path, with the requested name.                                   |
| **Output**      | Print the path of each match, including its starting-path prefix, as in `./a/b`.                         |
| **Termination** | Do not descend through the special `.` and `..` entries.                                                 |
| **Build**       | Put the source directly in `user/find.c` and register `$U/_find` in `UPROGS`; this tree has neither yet. |

### What the grader checks

The four tests in [`grade-lab-util`](../../grade-lab-util) total **40 points**; none installs a breakpoint. `NAME`, `DIR`, and the numbered paths below stand for freshly generated names, not literal input text.

| Test                                                       | Points | Setup and search                                                              | Required matches                                                     | Forbidden matches                 |
| ---------------------------------------------------------- | ------ | ----------------------------------------------------------------------------- | -------------------------------------------------------------------- | --------------------------------- |
| **`find, in current directory`**                           | 10     | Create `NAME` in `.`; run `find . NAME`.                                      | Prefix `./NAME`                                                      | Prefixes `./README`, `README`     |
| **`find, in sub-directory`**                               | 10     | Create the same basename in `.` and `DIR`; run `find DIR NAME`.               | Prefix `DIR/NAME`                                                    | Prefix `./NAME`                   |
| **`find, recursive`**                                      | 10     | Put `NAME` in a child, its grandchild, and another child; run `find . NAME`.  | Prefixes for all three paths                                         | None                              |
| **`find, missing target, no duplicates, directory match`** | 10     | Search for a present file, absent name, and directory name; then `echo DONE`. | Respectively 1, 0, and 1 lines matching `^\./.*NAME`; exact `^DONE$` | Prefixes `path too long`, `panic` |

### What the grader does not prove

The first three tests use [`re.match()` through `assert_lines_match()`](../../gradelib.py): their dots are unescaped wildcards and their path patterns have no end anchor. The recursive test does not count duplicate or extra lines. The corner test instead uses multiline `re.findall()` with a literal `./` prefix and exact counts; its names are still not end-anchored.

The subdirectory case observes the requested start point and rejects the root-level duplicate basename. None of these assertions establishes an output order, every pathname shape, a general cycle-detection strategy, argument diagnostics, or failure handling for `open`, `fstat`, and `stat`. A passing score establishes the listed observations, not those missing policies.

## Prerequisites

| When                | Read                                                                                                                                                                                                                                                                                                        | Ownership                                                                                                                                                                                                                        |
| ------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **Read first**      | [`02-build-boot-and-usage.md`](../02-build-boot-and-usage.md#adding-and-running-a-user-exercise)                                                                                                                                                                                                            | User-program location, arguments, xv6 C style, `UPROGS`, and image creation.                                                                                                                                                     |
| **Read first**      | [`C_String.md`](../../../c-programming/C_String.md)                                                                                                                                                                                                                                                         | The sibling study note introduces terminated strings and comparison; [`user/ulib.c`](../../user/ulib.c) supplies this tree's actual `strcmp`, `strlen`, and `memmove`.                                                           |
| **Read first**      | [`fixed_width_name_field.c`](../../../c-programming/notes/src/main/arrays_and_strings/strings/fixed_width_name_field.c) and its [`.excalidraw`](../../../c-programming/notes/src/main/arrays_and_strings/strings/fixed_width_name_field.excalidraw)                                                         | A hosted demo of a 14-byte name field: detecting a missing `\0`, bounded printing, the `DIRSIZ + 1` copy, `strcmp` versus `==`, and budgeting a joined path. The drawing opens in Excalidraw and shows each buffer byte by byte. |
| **Read first**      | [`dir_dot_entries.c`](../../../operating-system/codes/src/main/persistence/file-intro/dir_dot_entries.c) and [`dir-dot-entries-loop.svg`](../../../operating-system/images/file-systems/dir-dot-entries-loop.svg) ([source](../../../operating-system/images/file-systems/dir-dot-entries-loop.excalidraw)) | A Linux demo that proves `.` and `..` name the current and parent inodes, and contrasts Linux's `readdir()` with xv6's raw `read()` of `struct dirent`. The figure shows which records lead down and which lead back.            |
| **Read first**      | [`07-exercises.md`'s `ex9ls`](../07-exercises.md#ex9ls--a-directory-is-a-file) and [`user/ex9ls.c`](../../user/ex9ls.c)                                                                                                                                                                                     | A smaller directory reader before the fuller `user/ls.c` walkthrough below.                                                                                                                                                      |
| **Use when needed** | [`05-syscall-reference.md`](../05-syscall-reference.md#files-and-descriptors) and its [path limits](../05-syscall-reference.md#directories-and-paths)                                                                                                                                                       | `open`, `read`, `fstat`, `stat`, `close`, and pathname limits.                                                                                                                                                                   |
| **Use when needed** | [`book/ch10-file-system.md`](../book/ch10-file-system.md) and the book's [`fs.tex`](../../../xv6-riscv-book/fs.tex)                                                                                                                                                                                         | The xv6 directory and pathname mechanisms; the chapter note is still a placeholder, so the narrow record-format explanation below is held here temporarily.                                                                      |
| **Use when needed** | [`03-lab-workflow.md`](../03-lab-workflow.md) and [`06-build-artifacts.md`](../06-build-artifacts.md)                                                                                                                                                                                                       | Lab grading and generated system-call stubs; neither mechanism needs repeating here.                                                                                                                                             |
| **Use when needed** | [`fd_per_open_level.c`](../../../operating-system/codes/src/main/persistence/file-intro/fd_per_open_level.c)                                                                                                                                                                                                | A Linux demo of one descriptor per open nesting level, then `EMFILE` after lowering `RLIMIT_NOFILE` to xv6's `NOFILE` of 16.                                                                                                     |

> [!NOTE]
> The sibling C note and the three sibling demos teach hosted C and Linux concepts, not xv6's callable API. [`user/user.h`](../../user/user.h) is the complete user interface here; `opendir()` and `readdir()` are not available.

## The code to read first

| File                                                                          | Contributes                                                                                                    | Does not contribute                                                    |
| ----------------------------------------------------------------------------- | -------------------------------------------------------------------------------------------------------------- | ---------------------------------------------------------------------- |
| [`user/ls.c`](../../user/ls.c)                                                | `ls()` opens a path, checks its type, reads directory records, builds a child path, and closes the descriptor. | No recursive descent and no requested-name filter.                     |
| [`user/ex9ls.c`](../../user/ex9ls.c)                                          | `list()` exposes one fixed-size record at a time without `ls()`'s metadata formatting.                         | No full paths for entries and no recursive descent.                    |
| [`kernel/fs.h`](../../kernel/fs.h) and [`kernel/stat.h`](../../kernel/stat.h) | `struct dirent`, `DIRSIZ`, `struct stat`, and `T_DIR`.                                                         | No user-level traversal policy.                                        |
| [`user/ulib.c`](../../user/ulib.c)                                            | `stat()` delegates to `open`, `fstat`, and `close`; `strcmp()` compares terminated strings.                    | No host C library.                                                     |
| [`kernel/fs.c`](../../kernel/fs.c)                                            | `namex()` resolves path components, and `dirlookup()` resolves one name in a directory.                        | No `find` operation; the new program must choose which paths to visit. |

## `ls()` line by line

A `dirent` is one fixed-size directory-entry record from [`kernel/fs.h`](../../kernel/fs.h). The diagram follows only existing `ls()`'s directory branch after successful `open()` and `fstat()`; it adds neither recursion nor the target's name filter.

```mermaid
flowchart TD
    subgraph L["ls() reads directory entries in user/ls.c."]
        direction TB
        A{"Does the child path fit<br/>in the local buffer?"}
        A -->|No| X["ls() reports that the path is too long."]
        X --> Z["ls() closes the directory descriptor."]
        A -->|Yes| B{"Did read() return<br/>one complete dirent?"}
        B -->|No| Z
        B -->|Yes| C{"Is de.inum equal to 0?"}
        C -->|"Yes: the slot is unused."| B
        C -->|No| D["ls() builds the child path<br/>and terminates its copied name."]
        D --> E{"Did stat() obtain<br/>the child's metadata?"}
        E -->|No| F["ls() reports the metadata failure."]
        E -->|Yes| G["ls() prints the child's name<br/>and metadata."]
        F --> B
        G --> B
    end
    classDef routing fill:#f0f0ff,stroke:#66c,color:#111
    classDef processing fill:#fffff0,stroke:#cc6,color:#111
    classDef commit fill:#f0ffff,stroke:#6cc,color:#111
    classDef failure fill:#ffd9d9,stroke:#c66,color:#111
    class A,B,C,E routing
    class D,G processing
    class Z commit
    class X,F failure
```

_Blue marks decisions, yellow processes names, cyan closes the descriptor, and red reports failures._

A directory contains records rather than complete child paths. `ls()` supplies the parent-path prefix and a NUL-terminated copy of the record name before calling `stat()`. Its source distinguishes an unused slot from a failed metadata lookup: both skip output, but only the latter prints an error.

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

The book's [`fs.tex`](../../../xv6-riscv-book/fs.tex) and this tree's [`kernel/fs.c`](../../kernel/fs.c) describe `namex()` resolving paths from `/` or the process's current directory, then looking up each component. The current directory aliases `.` and `..` are real entries created by [`create()` in `kernel/sysfile.c`](../../kernel/sysfile.c). They are harmless to list, but following them repeatedly would revisit the same directories. See the [grader limits](#what-the-grader-does-not-prove) when interpreting the missing-target and duplicate checks.

![Inode boxes for ROOT, ROOT/sub, and ROOT/sub/file. Green edges lead down to children; red edges labelled . and .. lead back to the same directory and its parent.](../../../operating-system/images/file-systems/dir-dot-entries-loop.svg)

_The figure comes from the sibling Linux demo [`dir_dot_entries.c`](../../../operating-system/codes/src/main/persistence/file-intro/dir_dot_entries.c); its editable source is [`dir-dot-entries-loop.excalidraw`](../../../operating-system/images/file-systems/dir-dot-entries-loop.excalidraw). Green edges are the only ones a tree walk can follow without returning to an inode it has already visited._

Every directory, including the starting one, has both red edges. A walker that follows any record naming a directory therefore follows `.` back to itself and `..` back to its parent, so the same names repeat until a path or descriptor limit stops it. The inode numbers in the figure are what makes the loop visible: the record's name differs, but its `inum` is one already on the path.

`strcmp()` in [`user/ulib.c`](../../user/ulib.c) compares terminated strings; `==` compares pointer values. The 14-byte record field therefore needs a safe conversion before string comparison. `ls()` shows one such boundary. Its `stat()` call is a user-library wrapper, not a separate kernel primitive, and it closes the temporary descriptor it opens.

## Deriving it

**Contract:** return to the [requirements](#contract) and [grader evidence](#what-the-grader-checks); the table below identifies decisions, not a second specification.

| Decision for `user/find.c`                 | Source-backed question to settle                                                                                                                                                                                                                                 |
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

1. Register `$U/_find`, then build and boot using the [user-program workflow](../02-build-boot-and-usage.md#adding-and-running-a-user-exercise):

    ```bash
    make qemu
    ```

2. In a fresh guest image, create the handout's tree and check plain search, a restricted starting directory, a missing name, and a directory match:

    ```text
    echo > b
    mkdir a
    echo > a/b
    mkdir a/aa
    echo > a/aa/b
    find . b
    find a b
    find . absent-name
    find . aa
    ```

3. Quit QEMU with `Ctrl-a x`, then select the four tests whose titles contain `find`:

    ```bash
    ./grade-lab-util find
    ```

These are commands for the implementation to verify, not a transcript claiming a completed `user/find.c`. Interpret results using [the evidence limits](#what-the-grader-does-not-prove). [Docs 03](../03-lab-workflow.md#grading) owns failure transcripts; this set saves `xv6.out.find_curdir`, `xv6.out.find_subdir`, `xv6.out.find_recursive`, and `xv6.out.find_corner`.

`make qemu` rebuilds the image, removing files created in the earlier guest; `make qemu-fs` preserves that image when needed. [Docs 02](../02-build-boot-and-usage.md#adding-and-running-a-user-exercise) owns those mechanics. Neither the user-space directory reader nor its future recursion needs a kernel-debugger recipe.

## Questions

1. Why does [`ls()`](../../user/ls.c) call `fstat(fd, &st)` before treating reads as `struct dirent` records?

    **A:** `read()` returns raw bytes from any inode; only `st.type == T_DIR` guarantees those bytes are 16-byte `struct dirent` slots. A file's or the console's bytes would be misread as inode numbers and names. Until `fstat()` succeeds, `st` is also uninitialized, so the type test would branch on stack garbage.

2. What are the bytes of a full-length directory name in [`kernel/fs.h`](../../kernel/fs.h), and where does `ls()` add the terminator needed by C string functions?

    **A:** Two bytes of `inum`, then 14 name bytes with no `\0` among them. `ls()` copies the field with `memmove(p, de.name, DIRSIZ)` and writes `p[DIRSIZ] = 0` inside `buf`; its length check reserves that byte through the final `+ 1`.

3. Why does [`user/ls.c`](../../user/ls.c) tolerate `.` and `..`, while a traversal cannot follow them as ordinary children?

    **A:** `ls()` lists one directory and never follows a record, so `.` and `..` are only two more output lines. A traversal that follows every record naming a directory reaches the current inode through `.` and the parent through `..`, which lists the current directory again. It then repeats output until a path or descriptor limit stops it; see [`dir-dot-entries-loop.svg`](../../../operating-system/images/file-systems/dir-dot-entries-loop.svg).

4. If `open(path, O_RDONLY)` fails, which branch in [`ls()`](../../user/ls.c) runs, and which descriptor cleanup is unnecessary?

    **A:** The first `if` prints `ls: cannot open ...` on fd 2 and returns. No `close()` is needed because a failed `open()` allocated no descriptor; `close(-1)` would itself fail in `argfd()` in [`kernel/sysfile.c`](../../kernel/sysfile.c).

5. If `stat(buf, &st)` fails for one entry, what does [`ls()`](../../user/ls.c) do with the rest of that directory?

    **A:** It prints `ls: cannot stat ...` with `printf`, so on fd 1 rather than fd 2, and `continue`s with the next record. The directory descriptor stays open, and the final `close(fd)` still runs after the loop.

6. What does the corner test in [`grade-lab-util`](../../grade-lab-util) prove about missing names, duplicate output, and directories that match?

    **A:** Exactly one line matches `^\./.*PRESENT`, so the file is not reported again through `.` or `..`. No line matches the absent name, one line matches the directory name, and neither `path too long` nor `panic` appears before `DONE`. Because the names are not end-anchored, the test does not prove that a matched line ends at the name; see [what the grader does not prove](#what-the-grader-does-not-prove).

7. Why can a 512-byte buffer in [`ls()`](../../user/ls.c) still be rejected by `MAXPATH` in [`kernel/param.h`](../../kernel/param.h)?

    **A:** The buffer size is only `ls()`'s own limit. `sys_open()` in [`kernel/sysfile.c`](../../kernel/sysfile.c) copies the pathname into `char path[MAXPATH]` through `argstr()`, which fails when no `\0` appears within 128 bytes. `stat()` calls `open()`, so a 200-byte path fits `buf` but still fails there.

## Solution walkthrough

> [!CAUTION]
> **Spoiler.** This section explains the finished [`user/find.c`](../../user/find.c), which passes all four find tests. Stop here if you are still deriving it from the sections above.

The source's header comment carries the per-entry decision table and the resource table; each `Q:`/`A:` block beside the code answers one design question. This section adds the two views that need a drawing: the control flow of one call, and the memory and descriptors held across nested calls.

### One call of `find()`

```mermaid
flowchart TD
    subgraph F["find(path, name) in user/find.c"]
        direction TB
        A["find() opens path and calls fstat()."] --> B{"Did both succeed,<br/>and is path a T_DIR?"}
        B -->|No| X["find() reports on fd 2,<br/>closes any fd it opened,<br/>and returns -1."]
        B -->|Yes| C{"Do path, '/', DIRSIZ bytes,<br/>and '\0' fit in child_path?"}
        C -->|No| X
        C -->|Yes| D["find() copies path and '/'<br/>into child_path."]
        D --> E{"Did read() return<br/>one whole dirent?"}
        E -->|"No: EOF or error"| Z["find() closes fd and<br/>returns its status."]
        E -->|Yes| G{"Is de.inum equal to 0?"}
        G -->|"Yes: the slot is unused."| E
        G -->|No| H["find() copies de.name to child_name<br/>and terminates it."]
        H --> I{"Is child_name #quot;.#quot; or #quot;..#quot;?"}
        I -->|"Yes: skip, never descend."| E
        I -->|No| J{"Does child_name<br/>equal name?"}
        J -->|Yes| K["find() prints child_path."]
        J -->|No| L
        K --> L{"Does stat(child_path)<br/>report a T_DIR?"}
        L -->|"No, or stat() failed"| E
        L -->|Yes| R["find() calls find(child_path, name),<br/>still holding its own fd."]
        R -->|"The nested call returns."| E
    end
    classDef request fill:#fff0f0,stroke:#c66,color:#111
    classDef routing fill:#f0f0ff,stroke:#66c,color:#111
    classDef processing fill:#fffff0,stroke:#cc6,color:#111
    classDef commit fill:#f0ffff,stroke:#6cc,color:#111
    classDef failure fill:#ffd9d9,stroke:#c66,color:#111
    class A,R request
    class B,C,E,G,I,J,L routing
    class D,H processing
    class K,Z commit
    class X failure
```

_Pink opens or descends, blue decides, yellow builds the path, cyan prints or closes, and red reports a failure. This palette matches the `ls()` diagram above, with pink added for requests._

Compare it with the [`ls()` diagram](#ls-line-by-line): the loop shape is identical, and the differences are exactly the three decisions the [contract](#contract) adds. The `.`/`..` test sits before the match test, so `find . ..` prints nothing and neither entry is ever descended into. The match test reads `child_name`, the bare name, while the print reads `child_path`, the whole path; that split is what the [`find, in current directory`](#what-the-grader-checks) test's `./NAME` prefix observes. A matching directory reaches the `stat()` decision like any other entry, so it is printed and then searched, which the corner test's directory-match count requires.

A `stat()` failure is reported and marks the return status, but the loop continues with the next record. One unreadable child therefore does not hide the rest of the tree, and `main()` still exits 1 at the end.

### Memory and descriptors at the deepest point

![Three stack frames for find(".", "b"), find("./a", "b"), and find("./a/aa", "b"). Each frame shows its child_path bytes, where child_name points, and the fd it holds. Beside them, p->ofile[] shows fds 0 to 2 for the console, fds 3 to 5 held by the three frames, and fd 6 opened briefly by stat().](fig/util-find-recursion-state.svg)

_[View the SVG](fig/util-find-recursion-state.svg) or open [the editable Excalidraw drawing](fig/util-find-recursion-state.excalidraw) in Excalidraw. It shows `find . b` on the handout's tree at the moment it prints `./a/aa/b`._

Each active call owns one `child_path[MAXPATH]` on the stack and one directory descriptor. Neither can be released before the nested call returns, because the caller still has records left to read and xv6 has no `lseek()` to reopen and resume. The figure's three frames hold fds 3, 4, and 5; `stat()` adds a fourth only for the instant between its own `open()` and `close()`.

| Resource per active call  | Limit in this tree                                  | What hits it first                                                              |
| ------------------------- | --------------------------------------------------- | ------------------------------------------------------------------------------- |
| **Directory descriptor**  | `NOFILE` 16 in [`kernel/param.h`](../../kernel/param.h), 13 after fds 0–2 | About 12 nested directories, leaving one slot for `stat()`'s temporary fd.      |
| **`child_path` on stack** | `USERSTACK` 2 pages under `LAB_UTIL`                | Not reached first: 13 frames of roughly 200 bytes fit well inside 8 KB.         |
| **Path length**           | `MAXPATH` 128, enforced by `argstr()` in `sys_open()` | A long path is rejected by the length check before `open()` or `stat()` sees it. |

Sizing `child_path` at `MAXPATH` rather than `ls()`'s 512 bytes is what keeps the stack row harmless. A 512-byte buffer would buy nothing, because the kernel rejects any path that long at `open()`.

### Decisions, answered

This table answers the [Deriving it](#deriving-it) checklist row by row; the source comments hold the longer reasoning.

| Decision                                   | What `user/find.c` does                                                                                                       |
| ------------------------------------------ | ----------------------------------------------------------------------------------------------------------------------------- |
| **Interpret the two operands**             | `main()` requires `argc == 3`, prints `Usage: find <dir> <name>` on fd 2 otherwise, and passes `argv[1]` and `argv[2]`.       |
| **Inspect only directory records**         | The starting path must be `T_DIR`; children are passed to `find()` only after `stat()` reports `T_DIR`.                       |
| **Compare the requested basename exactly** | `memmove()` copies `DIRSIZ` bytes into `child_name`, a `'\0'` follows, and `strcmp(child_name, name)` decides the match.      |
| **Report every matching name once**        | Each record is tested once, and skipping `.` and `..` means no directory is entered twice.                                    |
| **Descend without revisiting ancestors**   | `.` and `..` are skipped before both the match and the descent.                                                               |
| **Respect path and resource limits**       | The `DIRSIZ` budget check rejects too-long prefixes; recursion holds one fd per level, bounded by `NOFILE`.                   |
