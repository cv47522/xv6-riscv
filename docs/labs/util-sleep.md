# Lab util: Deriving `sleep`

---

[toc]

---

The first exercise of MIT's [util lab](https://pdos.csail.mit.edu/6.1810/2026/labs/util.html) produces a small user program, but deriving it crosses four existing files: `user/user.h`, generated `user/usys.S`, `kernel/syscall.c`, and `kernel/sysproc.c`. This note explains that path and leaves the target `user/sleep.c` as the exercise.

> [!IMPORTANT]
> This note contains no solution or pseudocode. It records existing code, the target contract, source-backed decisions, and common traps.

## What the exercise asks for

| Requirement                           | What it fixes                                                                                          |
| ------------------------------------- | ------------------------------------------------------------------------------------------------------ |
| **Program `sleep` in `user/sleep.c`** | The host build product is `user/_sleep`; `mkfs` imports it into `fs.img` as the guest command `sleep`. |
| **One decimal argument in ticks**     | The interface unit is kernel ticks, not seconds.                                                       |
| **Wait through the kernel**           | Pass the requested count to `pause`; do not poll or count in user space.                               |
| **No argument prints an error**       | The missing-argument path must be visible and must return control to the shell.                        |
| **Add `$U/_sleep` to `UPROGS`**       | The Makefile already contains this entry in this tree.                                                 |

The three relevant tests in [`grade-lab-util`](../../grade-lab-util) are worth 20 points:

| Test                       | Runs                         | Exact observation                                                                                                                                                                                  |
| -------------------------- | ---------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **`sleep, no arguments`**  | `sleep`                      | Rejects an `exec` failure and a transcript containing only the command and prompts, so the program must print something.                                                                           |
| **`sleep, returns`**       | `sleep`, then `echo OK`      | Requires `OK` and rejects the same two failure patterns, so the no-argument invocation must finish.                                                                                                |
| **`sleep, makes syscall`** | `sleep 10`, then `echo FAIL` | Stops at a gdb breakpoint on `sys_pause` and rejects `FAIL`, proving that the program reaches the required syscall before the next command. It does not by itself measure elapsed time or CPU use. |

## Why the call is named `pause`

Upstream xv6 calls this system call `sleep`. This tree reserves `sleep()` for the kernel's sleep/wakeup primitive in [`kernel/proc.c`](../../kernel/proc.c), so the user-facing system call is `pause`, number 13 in [`kernel/syscall.h`](../../kernel/syscall.h). The program remains named `sleep`, while the grader breakpoints `sys_pause`; [`05-syscall-reference.md`](../05-syscall-reference.md#divergences-from-posix) records the divergence.

## The four existing layers

![The sleep program crosses from user space into the kernel: pause puts its argument in a0, ecall traps, syscall dispatch uses a7, and sys_pause retrieves argument zero before waiting](fig/util-sleep-syscall-boundary.svg)

_Pink forms the user request, blue crosses and routes the system-call boundary, yellow performs the kernel work, and cyan returns the result. [Edit the Excalidraw source.](fig/util-sleep-syscall-boundary.excalidraw)_

| File               | Contributes                                                                                          | Does not contribute                                   |
| ------------------ | ---------------------------------------------------------------------------------------------------- | ----------------------------------------------------- |
| `user/user.h`      | Declares `int pause(int)` and the user helpers available without a host C library.                   | No implementation or argument validation.             |
| `user/usys.S`      | Loads `SYS_pause`, executes `ecall`, and returns; it is generated from `user/usys.pl`.               | No policy and no reason to edit it for this exercise. |
| `kernel/syscall.c` | Dispatches call number 13 (`kernel/syscall.h`) and transfers the return value through the trapframe. | No knowledge of the argument's meaning.               |
| `kernel/sysproc.c` | Fetches the tick count and implements the wait in `sys_pause()`.                                     | No parsing of command-line text.                      |

[`06-build-artifacts.md`](../06-build-artifacts.md#the-four-line-stub-decoded) owns the `ecall`, `a7`, and generated-stub mechanics. [`02-build-boot-and-usage.md`](../02-build-boot-and-usage.md#adding-and-running-a-user-exercise) owns user-program layout, headers, formatting, and `UPROGS`.

### Names before mechanisms

| Term                    | Meaning in this exercise                                                                                                                                                                                                                                                                                                                                                          |
| ----------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **User program**        | An executable that runs outside the kernel, such as the future `sleep` command. The `user/` directory also contains library sources such as `ulib.c`, `printf.c`, and `umalloc.c`, so a `.c` file under `user/` is not automatically a separate program. An entry such as `user/_sleep` in `UPROGS` builds a host file with that name; `mkfs` installs it in `fs.img` as `sleep`. |
| **Process**             | One running instance of a user program. Its saved `struct trapframe` holds the user registers while the kernel handles its system call.                                                                                                                                                                                                                                           |
| **Kernel**              | The privileged `kernel/kernel` image built from the files under `kernel/`. `kernel/sysproc.c` contributes handlers to that image; it is not an independently run program.                                                                                                                                                                                                         |
| **System-call handler** | Kernel code that performs a user request. `pause(int)` is the user-facing function, while `sys_pause(void)` is its dispatcher-compatible kernel handler.                                                                                                                                                                                                                          |
| **Hart**                | A RISC-V hardware execution context. QEMU presents each configured xv6 CPU as a hart, so this tree often uses “CPU” and “hart” for the same hardware role. A hart is not a process and is not a user-level software thread.                                                                                                                                                       |
| **Tick**                | One increment of the global `ticks` counter by `clockintr()` on hart 0. Other harts can run processes, but they do not each maintain a separate counter.                                                                                                                                                                                                                          |

> [!NOTE]
> For `user/sleep.c`, the important boundary is simple: the user program validates text, converts it to an integer, and asks `pause` to wait that many ticks. The kernel owns how waiting stops consuming CPU. The lock and wakeup protocol explains how that ownership is implemented safely, but it is not part of the user-program design.

## `sys_pause()` line by line

The waiting logic is this complete function from [`kernel/sysproc.c`](../../kernel/sysproc.c):

```c
uint64
sys_pause(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  if (n < 0)
    n = 0;
  acquire(&tickslock);
  ticks0 = ticks;
  while (ticks - ticks0 < n) {
    if (killed(myproc())) {
      release(&tickslock);
      return -1;
    }
    sleep_prepare(&ticks);
    release(&tickslock);
    sleep();
    acquire(&tickslock);
  }
  release(&tickslock);
  return 0;
}
```

| Code                             | Meaning                                                                                                                                                                                                                                                                                                                   |
| -------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **`argint(0, &n)`**              | Fetches system-call argument index 0 from the saved trapframe. `argraw()` in `kernel/syscall.c` maps indices 0 through 5 to `a0` through `a5`, so index 0 means `a0`; `a7` is separate because it holds the system-call number. Every handler has the dispatcher-compatible form `uint64 f(void)`.                        |
| **Negative clamp**               | Converts negative counts to zero. This prevents the later unsigned comparison from turning `-1` into a multi-year wait.                                                                                                                                                                                                   |
| **`tickslock` and `ticks0`**     | `ticks` and `tickslock` are defined in `kernel/trap.c` and declared for other kernel files in `kernel/defs.h`. The lock protects the shared global counter, while local variable `ticks0` snapshots this caller's starting tick.                                                                                          |
| **`while (ticks - ticks0 < n)`** | Rechecks the caller's own deadline because every tick wakes every waiter on `&ticks`. Unsigned subtraction remains correct across counter wraparound.                                                                                                                                                                     |
| **Killed check**                 | Allows the handler to return `-1`. A sleeping victim is made runnable by `kkill()` in `kernel/proc.c`; after the handler returns, `usertrap()` in `kernel/trap.c` sees the kill flag and calls `kexit(-1)` before returning to user mode. User code therefore cannot observe or recover from this `-1` path in this tree. |
| **Prepare, release, sleep**      | Registers the wait channel before releasing `tickslock`, preventing a lost wakeup. `sleep()` does not block if a wakeup already cleared the channel.                                                                                                                                                                      |
| **Reacquire and return**         | Rechecks under the lock, then returns zero after enough ticks.                                                                                                                                                                                                                                                            |

[`docs/book/ch09-sleep-and-wakeup.md`](../book/ch09-sleep-and-wakeup.md) owns the general lost-wakeup mechanism and this tree's prepare/sleep split.

![sys_pause snapshots the global tick count, sleeps while too few ticks have elapsed, wakes after clock interrupts, and either rechecks or returns](fig/util-sleep-tick-wait.svg)

_The main loop is “snapshot, wait, recheck.” The red wait state does not mean failure; it marks where the process is deliberately not runnable until a wakeup. [Edit the Excalidraw source.](fig/util-sleep-tick-wait.excalidraw)_

## What a tick means

`clockintr()` in [`kernel/trap.c`](../../kernel/trap.c) increments the global counter only on hart 0, calls `wakeup(&ticks)`, and schedules the next interrupt `1000000` RISC-V `time`-register timebase units later. Its source comment calls that interval about one tenth of a second.

| Call        | Approximate nominal interval |
| ----------- | ---------------------------- |
| `pause(1)`  | 0.1 s                        |
| `pause(10)` | 1 s                          |
| `pause(20)` | 2 s                          |

> [!CAUTION]
> Ticks, not seconds, are the interface. The conversion is an observation about the current timer constant, not a guarantee for user programs to encode.

For `n > 0`, the condition becomes false after between `n - 1` and `n` timer intervals because `ticks0` may be sampled anywhere within an interval. `pause(0)` returns immediately. Scheduling can delay the process after the final wakeup, so actual return may be later.

## Deriving `user/sleep.c`

### Contract

| Aspect               | Fixed requirement                                                                                                           |
| -------------------- | --------------------------------------------------------------------------------------------------------------------------- |
| **Input**            | One command-line argument containing a decimal tick count.                                                                  |
| **Available APIs**   | Only declarations in `user/user.h`; there is no host C library.                                                             |
| **Waiting**          | Pass the tick count to `pause`; do not implement a user-space timer.                                                        |
| **Missing argument** | Print a diagnostic and finish so the shell can continue.                                                                    |
| **Exit status**      | A nonzero error status and zero success status are this repository's convention; the grader does not inspect them directly. |

### Decisions and evidence

1. **How should `main` receive arguments?** Compare an argument-taking program such as `user/echo.c` and follow the top-level function style in [`02-build-boot-and-usage.md`](../02-build-boot-and-usage.md#house-style-for-a-new-user-program).
2. **Which `argc` value represents exactly one supplied argument?** Account for `argv[0]`, the program name.
3. **How is text converted to an integer?** Read `atoi()` in `user/ulib.c` and decide how its behavior for non-digits and a leading minus interacts with the kernel's negative clamp.
4. **How is a diagnostic written to file descriptor 2?** Find the descriptor-taking formatted-output function in `user/user.h`; `user/ex2create.c` demonstrates the repository's error style.
5. **What does the grader require from the missing-argument path?** Compare both no-argument test bodies in `grade-lab-util`; distinguish their output and return checks from local exit-status convention.

### Traps

| Trap                                    | Result                                                                                                                                                                                                    | Reason                                                             |
| --------------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ------------------------------------------------------------------ |
| **Continuing after a missing argument** | `argv[1]` is the null terminator of the argument vector, so `atoi(argv[1])` dereferences a null pointer and the process faults. The later `echo OK` may still run, but the required diagnostic is absent. | A diagnostic must be paired with leaving the error path.           |
| **Using `printf` for a diagnostic**     | The message goes to standard output.                                                                                                                                                                      | Descriptor 2 keeps diagnostics separate from normal output.        |
| **Converting ticks to seconds**         | The requested wait is scaled incorrectly.                                                                                                                                                                 | The syscall already accepts ticks.                                 |
| **Polling `uptime()`**                  | The syscall breakpoint is never reached, and the process remains runnable.                                                                                                                                | The exercise requires kernel-managed waiting.                      |
| **Omitting `UPROGS`**                   | The shell reports `exec sleep failed`.                                                                                                                                                                    | Only named programs enter `fs.img`; this entry is already present. |
| **Using host-library APIs**             | Compilation fails under `-ffreestanding -nostdlib`.                                                                                                                                                       | `user/user.h` is the complete user API.                            |

## Verifying it

Build and exercise both paths in the guest:

```bash
make qemu
```

```text
$ sleep
$ sleep 10
$ echo OK
```

Run the focused grader:

```bash
./grade-lab-util sleep
```

Failures leave transcripts in `xv6.out.sleep` and `xv6.out.sleep_no_args`; [`03-lab-workflow.md`](../03-lab-workflow.md#grading) explains how to read them.

For a direct boundary check, run `make CPUS=1 qemu-gdb`, connect `gdb-multiarch kernel/kernel`, break on `sys_pause`, and inspect `n` after `argint(0, &n)`. Hitting the breakpoint confirms that `sleep 10` reached the kernel handler; it does not establish the duration or prove that no other implementation path consumed CPU.

## Questions

1. Why does `sys_pause(void)` receive no C parameter when `pause(int)` does, and where is the integer stored across the trap?
2. What failure occurs if the program reads `argv[1]` when only `argv[0]` exists?
3. Why must the wait condition be a `while` rather than an `if` when two processes request different delays?
4. How does registering `&ticks` before releasing `tickslock` prevent a lost wakeup?
5. What happens from `kkill()` through `usertrap()` when the target is already sleeping in `sys_pause()`?
6. Why does unsigned subtraction handle tick-counter wraparound while a comparison against `ticks0 + n` may not?
