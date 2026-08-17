# Lecture Exercises

---

[toc]

---

The small programs the 6.1810 lectures hand out, one section each: what the exercise does, what its observable behaviour is, and where every idea it introduces lives in this tree.

> [!NOTE]
> This file owns the exercises themselves. The mechanics they all share — how a `.c` file under `user/` becomes an xv6 command, why the tree stays flat, and the house style a new program has to match — belong to [02-build-boot-and-usage.md](02-build-boot-and-usage.md#adding-and-running-a-user-exercise), and are not repeated here.

| Exercise                                              | Source           | From                                                                              | What it introduces                                                 |
| ----------------------------------------------------- | ---------------- | --------------------------------------------------------------------------------- | ------------------------------------------------------------------ |
| **[`ex1copy`](#ex1copy--the-lecture-1-input-filter)** | `user/ex1copy.c` | [Lecture 1, `ex1.c`](https://pdos.csail.mit.edu/6.1810/2026/lec/l-overview/ex1.c) | `read`, `write`, descriptors 0/1/2, EOF, and what a Unix filter is |

## `ex1copy` — the lecture 1 input filter

The official [Lecture 1 example](https://pdos.csail.mit.edu/6.1810/2026/lec/l-overview/ex1.c) is a filter: it copies every byte from descriptor 0 to descriptor 1 until `read()` returns 0 for EOF. It does not stop after one line, and it adds nothing to the data it copies.

### Waiting is part of the interface

Interactively, type a line, press Enter, and then press `Ctrl-d` at the next empty input position:

```text
$ ex1copy
ex1copy: copying standard input to standard output.
ex1copy: type a line and press Enter. Ctrl-d on an empty line finishes.
hello
hello
$
```

The first `hello` is the console echoing the keys you typed. The second is `ex1copy` writing the bytes returned by `read()`. The program then calls `read()` again, and `consoleread()` in `kernel/console.c` hands back at most one line per call — so once the typed input is drained, that read sleeps in the kernel. Pressing `Ctrl-d` there makes it return 0, which ends the loop and restores the shell prompt. The wait is expected input-filter behaviour, not a deadlock.

A finite pipeline supplies EOF automatically when its writer exits, and the two hint lines are gone:

```text
$ echo hello | ex1copy
hello
$
```

That difference is the point. A filter must not add bytes to its output, so the hint is bound by two rules that the program enforces itself:

| Rule                                        | Mechanism                                                                                                                                                              |
| ------------------------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **It never travels with the data**          | The hint is written to descriptor 2, never 1, so `ex1copy > out` and `ex1copy \| wc` see only copied bytes.                                                            |
| **It only appears when a human is waiting** | `fstat(0, &st)` decides: `T_DEVICE` means the console, so print it; `T_FILE` means a redirect, so stay quiet; and a pipe makes `fstat()` fail outright, so stay quiet. |

That last case is not a special case in `ex1copy` — `filestat()` in `kernel/file.c` serves `FD_INODE` and `FD_DEVICE` and returns -1 for `FD_PIPE`, so one `fstat()` call separates all three sources for free.

> [!NOTE]
> Without the hint, an interactive run is visually identical to a hung program: no prompt, no output, no cursor movement. That is the single most common first-encounter confusion with Unix filters, and it is why [02's troubleshooting table](02-build-boot-and-usage.md#when-things-go-wrong) still carries a row for it — `cat` with no arguments behaves exactly the same way and says nothing at all.

### What lecture 1 uses it to teach

Twenty lines of C carry most of the Unix I/O model. The commentary block at the top of `user/ex1copy.c` is the long form; this is the map from each teaching point to the code in this tree that implements it.

| Teaching point                                                                                                 | Where it lives here                                                                                                                                                                      |
| -------------------------------------------------------------------------------------------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **`read()` and `write()` look like function calls but jump into the kernel, preserving user/kernel isolation** | `user/usys.S` loads a syscall number into `a7` and executes `ecall`; `kernel/syscall.c` dispatches to `sys_read` and `sys_write` in `kernel/sysfile.c`.                                  |
| **The first argument is a file descriptor, naming an already-open file**                                       | Resolved through `p->ofile[NOFILE]` in `kernel/proc.h`. `NOFILE` is 16, so one process can hold many descriptors.                                                                        |
| **An FD may be a file, a pipe, a device, or the console**                                                      | `struct file` carries a type tag; `kernel/file.c` `fileread()` branches on it, so the caller's code is identical for all four.                                                           |
| **FD 0 is standard input and FD 1 is standard output, by convention**                                          | `user/sh.c` arranges the descriptors before `exec`, and `user/init.c` guarantees 0, 1, and 2 exist at boot.                                                                              |
| **The second and third arguments are a destination address and a maximum byte count**                          | `read()` may return fewer bytes than requested and never more, which is why the program writes `n` and not `sizeof(buf)`.                                                                |
| **The return value is a byte count, `0` for EOF, or `-1` for an error**                                        | All three outcomes appear in the loop: positive drives the copy, zero exits it, negative reaches the `read error` branch.                                                                |
| **Unix I/O is untyped 8-bit bytes**                                                                            | Nothing in `ex1copy` inspects `buf`. Interpreting the bytes as text, a record, or an image is the application's job, never the kernel's.                                                 |
| **A program can still ask what kind of thing a descriptor names**                                              | `fstat(0, &st)` reports `T_DEVICE` for the console and `T_FILE` for a redirect, and fails for a pipe. `ex1copy` uses it for one decision only: whether a human is sitting there waiting. |

The lecture ends on an open question — how do you make a _new_ file descriptor? — and the answer is `open()`, `pipe()`, and `dup()` in `user/user.h`. `user/sh.c` builds every redirection and pipeline out of those three plus `close()`, resting entirely on `fdalloc()`'s lowest-unused-descriptor rule; see [book/ch01](book/ch01-operating-system-interfaces.md#descriptors-two-levels-of-indirection).

### Regression tests

[`grade-ex1copy`](../grade-ex1copy) holds three tests, each pinning one of the behaviours above:

| Test                                     | What it asserts                                                                                                                                                        |
| ---------------------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **`copies finite piped input`**          | A 71-byte payload survives `echo … \| ex1copy` byte for byte. It deliberately exceeds `buf[64]`, so the copy takes two loop iterations rather than one.                |
| **`interactive input ends with Ctrl-d`** | Each typed line appears exactly twice — console echo, then the copy — and `\x04` at an empty position lets the shell go on to run the next command.                    |
| **`reports a closed output pipe`**       | With a reader that exits first, `write()` returns -1 and the program prints `ex1copy: write error` and exits, instead of looping back into `read()` and waiting there. |

```bash
./grade-ex1copy                          # all three
./grade-ex1copy "closed output pipe"     # one, by name
```

The harness itself — how it boots QEMU, drives the shell, and pattern-matches the transcript — is described in [03-lab-workflow.md](03-lab-workflow.md#grading).
