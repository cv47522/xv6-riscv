# xv6 lecture exercises: program behavior and regression tests

---

[toc]

---

The small programs the 6.1810 lectures hand out, one section each: what the exercise does, what its observable behaviour is, and where every idea it introduces lives in this tree.

> [!NOTE]
> This file owns the exercises themselves. The mechanics they all share — how a `.c` file under `user/` becomes an xv6 command, why the tree stays flat, and the house style a new program has to match — belong to [02-build-boot-and-usage.md](02-build-boot-and-usage.md#adding-and-running-a-user-exercise), and are not repeated here. To write the next one, use [`prompts/add-lecture-exercise.md`](prompts/add-lecture-exercise.md), which encodes the whole procedure; in Claude Code it is wired up as `/exercise`.

## Lecture 1 program map

Lecture 1 ([`l-overview.txt`](../lectures/l-overview.txt)) walks through nine example programs. Each is named `ex<N><verb>`, where `N` is the lecture's own numbering and the verb is the lecture's own wording — so `ex5forkexec` is lecture 1's `ex5.c`, described there as "fork() a new process, exec() a program".

| Exercise                                                      | Source               | Lecture                                                                | What it introduces                                                            |
| ------------------------------------------------------------- | -------------------- | ---------------------------------------------------------------------- | ----------------------------------------------------------------------------- |
| **[`ex1copy`](#ex1copy--the-lecture-1-input-filter)**         | `user/ex1copy.c`     | [`ex1.c`](https://pdos.csail.mit.edu/6.1810/2026/lec/l-overview/ex1.c) | `read`, `write`, descriptors 0/1/2, EOF, and what a Unix filter is            |
| **[`ex2create`](#ex2create--making-a-descriptor)**            | `user/ex2create.c`   | [`ex2.c`](https://pdos.csail.mit.edu/6.1810/2026/lec/l-overview/ex2.c) | `open`, the lowest-unused-descriptor rule, per-process descriptor name-spaces |
| **[`ex3fork`](#ex3fork--one-call-two-returns)**               | `user/ex3fork.c`     | [`ex3.c`](https://pdos.csail.mit.edu/6.1810/2026/lec/l-overview/ex3.c) | `fork`, parent/child, separate memory, and unordered output                   |
| **[`ex4exec`](#ex4exec--becoming-another-program)**           | `user/ex4exec.c`     | [`ex4.c`](https://pdos.csail.mit.edu/6.1810/2026/lec/l-overview/ex4.c) | `exec`, the argument array, and what survives an image replacement            |
| **[`ex5forkexec`](#ex5forkexec--the-shells-execution-model)** | `user/ex5forkexec.c` | [`ex5.c`](https://pdos.csail.mit.edu/6.1810/2026/lec/l-overview/ex5.c) | `wait`, exit status, and why `fork` and `exec` are two calls                  |
| **[`ex6redirect`](#ex6redirect--how--is-implemented)**        | `user/ex6redirect.c` | [`ex6.c`](https://pdos.csail.mit.edu/6.1810/2026/lec/l-overview/ex6.c) | `close` + `open` in the fork/exec window; redirection lives only in `sh`      |
| **[`ex7pipe`](#ex7pipe--a-buffer-with-two-descriptors)**      | `user/ex7pipe.c`     | [`ex7.c`](https://pdos.csail.mit.edu/6.1810/2026/lec/l-overview/ex7.c) | `pipe`, the kernel buffer, and why one process alone is a trap                |
| **[`ex8pipefork`](#ex8pipefork--two-processes-one-buffer)**   | `user/ex8pipefork.c` | [`ex8.c`](https://pdos.csail.mit.edu/6.1810/2026/lec/l-overview/ex8.c) | `pipe` + `fork`, and why EOF depends on closing unused ends                   |
| **[`ex9ls`](#ex9ls--a-directory-is-a-file)**                  | `user/ex9ls.c`       | [`ex9.c`](https://pdos.csail.mit.edu/6.1810/2026/lec/l-overview/ex9.c) | `struct dirent`, reading a directory with plain `read`                        |

They are not nine independent programs. Each one answers a question the previous one leaves open, and the last three are the shell's implementation taken apart:

```mermaid
flowchart TD
    ex1["ex1copy<br/>read/write, fd 0 and 1"] --> ex2["ex2create<br/>open makes a new fd"]
    ex2 --> ex6
    ex1 --> ex3["ex3fork<br/>a second process"]
    ex3 --> ex5["ex5forkexec<br/>fork + exec + wait"]
    ex4["ex4exec<br/>a different program"] --> ex5
    ex5 --> ex6["ex6redirect<br/>close + open, then exec"]
    ex5 --> ex8
    ex7["ex7pipe<br/>a kernel buffer"] --> ex8["ex8pipefork<br/>two processes, one pipe"]
    ex6 --> sh["user/sh.c<br/>redirection and pipelines"]
    ex8 --> sh
    ex2 --> ex9["ex9ls<br/>directories are files"]

    classDef io fill:#f0fff0,stroke:#4a4
    classDef proc fill:#fff0f0,stroke:#a44
    classDef combine fill:#f0f0ff,stroke:#44a
    classDef target fill:#ffffd0,stroke:#aa4
    class ex1,ex2,ex9 io
    class ex3,ex4,ex7 proc
    class ex5,ex6,ex8 combine
    class sh target
```

Green is I/O, red is process and buffer creation, blue is the combinations that need both, and yellow is where the whole thing was going. Reading `user/sh.c` after `ex8pipefork` is the intended payoff — every construct in it has appeared by then.

> [!TIP]
> Run them in order in one session. Several depend on files an earlier one wrote: `ex9ls` shows the `ex2.out` and `ex6.out` that `ex2create` and `ex6redirect` leave behind, which is a cheap way to see that they really did touch the file system. `make qemu` rebuilds `fs.img` and discards them; `make qemu-fs` keeps them.

## `ex1copy` — the lecture 1 input filter

The official [Lecture 1 example](https://pdos.csail.mit.edu/6.1810/2026/lec/l-overview/ex1.c) is a filter: it copies every byte from descriptor 0 to descriptor 1 until `read()` returns 0 for EOF. It does not stop after one line, and it adds nothing to the data it copies.

### Interactive input: echo, blocking, and Ctrl-d

Interactively, type a line, press Enter, and then press `Ctrl-d` at the next empty input position:

```text
$ ex1copy
ex1copy: copying standard input to standard output.
ex1copy: type a line and press Enter. Ctrl-d on an empty line finishes.
hello
hello
$
```

The two copies of `hello` come from different places:

1. **Console echo** displays the keys you typed.
2. **ex1copy output** writes the bytes returned by `read()`.
3. **The next read waits** once that input is drained: `consoleread()` in `kernel/console.c` returns at most one line per call.
4. **Ctrl-d at the empty input position** makes `read()` return 0, ends the loop, and restores the shell prompt.

That wait is expected input-filter behaviour, not a **deadlock**.

A finite pipeline supplies EOF automatically when its writer exits, and the two hint lines are gone:

```text
$ echo hello | ex1copy
hello
$
```

That difference is the point. A filter **must not** add bytes to its output, so the hint is bound by two rules that the program enforces itself:

| Rule                                        | Mechanism                                                                                                                                                              |
| ------------------------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **It never travels with the data**          | The hint is written to descriptor 2, **never** 1, so `ex1copy > out` and `ex1copy \| wc` see only copied bytes.                                                        |
| **It only appears when a human is waiting** | `fstat(0, &st)` decides: `T_DEVICE` means the console, so print it; `T_FILE` means a redirect, so stay quiet; and a pipe makes `fstat()` fail outright, so stay quiet. |

That last case is not a special case in `ex1copy` — `filestat()` in `kernel/file.c` serves `FD_INODE` and `FD_DEVICE` and returns -1 for `FD_PIPE`, so one `fstat()` call separates all three sources for free.

> [!NOTE]
> Without the hint, an interactive run is visually identical to a hung program: no prompt, no output, no cursor movement. That is the single most common first-encounter confusion with Unix filters, and it is why [02's troubleshooting table](02-build-boot-and-usage.md#troubleshoot-build-boot-and-runtime-failures) still carries a row for it — `cat` with no arguments behaves exactly the same way and says nothing at all.

### ex1copy: Unix I/O concepts and implementing code

Twenty lines of C carry most of the Unix I/O model. The commentary block at the top of `user/ex1copy.c` is the long form; this is the map from each teaching point to the code in this tree that implements it.

| Teaching point                                                                                                 | Where it lives here                                                                                                                                                                      |
| -------------------------------------------------------------------------------------------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **`read()` and `write()` look like function calls but jump into the kernel, preserving user/kernel isolation** | `user/usys.S` loads a syscall number into `a7` and executes `ecall`; `kernel/syscall.c` dispatches to `sys_read` and `sys_write` in `kernel/sysfile.c`.                                  |
| **The first argument is a file descriptor, naming an already-open file**                                       | Resolved through `p->ofile[NOFILE]` in `kernel/proc.h`. `NOFILE` is 16, so one process can hold many descriptors.                                                                        |
| **An FD may be a file, a pipe, a device, or the console**                                                      | `struct file` carries a type tag; `kernel/file.c` `fileread()` branches on it, so the caller's code is identical for all four.                                                           |
| **FD 0 is standard input and FD 1 is standard output, by convention**                                          | `user/sh.c` arranges the descriptors before `exec`, and `user/init.c` guarantees 0, 1, and 2 exist at boot.                                                                              |
| **The second and third arguments are a destination address and a maximum byte count**                          | `read()` may return fewer bytes than requested and **never** more, which is why the program writes `n` and not `sizeof(buf)`.                                                            |
| **The return value is a byte count, `0` for EOF, or `-1` for an error**                                        | All three outcomes appear in the loop: positive drives the copy, zero exits it, negative reaches the `read error` branch.                                                                |
| **Unix I/O is untyped 8-bit bytes**                                                                            | Nothing in `ex1copy` inspects `buf`. Interpreting the bytes as text, a record, or an image is the application's job, **never** the kernel's.                                             |
| **A program can still ask what kind of thing a descriptor names**                                              | `fstat(0, &st)` reports `T_DEVICE` for the console and `T_FILE` for a redirect, and fails for a pipe. `ex1copy` uses it for one decision only: whether a human is sitting there waiting. |

The lecture ends on an open question — how do you make a _new_ file descriptor? — and the answer is `open()`, `pipe()`, and `dup()` in `user/user.h`. `user/sh.c` builds every redirection and pipeline out of those three plus `close()`, resting entirely on `fdalloc()`'s lowest-unused-descriptor rule; see [book/ch01](book/ch01-operating-system-interfaces.md#descriptors-two-levels-of-indirection).

### ex1copy: input detection, diagnostics, and EOF questions

Five things in `user/ex1copy.c` look arbitrary until you follow them into the kernel. The source carries the same answers inline, next to the code each one explains.

#### Why `fstat(0, …)` rather than `fstat(1, …)`?

**The hint depends on the input side:** is a human about to type? Descriptor 1 cannot answer that question:

| Invocation           | Descriptor 1 | Human waiting at input? |
| -------------------- | ------------ | ----------------------- |
| `ex1copy > out`      | File         | Yes                     |
| `echo hi \| ex1copy` | Console      | No                      |

`fstat()` takes a descriptor rather than a path, so `fstat(0, …)` reports on whatever `sh` wired to standard input before `exec`.

#### What counts as `T_DEVICE` besides the console? Is the display separate?

**The console is this tree's only device file, and one inode covers both directions.** Two source facts establish that:

- **Creation:** `sys_mknod()` in `kernel/sysfile.c` stamps the `T_DEVICE` inode type. The tree's only `mknod()` call is `mknod("console", CONSOLE, 0)` in `user/init.c`.
- **Input and output:** `consoleinit()` registers `devsw[CONSOLE].read = consoleread` and `devsw[CONSOLE].write = consolewrite` over the same UART. Keyboard and display therefore share the inode with major number `CONSOLE`.

| Symbol     | Defined in       | What it actually bounds                                                         |
| ---------- | ---------------- | ------------------------------------------------------------------------------- |
| `T_DEVICE` | `kernel/stat.h`  | inode type `3`, written only by `mknod()`, never by `create()` for a plain file |
| `CONSOLE`  | `kernel/file.h`  | major number `1`, the only major number anything claims                         |
| `NDEV`     | `kernel/param.h` | size of `devsw[]` — room for ten majors, nine of which stay empty               |

> [!NOTE]
> On Linux the same test would pass for every character device — `/dev/null`, a disk, any tty — which is why portable filters call `isatty()` instead of inspecting the file type. xv6 has no `isatty()`, and with a single device file it does not need one.

#### Why write the hint to descriptor 2 when it is not an error?

**Descriptor 2 carries diagnostics about the run:** errors, progress, prompts, and this hint. It stays pointed at the terminal when `>` moves descriptor 1 elsewhere.

The filter rule forces that choice: `ex1copy > out` and `ex1copy | wc` **must** see the input bytes and nothing more. `sh` redirects only the descriptor you name.

#### Why `fprintf(2, …)` rather than `printf(…)`?

`printf()` in `user/printf.c` is `vprintf(1, …)` with descriptor 1 hardwired; `fprintf()` takes the descriptor as its first argument. xv6 has no `FILE`, no `stderr` stream, and no `fdopen()` — `user/user.h` declares exactly these two functions — so `fprintf(2, …)` is the only way to reach descriptor 2 at all.

#### Where is Ctrl-d implemented?

Nowhere in `ex1copy.c`, which **never** names it: `read()` returns 0 and the `> 0` loop test does the rest. Two functions in `kernel/console.c` produce that zero.

| Function        | What it does with `C('D')`                                                                                                                                                     |
| --------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| `consoleintr()` | Accepts byte `0x04` — `C(x)` is `((x) - '@')` — as a line terminator alongside `'\n'`, advancing `cons.w` and waking a sleeping reader even though no newline arrived.         |
| `consoleread()` | Pulls that byte back out, `break`s without copying it, and returns `target - n`. At an empty position nothing has been copied yet, `n` is still `target`, and the result is 0. |

**Ctrl-d after partial input first returns those bytes.** With `abc` already typed, `consoleread()` takes `if (n < target) cons.r--` and pushes Ctrl-d back into the buffer. That `read()` returns three bytes; only the next call returns 0. This is why the interactive instruction specifies an empty line and a partial line needs two presses.

### ex1copy regression tests

[`grade-exercises`](../grade-exercises) holds three tests for `ex1copy`, each pinning one of the behaviours above:

| Test                                 | What it asserts                                                                                                                                                        |
| ------------------------------------ | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `copies finite piped input`          | A 71-byte payload survives `echo … \| ex1copy` byte for byte. It deliberately exceeds `buf[64]`, so the copy takes two loop iterations rather than one.                |
| `interactive input ends with Ctrl-d` | Each typed line appears exactly twice — console echo, then the copy — and `\x04` at an empty position lets the shell go on to run the next command.                    |
| `reports a closed output pipe`       | With a reader that exits first, `write()` returns -1 and the program prints `ex1copy: write error` and exits, instead of looping back into `read()` and waiting there. |

```bash
./grade-exercises                          # every exercise, 18 tests
./grade-exercises ex1copy                  # just this one
./grade-exercises "closed output pipe"     # one test, by name
```

Filtering is a substring match against the test title, and every title is prefixed with its exercise name — see [the suite](#grade-exercises-coverage-and-mutation-checks) below. The harness itself — how it boots QEMU, drives the shell, and pattern-matches the transcript — is described in [03-lab-workflow.md](03-lab-workflow.md#run-graders-and-interpret-failure-transcripts).

## `ex2create` — making a descriptor

`ex1copy` ends on the lecture's open question: how do you make a _new_ file descriptor? `open()` is the first of the three answers, and the exercise exists to show the number it hands back.

```text
$ ex2create
ex2create: open() returned fd 3
$ cat ex2.out
Hello
```

### Why open() returns descriptor 3

**`fdalloc()` returns the lowest unused descriptor.** It scans `p->ofile[]` from index 0 in `kernel/sysfile.c`.

Slots 0, 1, and 2 are already occupied before `main()` starts: `user/init.c` opens the console and `dup()`s it twice, and `sh` inherits all three through `fork()`. The exercise's first `open()` therefore gets slot 3.

| Slot  | Holds                    | Put there by                     |
| ----- | ------------------------ | -------------------------------- |
| **0** | the console, for reading | `user/init.c`, inherited by `sh` |
| **1** | the console, for writing | `dup(0)` in `user/init.c`        |
| **2** | the console, for writing | `dup(0)` in `user/init.c`        |
| **3** | _empty_                  | ← `fdalloc()` picks it           |

That rule is not a curiosity. It is load-bearing: `close(1)` frees slot 1, so the next `open()` is _guaranteed_ to land there, which is the entire mechanism behind [`ex6redirect`](#ex6redirect--how--is-implemented).

### open(), fork(), and dup(): descriptor and offset sharing

`open()` builds a chain, and later exercises depend on knowing where it can be cut:

```text
p->ofile[3]  ──►  struct file  ──►  inode
 (per process)     (holds the        (holds the
                    offset)           data)
```

The shared `struct file` determines whether two descriptors move one offset:

| Descriptor origin                | Offset relationship                                                                  |
| -------------------------------- | ------------------------------------------------------------------------------------ |
| **fork()**                       | Copies the descriptor-table level and shares the `struct file`.                      |
| **dup()**                        | Adds another descriptor for the same `struct file`.                                  |
| **Two independent open() calls** | Create separate `struct file` objects and separate offsets, even for the same inode. |

Thus two descriptors share an offset **if and only if** they descend from the same original through `fork()` or `dup()`. A parent and child writing through that shared offset append in turn rather than overwriting each other.

### open(): differences from Linux and missing lseek()

| Compared with Linux       | In xv6                                                                                                                                 |
| ------------------------- | -------------------------------------------------------------------------------------------------------------------------------------- |
| `open(path, flags, mode)` | Two arguments only. There are no users, groups, or permission bits, so there is no `mode` to pass.                                     |
| `O_CREAT`                 | Spelled `O_CREATE`, in `kernel/fcntl.h`. Five flags exist in total: `O_RDONLY`, `O_WRONLY`, `O_RDWR`, `O_CREATE`, `O_TRUNC`.           |
| `lseek`                   | Absent. The book names it as one of the basic calls that make xv6 non-POSIX; an offset only ever moves forward, by reading or writing. |

### ex2create: open() and short-write errors

The lecture slide says of its own examples: "these examples ignore errors — don't be this sloppy!" The official `ex2.c` does ignore them; this version does not, because neither failure announces itself where it happens.

| Ignored                 | What you actually see                                                                                                          |
| ----------------------- | ------------------------------------------------------------------------------------------------------------------------------ |
| **`open()` returns -1** | `write(-1, …)` fails silently, the program reports success, and `cat ex2.out` says it cannot open the file — three steps away. |
| **`write()` short**     | A truncated or empty file, and no message at all.                                                                              |

## `ex3fork` — one call, two returns

```text
$ ex3fork
(ex3fork) fork() returned 6  (I am pid 5)
(ex3fork) fork() returned 0  (I am pid 6)
(ex3fork) parent: my child is pid 6
(ex3fork) child: my own pid is 6
```

One `printf()` in the source; two lines on the console. That is the exercise. After `fork()` there are two processes executing the same instructions, and the only thing distinguishing them is the value the call returned.

| Return  | In which process | Means                                                                           |
| ------- | ---------------- | ------------------------------------------------------------------------------- |
| **> 0** | the parent       | the child's pid — the parent is told who its child is, so it can `wait()` later |
| **0**   | the child        | "you are the new one"; 0 is a pid no process can have                           |
| **-1**  | the parent       | no child was created: `NPROC` reached, or out of memory                         |

`fork()` copies instructions, data, stack, the descriptor table, and the current directory. From that moment the two memories are separate — the program sets `x` to different values in each half to make that concrete — while the file _offsets_ behind the copied descriptors stay shared.

### The output is not just reordered, it is interleaved mid-word

A real run looks like this:

```text
(ex3fork) forke(x3for)k returned : fork() retu6r n e(dI  0  (I ama mp
```

> [!IMPORTANT]
> `printf()` in `user/printf.c` is **not atomic**, and output can mix one byte at a time.

- **Between writes:** `printf()` formats into a small buffer and calls `write()` as it goes; the scheduler can switch processes between those calls.
- **Across harts:** the default `CPUS=3` lets both processes run at the same instant. `uartputc()` locks per character rather than per line, so individual bytes can interleave.

This is the first appearance of the problem the locking and scheduling chapters exist to solve. `make qemu CPUS=1` usually produces whole lines, which is not a fix — it only makes the race harder to hit.

A third process joins in: `sh` forked this program and is sitting in `wait()`, which returns as soon as _its_ child exits. The child here may still be running then, so the `$` prompt can land in the middle. The prompt is not a promise that everything finished.

This program deliberately omits `wait()` so the raw behaviour is visible. A parent that exits first leaves an **orphan**, which `kexit()` reparents to `init` (pid 1); `init` waits in a loop forever so no exited process stays a zombie.

## `ex4exec` — becoming another program

```text
$ ex4exec
this is echo
```

The program you typed printed nothing. `this is echo` came from `user/echo.c`, running in the very process that started as `ex4exec`. No new process was created — two programs took turns inside one pid.

| Replaced by `exec()` | Preserved across `exec()` |
| -------------------- | ------------------------- |
| instructions         | pid                       |
| data and stack       | the file descriptor table |
| heap                 | the current directory     |
|                      | the parent relationship   |

The preserved column is the one that matters. Because `exec()` keeps the descriptor table, a caller can rearrange descriptors _first_ and then `exec()` a program that knows nothing about the rearrangement — which is exactly how `>` and `|` are implemented.

`kexec()` in `kernel/exec.c` swaps the new page table in as its **last** step, so a failure at any earlier point leaves the original program intact and lets `exec()` return -1 into it.

### Why the argument array ends in `0`

Because nothing else says how long it is. `exec()` takes no count, so the kernel walks the array until it finds a null pointer. From `sys_exec()` in `kernel/sysfile.c`:

```c
for (i = 0;; i++) {
  if (i >= NELEM(argv)) goto bad;                        // MAXARG (32) reached
  if (fetchaddr(uargv + sizeof(uint64) * i, &uarg) < 0) goto bad;
  if (uarg == 0) { argv[i] = 0; break; }                 // the terminator
  ...
}
```

**A missing null pointer makes the kernel read beyond the argument array.** It fetches whatever follows on the stack until one of these outcomes:

- **Garbage arguments:** later stack contents supply unintended pointers and a terminator.
- **Failure:** an invalid access reaches `bad` and returns -1, or `MAXARG` stops the walk at 32 entries.

The walk is bounded, but the result can still be a failed `exec` or arguments nobody typed. A C string needs a sentinel byte; an array of `char *` needs a sentinel pointer.

`argv[0]` is conventionally the program's own name. Nothing enforces it, and `user/echo.c` ignores it entirely — its loop starts at `argv[1]`, which is why the output above begins at `this`.

> [!NOTE]
> `exec()` does not return on success, so the statement _after_ it is the error path. No `if` is needed there, and adding one would misrepresent the control flow.

### Why a shell cannot call exec() without fork()

`exec()` consumed the caller. A shell **cannot** work this way: if `sh` called `exec()` directly it would _become_ `echo`, print, exit, and **never** read a second command. The fix is to `exec()` in a process you can afford to lose — which is the next exercise.

### Why there is no `fork()` here

Because the exercise is about what `exec()` does, and forking first would hide it. `exec()` replaces the program inside _this_ process — same pid, same descriptors, same parent — and the proof is that `this is echo` appears while `ex4exec`'s own error line **never** runs. Add a `fork()` and you are watching two processes instead of one program becoming another, which is [`ex5forkexec`](#ex5forkexec--the-shells-execution-model).

| Shape                                    | The caller afterwards                                 | Good for                                 |
| ---------------------------------------- | ----------------------------------------------------- | ---------------------------------------- |
| **`exec()` alone**                       | gone — its instructions no longer exist               | the _last_ thing a process ever does     |
| **`fork()`, then `exec()` in the child** | untouched, and holding the child's pid to `wait()` on | a shell, which must print another prompt |

> [!TIP]
> `exec()` is a career change, not a hire. The person keeps their badge, keys, and desk — pid, descriptors, current directory, parent — and forgets every trade they knew. A shopkeeper who does that stops being a shopkeeper, and nobody is left to serve the next customer. `fork()` then `exec()` is hiring instead: a copy of you walks off and becomes the new thing while you stay behind the counter.

`ex4exec` can afford the career change because it has nothing left to do.

## `ex5forkexec` — the shell's execution model

```text
$ ex5forkexec
ex5forkexec: parent waiting for child 9
THIS IS ECHO
ex5forkexec: child 9 exited with status 0
```

Unlike `ex3fork`, the order is fixed after the first line: `wait()` does not return until the child is gone, so the exit message can **never** precede the child's output. This is the complete shape of what `user/sh.c` does for **every** command you type.

```mermaid
sequenceDiagram
    participant P as parent
    participant C as child

    Note over P,C: fork — two processes from one call
    rect rgb(255, 240, 240)
        P->>C: fork() returns 0 here, the child's pid there
    end

    Note over C: exec — the child becomes a different program
    rect rgb(240, 240, 255)
        C->>C: exec("echo", argv) — never returns
        C->>C: echo writes to fd 1
    end

    Note over P,C: wait — the parent blocks until the child exits
    rect rgb(240, 255, 255)
        C-->>P: exit(0) — 32 bits of status
        P->>P: wait(&status) returns the child's pid
    end
```

### `wait()` does not order what comes _before_ it

The first line still races, and a real run shows it:

```text
$ ex5forkexec
ex5forkexec: parenTHIS t ISw aECHO
iting for child 8
ex5forkexec: child 8 exited with status 0
```

The `parent waiting` message is printed in the gap between `fork()` and `wait()`, so it competes with the child's `THIS IS ECHO` exactly as the two halves of [`ex3fork`](#ex3fork--one-call-two-returns) compete — same non-atomic `printf()`, same per-character `uartputc()` lock. `wait()` orders everything after the call and nothing before it. Move the message above the `fork()`, or below the `wait()`, and the run comes out clean.

> [!WARNING]
> **`exec ex5forkexec failed` comes from the shell:** the exercise never started. Its own error is `ex5forkexec: exec echo failed`.

`runcmd()` in `user/sh.c` emits the shell error when it cannot exec the name it read. Pasted or typed-ahead input can corrupt that name:

- **Buffer limit:** `consoleintr()` in `kernel/console.c` accepts input only under `if (c != 0 && cons.e - cons.r < INPUT_BUF_SIZE)`. Once its 128-byte line buffer is full, it drops characters without echoing them.
- **Reproduction:** feeding QEMU 100 command lines at once produced `exec ex5forkexx5fokrc failed`.
- **Prompt-paced check:** feeding 120 runs one line per prompt finished all 120 with zero failures.

### `wait()` returns a pid and delivers status through a pointer

One integer **cannot** carry both the identity of the child and the value it exited with, so the status leaves through an out-parameter.

| Return  | Means                                                                                                                                |
| ------- | ------------------------------------------------------------------------------------------------------------------------------------ |
| **> 0** | the pid of a child that exited. With several children this is how you learn _which_ — `wait()` reaps any, and xv6 has no `waitpid()` |
| **-1**  | the caller has no children at all                                                                                                    |

Whether `wait()` blocks depends on the children's state:

| State                                   | wait() behavior                           |
| --------------------------------------- | ----------------------------------------- |
| **No child has exited yet**             | Blocks until a child exits.               |
| **An exited child is already a zombie** | Collects that child's status immediately. |

A **zombie** has exited but remains available for status collection. `kexit()` sets `p->state = ZOMBIE` and wakes the parent; `kwait()` copies `p->xstate` out and calls `freeproc()`. `user/zombie.c` demonstrates a parent that never waits.

Passing `0` instead of an address tells the kernel to skip the store — `kwait()` guards it with `if (addr != 0 && copyout(...) < 0)`. That is the documented way to say "I don't want the status", and it is what [`ex6redirect`](#ex6redirect--how--is-implemented) does.

The exit status is convention, not enforcement: `0` for success, non-zero for failure. It is how a child reports an outcome to a parent that **cannot** see its memory, and 32 bits is the entire channel.

### Why `fork` and `exec` are not one call

The gap between them is where redirection and pipelines are built. Code running in the child after the `fork` but before the `exec` can change the child's world — and only the child's — without the parent or the exec'd program knowing.

> [!NOTE]
> The xv6 book makes the counterfactual explicit: a combined `forkexec()` would force the shell to modify its own I/O and undo it afterwards, or take redirection instructions as arguments, or (least attractively) teach every program like `cat` to do its own redirection.

`fork()` copies the whole address space and `exec()` discards it instructions later. Real kernels avoid the waste with copy-on-write, which is the `cow` lab.

## `ex6redirect` — how `>` is implemented

```text
$ ex6redirect
ex6redirect: parent waiting for child 11
ex6redirect: child 11 finished; try `cat ex6.out`
$ cat ex6.out
ex6redirect's redirected echo
```

`echo`'s output **never** reaches the console, and nothing about `echo` changed. The child rewired descriptor 1 before exec'ing it.

```text
child's FD table       close(1)              open("ex6.out", ...)
+--------------+       +--------------+      +--------------------+
| 0 -> console |       | 0 -> console |      | 0 -> console       |
| 1 -> console |  ==>  | 1 <empty>    |  ==> | 1 -> ex6.out       |
| 2 -> console |       | 2 -> console |      | 2 -> console       |
+--------------+       +--------------+      +--------------------+
                              ^                       ^
                slot 1 is now the lowest    fdalloc() must pick it
                free slot in the table      — there is no choice
```

Three properties combine, and removing any one breaks it:

| Step               | What it provides                                           |
| ------------------ | ---------------------------------------------------------- |
| `fork()`           | a process whose descriptors can safely be damaged          |
| **`close`+`open`** | puts the file where the program will look for descriptor 1 |
| `exec()`           | keeps the table while replacing the program                |

Descriptor 2 is untouched, which is why an error from `echo` would still reach the terminal — `>` redirects only the descriptor you name. That is the same reason `ex1copy` puts its hint on 2.

### Why the parent is unaffected

`close(1)` runs in the child, whose descriptor table is a _copy_ made by `fork()`. Closing a slot marks `p->ofile[1] = 0` in one `struct proc` only. This is precisely why the shell **must** fork before redirecting: a shell that closed its own descriptor 1 would lose the terminal permanently, and every later prompt would land in whatever file the last command mentioned.

> [!TIP]
> The `(int *)` cast in `wait((int *)0)` is style, not necessity — a bare `0` is a valid null pointer constant and compiles identically. The xv6 book writes the cast to make the argument's type visible; `user/sh.c` writes plain `wait(0)`. Both appear in this tree. The cast is worth keeping where a lone `0` would read like a status _value_.

## `ex7pipe` — a buffer with two descriptors

```text
$ ex7pipe
ex7pipe: pipe() gave read fd 3, write fd 4
xyz
```

One `pipe()` call, two descriptors, one kernel object:

| `fds[]`  | End   | Direction       | Blocks when                                   |
| -------- | ----- | --------------- | --------------------------------------------- |
| `fds[0]` | read  | out of the pipe | the buffer is empty and a writer still exists |
| `fds[1]` | write | into the pipe   | the buffer is full and a reader still exists  |

The buffer is `char data[PIPESIZE]` — 512 bytes — in `kernel/pipe.c`. It is not a file, nothing touches the disk, and it has **no name in the file system**, which is why a pipe can only be handed to another process by `fork()`.

Nothing here uses a pipe-specific call. `read()` and `write()` are the same ones from `ex1copy`; `fileread()` and `filewrite()` branch on `f->type` and forward to `piperead()`/`pipewrite()`. That uniformity is the whole abstraction: `ls | grep x` works because `grep` reads descriptor 0 with no idea a pipe is behind it.

> [!WARNING]
> **This one-process version works only by staying under the buffer size.** Writing more than `PIPESIZE` before reading makes the process block on itself.

`pipewrite()` sleeps at `pi->nwrite == pi->nread + PIPESIZE`, waiting for the same process to drain the buffer. That process cannot reach its read while asleep: a pipe needs two schedulable participants for this rendezvous. `Ctrl-p` dumps the process table and shows it stuck in state `sleep`.

A pipe has no EOF marker in the data. `read()` returns 0 only when the buffer is empty **and** every write-end descriptor has been closed. Until then, an empty pipe means "wait", not "finished" — which is the subject of the next exercise.

## `ex8pipefork` — two processes, one buffer

```text
$ ex8pipefork
ex8pipefork: parent read 17 bytes: hello from child
```

The bytes crossed a process boundary. Neither process can see the other's variables, so the pipe is the only channel between them.

### `pipe()` must come before `fork()`

A pipe has no name, so no process can `open()` one somebody else created. The only route is inheritance: make the pipe first, then fork, and the child's copied descriptor table already points at the same kernel buffer. Reversing the order gives each process a private, unconnected pipe.

After the fork there are **four** descriptors on one pipe — two read ends and two write ends. That is one more of each than the design needs, and the extras are not harmless.

### Closing unused ends is protocol, not hygiene

`piperead()` sleeps while `pi->nread == pi->nwrite && pi->writeopen`, and `pipeclose()` clears `writeopen` only when the _last_ writer goes away. So a parent holding its own copy of the write end keeps the pipe open forever, even after the child exits:

```text
parent forgets close(fds[1])
  → child exits, its write end closes
  → writeopen is still 1, because the parent still holds one
  → parent's read() sees an empty buffer and sleeps
  → nothing will ever write, and nothing will ever close
```

The mirror-image rule holds on the other side: if every read end closes while a writer is still writing, `pipewrite()` sets the writer's `p->killed` and `write()` returns -1 — xv6's stand-in for `SIGPIPE`, and what the `reports a closed output pipe` test exercises.

> [!NOTE]
> This program reads a fixed byte count and would survive the mistake. The closes are there anyway, because in a real pipeline the reader always waits for EOF.

### Shell pipelines: dup() connects the pipe to descriptor 0

`ls | grep x` needs something this file does not show. Here the parent reads descriptor 3 knowingly; `grep` **must** read descriptor 0, because that is all any program knows. The shell bridges the gap with `dup()`:

```c
close(0);          // give up the console
dup(p[0]);         // the pipe's read end lands in slot 0
close(p[0]);       // drop the now-redundant original
close(p[1]);       // and the write end, so EOF can happen
exec("grep", argv);
```

Same lowest-free-descriptor rule as `ex6redirect`, with `dup()` in place of `open()` because the target is an existing descriptor rather than a file. A pipe is one-directional — writing to `fds[0]` fails — so two-way conversation needs two pipes, which is exactly the chapter 1 exercise in the xv6 book.

## `ex9ls` — a directory is a file

```text
$ ex9ls
.
..
README
cat
echo
...
```

There is no `opendir()`, no `readdir()`, and no directory-listing system call. A directory is a file whose contents are a plain array of fixed-size records, and you list it with the same `open()` and `read()` as everything else. From `kernel/fs.h`:

```c
#define DIRSIZ 14

struct dirent {
  ushort inum;                 // which inode, or 0 for a free slot
  char name[DIRSIZ] __attribute__((nonstring));
};
```

Sixteen bytes per entry, so a directory of _n_ entries is exactly 16*n* bytes. The read loop therefore contains no parsing at all: each `read()` of `sizeof(de)` bytes lands one whole record, and the file's length says when to stop.

### Directory records: free slots, name termination, and short reads

| Detail                                     | Why, and what happens otherwise                                                                                                                                                                                                        |
| ------------------------------------------ | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **Skip entries with `inum == 0`**          | Those are free slots. `unlink()` blanks a `dirent` by zeroing it rather than compacting the directory, so the array is sparse. Printing them yields blank lines and stale names.                                                       |
| **Do not assume `name` is NUL-terminated** | It is marked `__attribute__((nonstring))`: a 14-character name fills the field with no room for a terminator, so `printf("%s")` would run off the end into the next record. Copy into a `DIRSIZ + 1` buffer and terminate it yourself. |
| **Read whole records**                     | `== sizeof(de)` is both the loop condition and the guard — a short read means the directory ended, and a partial record is **never** processed.                                                                                        |

`ex9ls` also calls `fstat()` first and rejects anything that is not `T_DIR`. Without that check a plain file would be read 16 bytes at a time and its contents printed as names, with the first two bytes of every chunk silently taken for an inode number.

### `.` and `..` are real entries

Not shell syntax, and not special-cased by the program: they are written into every directory when `create()` makes it. `.` links to the directory itself and `..` to its parent, which is how relative path resolution works at all — `namei()` just walks entries by name.

So `open(".", O_RDONLY)` opens the process's current directory, tracked as `p->cwd` and changed with `chdir()`.

> [!IMPORTANT]
> This is why `cd` had to be built **into** the shell rather than shipped as a program. `sh` forks a child for each command, so a `cd` running as its own process would change that child's `p->cwd` and exit, leaving the shell exactly where it was.

### Why the program cannot write the directory back

Reading a directory is allowed; writing one is not. `sys_open()` refuses a directory opened with anything but `O_RDONLY`. Only the kernel edits directory contents, through `create()`, `sys_link()`, and `sys_unlink()`.

The reason is integrity: a directory entry references an inode, and the inode's `nlink` count **must** stay in step with it. A user program writing raw bytes could produce an entry pointing at a free inode, or a link count nothing agrees on — a corrupt file system with no way back. Early Unix did allow this, and it is one of the places where later designs tightened the interface.

## Lecture 2 — C and memory

For a small assembly experiment behind these C observations, see [load widths, signedness, and byte order](../../assembly-language/docs/riscv/02-memory-and-pointers.md#loads-width-signedness-and-byte-order). The exercise results below are from this tree.

Lecture 2 ([`l-c.txt`](../lectures/l-c.txt)) hands out three xv6 demos that already have descriptive names, so the practice versions keep them instead of taking an `ex<N>` prefix; `hwstruct` adds the homework demo the lecture runs but MIT did not publish. The lecture's own concepts, the user address space, and the Homework 2 answer live in [10-c-in-xv6.md](10-c-in-xv6.md); these sections cover only the programs.

| Program                                                    | Source            | Lecture                                                             | What it introduces                                                   |
| ---------------------------------------------------------- | ----------------- | ------------------------------------------------------------------- | -------------------------------------------------------------------- |
| **[`ptr`](#ptr--global-stack-and-function-addresses)**     | `user/ptr.c`      | [`ptr.c`](https://pdos.csail.mit.edu/6.1810/2026/lec/l-c/ptr.c)     | `&`, `*`, `->`, `int **`, function pointers, and `argv` on the stack |
| **[`array`](#array--two-spellings-of-one-access)**         | `user/array.c`    | [`array.c`](https://pdos.csail.mit.edu/6.1810/2026/lec/l-c/array.c) | `a[i]` versus `*(a + i)`, and arithmetic that scales by element size |
| **[`hwstruct`](#hwstruct--homework-2-on-64-bit-xv6)**      | `user/hwstruct.c` | lecture's "[demo hw.c]"                                             | Homework 2's struct expressions, run instead of computed             |
| **[`str`](#str--writable-stack-versus-read-only-literal)** | `user/str.c`      | [`str.c`](https://pdos.csail.mit.edu/6.1810/2026/lec/l-c/str.c)     | `0`-terminated strings and a deliberate overflow                     |

### `ptr` — global, stack, and function addresses

```text
$ ptr x yz
p=0x0000000000001000
q=0x0000000000004F5C
...
func=0x0000000000000000
argv=0x0000000000004FB0
argv[0]=0x0000000000004FF0 "ptr"
...
argv[3]=0x0000000000000000
```

Each printed address falls into one region of the [user address space](10-c-in-xv6.md#user-address-space-text-data-guard-stack-and-heap): `p` (`&g`) in the `.data` page, `q` and `pp` in `main`'s stack frame, and `argv` above them where `kexec()` left it. Two results are the point of the program:

| Output                    | Why                                                                                                                       |
| ------------------------- | ------------------------------------------------------------------------------------------------------------------------- |
| `*c=11`                   | `*(char *)&g` reads one byte. RISC-V is little-endian, so it is the low byte of `11`; the pointer's type chose the width. |
| `func=0x0000000000000000` | `main` is linked at address 0. A valid function pointer equals null here.                                                 |

The lecture passes a function pointer straight to `%p` and **never** uses `main`'s parameters. Practice programs here build with `-Wextra -pedantic` (`STRICT_CFLAGS` in the [Makefile](../Makefile)), which rejects both, so `ptr.c` casts through `uint64` and uses `argc`/`argv` to print the stack layout the lecture draws for `ls.c`.

### `array` — two spellings of one access

```text
$ array
...
a=0x0000000000001010
a+1=0x0000000000001014
...
b=0x0000000000001000
b+1=0x0000000000001001
```

`a + 1` moves 4 bytes and `b + 1` moves 1, from the same `+ 1`. The arrays stay global as in the lecture, so `nm -n user/_array` prints the same addresses before the program ever runs. Why `b` lands below `a` is answered in [10-c-in-xv6.md](10-c-in-xv6.md#why-does-b-sit-below-a-in-array).

### `hwstruct` — Homework 2 on 64-bit xv6

```text
$ hwstruct
sizeof: f=36 g=24 h=32
&h->g.d[2]=0x0000000000001018
&h->g.c=0x0000000000001008
&h->f->a=0x0000000000000000
&h->g.c[10]=0x000000000000000A
```

The first two addresses are guaranteed; the last two are loads from whatever this binary has at `0x1000`, which happens to be `umalloc.c`'s zeroed `freep`. The [Homework 2 section](10-c-in-xv6.md#homework-2-struct-addresses) has the 32-bit answer the handout asks for and the table of which expressions touch memory.

### `str` — writable stack versus read-only literal

```text
$ str
s=123
...
s1[4] write: no fault
usertrap(): unexpected scause 0xf pid=3
            sepc=0x86 stval=0x914
```

Both writes are one byte past a 4-byte string, but only one is caught:

| Write         | Lands in                                              | Result                                                                                           |
| ------------- | ----------------------------------------------------- | ------------------------------------------------------------------------------------------------ |
| `s1[4] = 'D'` | `main`'s stack frame, mapped writable                 | nothing happens; **undefined behaviour** that can corrupt the frame                              |
| `s[4] = 'D'`  | `.rodata`, in the text segment mapped without `PTE_W` | store page fault (`scause` `0xf`); `usertrap()` kills the process, so `not reached` never prints |

The fault report identifies both the destination and the store:

- **`stval = 0x914`:** the faulting address is `s + 4`; `objdump -s -j .rodata user/_str` places the literal `"123"` at `0x910`.
- **`sepc = 0x86`:** the faulting instruction is the `sb` in `user/str.asm`.

**Page permissions cause the fault**, not the array boundary: `s[0] = 'D'` would fault too. The lecture keeps `s` global and performs only the silent `s1[4]` write; this practice version adds the literal write to expose the permission difference.

## Lecture 3 — the user/kernel boundary

Lecture 3 ([`l-os.txt`](../lectures/l-os.txt)) publishes no example code, so all three programs are written here to make the lecture's claims observable: `isolate` attacks the boundary four ways, `forkfill` answers the lecture's "what happens if out of proc's?", and `hwdesign` runs Homework 3's three questions on a full table. The concepts, the boot path, and the `fork`/`exec` walkthroughs live in [11-os-design-in-xv6.md](11-os-design-in-xv6.md); these sections cover only the programs.

| Program                                                         | Source            | Lecture                                                 | What it introduces                                                     |
| --------------------------------------------------------------- | ----------------- | ------------------------------------------------------- | ---------------------------------------------------------------------- |
| **[`isolate`](#isolate--four-attempts-four-refusals)**          | `user/isolate.c`  | "hardware-level isolation", "How do system calls work?" | privileged instructions, `PTE_U`, `a7` checks, and user-pointer checks |
| **[`forkfill`](#forkfill--running-out-of-process-slots)**       | `user/forkfill.c` | "HW: allocproc; what happens if out of proc's?"         | `NPROC`, `allocproc()` failure, and slots freed only by `wait()`       |
| **[`hwdesign`](#hwdesign--homework-3-on-a-full-process-table)** | `user/hwdesign.c` | Homework 3, _kernel design_                             | `exec()` in an existing slot, and `kill()` freeing nothing by itself   |

### `isolate` — four attempts, four refusals

```text
$ isolate csr
isolate: csrr sstatus from user mode
usertrap(): unexpected scause 0x2 pid=3
            sepc=0x44 stval=0x100027f3
$ isolate kload
isolate: load from TRAMPOLINE 0x0000003FFFFFF000
usertrap(): unexpected scause 0xd pid=4
            sepc=0x86 stval=0x3ffffff000
$ isolate badsys
isolate: ecall with a7=99
5 isolate: unknown sys call 99
isolate: ecall returned -1
$ isolate kcopy
isolate: read() into TRAPFRAME 0x0000003FFFFFE000 returned -1
isolate: read() into a user buffer returned 2: "hi"
```

The first two runs die and the last two survive, which is the lecture's split between hardware isolation and a careful system-call interface; the [boundary table](11-os-design-in-xv6.md#program-isolation-hardware-faults-and-kernel-checks) names the check behind each line. Two program-specific findings:

| Finding                                  | Why                                                                                                                                      |
| ---------------------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------- |
| **`stval=0x100027f3` is not an address** | For an illegal instruction, RISC-V reports the instruction's encoding; `user/isolate.asm` shows `100027f3` at `0x44`, `csrr a5,sstatus`. |
| **`"hi"` survives the refused read**     | `piperead()` returns `-1` from inside its copy loop before `nread++`, so a failed `copyout()` consumes nothing.                          |

`badsys` needs a hand-written `ecall`, because every stub in `user/usys.S` hard-codes its own `a7`. `r_sstatus()` is the kernel's own accessor from `kernel/riscv.h`, included unchanged: the same inline function is legal in S-mode and fatal in U-mode.

### `forkfill` — running out of process slots

```text
$ forkfill
forkfill: fork returned -1 after 61 children
forkfill: reaped 61 children
forkfill: fork works again
```

**61 children fill the remaining process slots:** `NPROC` is 64, with `init`, `sh`, and `forkfill` already occupying three.

1. Each child closes its inherited write end before blocking in `read()`.
2. The parent closes its sole remaining `p[1]`, releasing all the readers at once.
3. The parent reaps the children; a second run again finds 61 slots, showing they returned to `UNUSED`.

A child that kept `p[1]` would hold the pipe open forever, the same protocol as [`ex8pipefork`](#closing-unused-ends-is-protocol-not-hygiene).

### `hwdesign` — Homework 3 on a full process table

```text
$ hwdesign
hwdesign: table full: fork returned -1 after 60 children
hwdesign: probe exec'd echo while the table was full
hwdesign: exec used the probe's own slot; probe exited 0
hwdesign: refilled 1 freed slot
hwdesign: kill(65) returned 0; fork right after returned -1
hwdesign: wait reaped killed pid 65, status -1
hwdesign: fork after that wait succeeded
hwdesign: reaped 60 children
```

**60 children, not 61:** a probe child is forked before the table fills, so it already owns a slot when the parent later tells it to `exec("echo", ...)`. The pids vary from run to run.

1. The parent fills the table with children blocked on a pipe, exactly as `forkfill` does.
2. It releases the probe, whose `exec()` succeeds, then refills the probe's freed slot.
3. It kills its last child and forks at once: `-1`, because the victim is the parent's own unreaped child.
4. One `wait()` returns the victim with status `-1`, after which `fork()` succeeds.

The answer and its reasoning live in [11-os-design-in-xv6.md](11-os-design-in-xv6.md#homework-3-a-full-process-table-versus-exec-fork-and-kill).

## grade-exercises: coverage and mutation checks

[`grade-exercises`](../grade-exercises) covers all nine Lecture 1 exercises, the four Lecture 2 programs, and the three Lecture 3 programs: nineteen tests in about twenty seconds. Each test pins a claim this document or a source comment makes as fact — the point is that the prose **cannot** drift away from the programs without something going red.

| Test             | The claim it pins                                                                                                                                                              |
| ---------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| **`ex1copy`** ×3 | Filter semantics: a finite pipe is copied byte for byte, `Ctrl-d` at an empty position ends an interactive run, and a closed reader produces `write error` rather than a hang. |
| `ex2create`      | `open()` returns **3**, and `Hello` reaches the disk.                                                                                                                          |
| `ex3fork`        | Both branches run, and each writes its **own** `x` — the address spaces are copies, not shared.                                                                                |
| `ex4exec`        | `echo`'s output appears and `ex4exec` prints nothing of its own, because the statement after a successful `exec()` **never** runs.                                             |
| `ex5forkexec`    | Every byte the child wrote is out **before** the parent's status line.                                                                                                         |
| `ex6redirect`    | The redirected text reaches the console **only** via `cat`, **never** while the program runs.                                                                                  |
| `ex7pipe`        | `pipe()` returns descriptors 3 and 4, and the bytes come back.                                                                                                                 |
| `ex8pipefork`    | All 17 bytes cross the process boundary.                                                                                                                                       |
| **`ex9ls`** ×2   | `.` and `..` are listed, free slots are skipped, and a plain file and a missing path are both rejected with a message rather than garbage.                                     |
| `array`          | Values after index and pointer writes, `a + 1` moving 4 bytes and `b + 1` moving 1, and printed addresses equal to `nm`'s.                                                     |
| `ptr`            | Writes through `p`, `q`, `c`, and `pp`; `p` and `func` equal `nm`'s `g` and `main`; `argv` strings in order with a `0` terminator.                                             |
| `str`            | Index and pointer access agree, the `s1[4]` overflow is silent, and `s[4]` raises a store fault at exactly the literal's address + 4 with the next line **never** printed.     |
| `hwstruct`       | RV64 sizes 36/24/32, the two guaranteed addresses, and all four lines printing without a fault.                                                                                |
| `isolate`        | `csr` and `kload` die with `scause` 2 and `0xd`, `stval` naming the `csrr` encoding and `TRAMPOLINE`; `badsys` and `kcopy` return `-1`; `"hi"` is kept.                        |
| `forkfill`       | `fork()` fails after exactly 61 children, all 61 are reaped, and a second run finds the same 61 free slots.                                                                    |
| `hwdesign`       | With the table full, `exec()` succeeds, `fork()` returns `-1` even right after `kill()`, and the pid `wait()` reaps with status `-1` is the one killed.                        |

Several of these pin _kernel_ invariants rather than program output — `fdalloc()`'s lowest-free rule, `exec()` preserving the descriptor table, `wait()` ordering. **A lab that touches `kernel/proc.c`, `kernel/exec.c`, or `kernel/sysfile.c` should run this suite**; it will tell you within seconds whether you changed a user-visible contract.

> [!IMPORTANT]
> **When adding a test, deliberately break the code and confirm the test goes red.** Mutation testing exposed two initially vacuous assertions:

| Mutation that initially passed      | Why the test missed it                                                       | Coverage correction                                             |
| ----------------------------------- | ---------------------------------------------------------------------------- | --------------------------------------------------------------- |
| **Delete ex9ls's `inum == 0` skip** | A fresh `fs.img` has no free slots, and `.strip()` hid trailing blank lines. | Create a directory hole deliberately and preserve the evidence. |
| **Delete ex5forkexec's wait()**     | With three harts the child usually wins the race anyway.                     | Pin the ordering test to `CPUS=1`.                              |

A trusted green test that cannot fail is worse than no test.

## Add a lecture exercise: naming and verification procedure

The naming rule is `ex<N><verb>`: `N` from the lecture's own numbering, verb from the lecture's own wording, and the whole guest command name at most **14 bytes** because `DIRSIZ` in `kernel/fs.h` bounds every filename in `fs.img`. When the lecture's file already has a descriptive name, as Lecture 2's `ptr.c` does, keep it.

The full procedure — the checklist, the comment-block shape, the verification loop — is [`prompts/add-lecture-exercise.md`](prompts/add-lecture-exercise.md), runnable as `/exercise` in Claude Code and copy-pasteable into any other agent. The build-side mechanics it depends on are in [02-build-boot-and-usage.md](02-build-boot-and-usage.md#adding-and-running-a-user-exercise).
