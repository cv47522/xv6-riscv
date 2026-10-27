# Lab util: Deriving shell command history

---

[toc]

---

The shell challenge in MIT's [2026 util lab](https://pdos.csail.mit.edu/6.1810/2026/labs/util.html) suggests "modify the shell to keep a history of past shell commands", rated moderate. Keeping the list is easy; the moderate part is deciding exactly which bytes a "command" is, which process owns the list, and how a recalled command re-enters the shell. This note explains the existing input path those decisions depend on.

> [!IMPORTANT]
> No solution or pseudocode. The C quotation is the unchanged `gets()` from `user/ulib.c`, which is not a target. `user/sh.c` is described from its unmodified state; [the prompt note](08-util-sh-prompt.md) records that rule.

## What the exercise asks for

### Contract

| Aspect                | Requirement or local convention                                                                                                                                           |
| --------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **Target**            | Handout: modify `user/sh.c`. No new program or Makefile change is required.                                                                                               |
| **Behavior**          | Handout: keep a history of past shell commands. Conventional minimum: a way to list them. Re-running one, such as `!!` or `!n`, is a common extension, not a requirement. |
| **Lifetime**          | Not specified. In memory for one shell process, or persisted in a file, is your decision.                                                                                 |
| **What is recorded**  | Not specified. Blank lines, `cd`, the listing command itself, and script input are each a policy choice.                                                                  |
| **Arrow-key recall**  | Not required. It needs per-key console input, which is a kernel change; see [the tab-completion note](10-util-sh-tab-completion.md#the-concept-underneath).               |
| **Existing behavior** | `cd`, parsing, redirection, pipes, background jobs, and exit at end of input keep working.                                                                                |
| **Prompt**            | Required by the grader: keep the console prompt exactly `$ `. See [the harness dependency](08-util-sh-prompt.md#the-harness-depends-on-the-prompt).                       |

### What the grader checks

> [!NOTE]
> **No test.** No test in [`grade-lab-util`](../../grade-lab-util) lists or recalls commands. The suite types ordinary commands only, so it is a regression check that recording does not disturb execution or the prompt. A command name you choose for listing must not collide with a test's command; none uses `history`.

## Prerequisites

| When                | Read                                                                                                               | Why it matters here                                                                   |
| ------------------- | ------------------------------------------------------------------------------------------------------------------ | ------------------------------------------------------------------------------------- |
| **Read first**      | [`08-util-sh-prompt.md`](08-util-sh-prompt.md)                                                                     | `getcmd()`, the input-source test, and the grader's reliance on `$ `.                 |
| **Read first**      | [`09-util-sh-wait.md`](09-util-sh-wait.md#main-and-the-back-case-line-by-line)                                     | `main()` quoted in full, the `cd` builtin, and why some commands must not fork.       |
| **Read first**      | [`book/ch01-operating-system-interfaces.md`](../book/ch01-operating-system-interfaces.md#creating-a-process-kfork) | `kfork()` copies the parent's memory, which decides what a child can see.             |
| **Use when needed** | [`05-syscall-reference.md`](../05-syscall-reference.md#memory)                                                     | `sbrk`, and therefore what `malloc()` in `user/umalloc.c` can grow into.              |
| **Use when needed** | [`10-util-sh-tab-completion.md`](10-util-sh-tab-completion.md#consoleintr-line-by-line)                            | The console driver, which holds a line until Enter and echoes every byte it receives. |

## The code to read first

| File                                         | Contributes                                                                                                  | Does not contribute                                           |
| -------------------------------------------- | ------------------------------------------------------------------------------------------------------------ | ------------------------------------------------------------- |
| [`user/ulib.c`](../../user/ulib.c)           | `gets()`, quoted below: exactly what one command line contains.                                              | No editing or recall.                                         |
| [`user/sh.c`](../../user/sh.c)               | `main()`'s loop, the `cd` builtin, and `parsecmd()` with `nulterminate()`, which rewrites the line in place. | No storage for past lines; `buf` is reused for every command. |
| [`user/umalloc.c`](../../user/umalloc.c)     | `malloc()` and `free()`, built on `sbrk`.                                                                    | No `realloc()` or `strdup()`.                                 |
| [`user/user.h`](../../user/user.h)           | `strcpy`, `strlen`, `strcmp`, `memmove`, `atoi`, and `printf`: the string tools available.                   | No `strncpy` in user space, no `snprintf`.                    |
| [`kernel/console.c`](../../kernel/console.c) | `consoleintr()` echoes and stores every typed byte, including an arrow key's escape sequence.                | No key decoding.                                              |

## `gets()` line by line

This is the unchanged function from [`user/ulib.c`](../../user/ulib.c), which `getcmd()` calls with `sizeof(buf)`, 100:

```c
gets(char *buf, int max)
{
  int i, cc;
  char c;

  for (i = 0; i + 1 < max;) {
    cc = read(0, &c, 1);
    if (cc < 1)
      break;
    buf[i++] = c;
    if (c == '\n' || c == '\r')
      break;
  }
  buf[i] = '\0';
  return buf;
}
```

| Line or case                       | Consequence for a recorded line                                                                            |
| ---------------------------------- | ---------------------------------------------------------------------------------------------------------- |
| **`i + 1 < max`**                  | At most 99 bytes are stored, leaving room for the terminator.                                              |
| **`buf[i++] = c` before the test** | The newline is part of the stored line. A typical entry ends in `\n` followed by `\0`.                     |
| **Loop ends at 99 bytes**          | A longer line is split: the first 99 bytes have no newline, and the remainder arrives as the next command. |
| **`cc < 1`**                       | End of input or an error. `getcmd()` treats an empty result as end of input.                               |
| **`buf[i] = '\0'`**                | The line is always a C string, whatever ended it.                                                          |

`gets()` reads one byte per system call, but on the console every byte of the line has already been released together by `consoleread()`, so the loop does not wait between characters. From a file, as in `sh < script`, the same loop reads the file one byte at a time.

## The concept underneath

The shell's single `buf` is written by several pieces of code, in two processes:

| Writer                                | Process                   | What it does to the line                                                              |
| ------------------------------------- | ------------------------- | ------------------------------------------------------------------------------------- |
| **`memset()` in `getcmd()`**          | The shell                 | Clears all 100 bytes before each read.                                                |
| **`gets()`**                          | The shell                 | Stores the new line and its terminator.                                               |
| **The `cd` branch of `main()`**       | The shell                 | Overwrites the last byte, normally the newline, with `0`.                             |
| **`nulterminate()` via `parsecmd()`** | The child after `fork1()` | Writes `0` at the end of every word, so the child's copy no longer reads as one line. |

> [!IMPORTANT]
> **`buf` holds only the current command.** The next `getcmd()` clears it, so a stored pointer into `buf` sees the next command, or nothing. History must copy the bytes, and the copy must be taken before any writer above changes them.

`kfork()` gives the child a copy of the parent's whole user memory, so `nulterminate()` changes only the child's copy; the shell's `buf` is intact after `wait(0)` returns. The same copy rule cuts the other way: a child can read the history as it stood at the fork, but nothing a child adds survives in the shell. That is why the processes in [the `wait` note's `main()`](09-util-sh-wait.md#main-and-the-back-case-line-by-line) matter here too.

| Input event                     | What reaches the shell                                                                                  | Source                                                                                        |
| ------------------------------- | ------------------------------------------------------------------------------------------------------- | --------------------------------------------------------------------------------------------- |
| **Typed line and Enter**        | The whole line, ending in `\n`, after Enter                                                             | `consoleintr()`, `consoleread()`                                                              |
| **Up arrow, then Enter**        | Typically the escape bytes `ESC`, `[`, `A`, then `\n`; their echo has already moved the terminal cursor | `consoleintr()` `default` case                                                                |
| **`Ctrl-d` at an empty prompt** | A zero-byte read: the shell exits and `init` starts a new one with empty memory                         | [docs 02](../02-build-boot-and-usage.md#xv6-console-keys), [`user/init.c`](../../user/init.c) |

## Deriving it

**Contract:** use the [requirements above](#contract); settle each question from the named source.

| Decision for `user/sh.c`           | Source-backed question to settle                                                                                                                                                   |
| ---------------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **Define one entry**               | Which bytes does `gets()` store, and will an entry keep or drop the newline? What happens to a 99-byte prefix that has none?                                                       |
| **Copy at the right moment**       | Which writers in [the table above](#the-concept-underneath) run before your copy, and does the `cd` branch's chop affect what you record?                                          |
| **Bound the storage**              | Will you keep a fixed number of 100-byte slots, or `malloc()` each line? What happens at the limit: drop the oldest, or stop recording?                                            |
| **Choose what is recorded**        | Blank lines, `cd`, the listing command, a recalled command, and lines from `sh < script`: which belong in the history?                                                             |
| **List it from the right process** | Reading history works in a forked child, because `kfork()` copied it. Would running the listing through `runcmd()` let `history > file` or `history \| grep x` work?               |
| **Re-run from the right process**  | If a recalled command is `cd d`, which process must see its text before any fork, and how does it reach the same dispatch as a typed line?                                         |
| **Persist or not**                 | Should history survive `Ctrl-d` and the replacement shell from `init`? If it is written to a file, in which directory, given that the shell's current directory changes with `cd`? |

| Trap                                                  | Observable consequence                                                                                               | Source to inspect                                                                                  |
| ----------------------------------------------------- | -------------------------------------------------------------------------------------------------------------------- | -------------------------------------------------------------------------------------------------- |
| **Storing a pointer to `buf`**                        | Every entry shows the latest command, or an empty string after `memset()`.                                           | `getcmd()` in [`user/sh.c`](../../user/sh.c)                                                       |
| **Recording inside the forked child**                 | Entries vanish when the child exits; the shell's own list never grows.                                               | [`book/ch01`](../book/ch01-operating-system-interfaces.md#creating-a-process-kfork)                |
| **Recording after `parsecmd()` in the shell process** | `nulterminate()` has split the line, so the entry ends at the first word.                                            | `nulterminate()` in [`user/sh.c`](../../user/sh.c)                                                 |
| **Re-running a recalled `cd` through `fork1()`**      | The child execs a program named `cd`, which does not exist: `exec cd failed` appears and the directory is unchanged. | `main()` in [`user/sh.c`](../../user/sh.c)                                                         |
| **Copying with `strcpy` into a too-small slot**       | Adjacent entries are overwritten; there is no bounded copy in `user/user.h`.                                         | [`user/user.h`](../../user/user.h)                                                                 |
| **Implementing arrow keys in user space only**        | The escape bytes arrive after Enter, and their echo has already moved the cursor up a line.                          | [`10-util-sh-tab-completion.md`](10-util-sh-tab-completion.md#consoleintr-line-by-line)            |
| **Changing the prompt to show an entry number**       | The grader waits for `$ ` and splits output on `$ command`; any other prompt text breaks the suite.                  | [`08-util-sh-prompt.md`](08-util-sh-prompt.md#the-harness-depends-on-the-prompt)                   |
| **Writing a history file after every command**        | Each write is a file-system transaction, and a relative file name lands in whatever directory `cd` last chose.       | [`book/ch11-logging.md`](../book/ch11-logging.md), `namex()` in [`kernel/fs.c`](../../kernel/fs.c) |

## Verifying it

1. Build and boot:

    ```bash
    make qemu
    ```

2. In the guest, exercise ordinary commands, a builtin, a blank line, a pipe, and recall if you implemented it. Replace `history` with your command's name:

    ```text
    echo one
    mkdir d
    cd d

    echo two | wc
    history
    ```

    Then check the boundaries: a line longer than 99 bytes, more lines than your storage bound, `sh < ../findtest.sh` if you chose not to record scripts, and `Ctrl-d` followed by `history` in the replacement shell. Press the up arrow and Enter once to see why arrow recall is out of scope without a kernel change.

3. Quit QEMU with `Ctrl-a x`, then run the suite as the execution and prompt regression check:

    ```bash
    make grade
    ```

These are checks to run, not a transcript. `findtest.sh` lives in `/`, so after `cd d` it is `../findtest.sh`. No kernel-debugger recipe is needed; history is user-space state, and `printf` from the shell shows it directly.

## Questions

1. Does a line stored by `gets()` normally end with `\n`, `\0`, or both? What changes for a line of exactly 120 typed characters?
2. Which code in [`user/sh.c`](../../user/sh.c) overwrites a byte of `buf` in the shell process itself, and for which command?
3. Why does `nulterminate()` not damage the shell's copy of the line, according to how `kfork()` creates the child?
4. A listing command run in a forked child prints the right entries. Why does an entry added by that child not appear in the next listing?
5. Failure scenario: history stores `buf` pointers. What does every entry print after three commands?
6. Failure scenario: `!n` recalls `cd d`, and the recalled text is passed straight to `fork1()` and `runcmd()`. What is the current directory afterwards, and what message, if any, appears?
7. What bytes does the shell receive when you press the up arrow and then Enter, and why has the terminal display already changed before the shell runs?
8. After `Ctrl-d`, which process starts the next shell, and what does an in-memory history contain in it?
