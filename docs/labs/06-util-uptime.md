# Lab util: Deriving `uptime`

---

[toc]

---

The first optional challenge in MIT's [2026 util lab](https://pdos.csail.mit.edu/6.1810/2026/labs/util.html) asks for a guest program that reports how long xv6 has been running, measured in clock ticks, through the existing `uptime` system call. This note explains that call and the tick counter behind it, then leaves `user/uptime.c` to the reader.

> [!IMPORTANT]
> No solution or pseudocode before the end: the C quotation below is existing kernel code from `kernel/sysproc.c`, and the target gets a contract, source-backed decisions, and traps. The one exception is the final [Solution walkthrough](#solution-walkthrough), a marked spoiler; stop before it if you are still deriving.

## What the exercise asks for

### Contract

| Aspect                 | Requirement or local convention                                                                                                                                                 |
| ---------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **Program**            | Handout: write a program named `uptime`. Repository: put it at `user/uptime.c`, so its build product `user/_uptime` enters `fs.img`.                                            |
| **Source of the time** | Handout: obtain the value from the `uptime` system call; do not count or estimate it in user space.                                                                             |
| **Unit**               | Handout: print the value in ticks. Converting to seconds is not requested; see [What a tick means](01-util-sleep.md#what-a-tick-means) for why it is not a guarantee either.    |
| **Arguments**          | The handout specifies none. The local grader requires rejection: `uptime extra` must print the line `Usage: uptime` and no reading.                                             |
| **Output**             | The handout fixes no format. The local grader requires one line of the form `Uptime: N ticks`, with `N` a decimal number.                                                       |
| **Available APIs**     | Repository: only [`user/user.h`](../../user/user.h); there is no host C library or `<time.h>`.                                                                                  |
| **Build registration** | Repository: this tree lists `uptime` in `LAB_PRACTICE_PROGS` in the [Makefile](../../Makefile), which adds `$U/_uptime` to `UPROGS` and compiles it with `STRICT_CFLAGS`.       |
| **Exit status**        | Repository convention: `0` on success. `uptime()` [cannot fail](../05-syscall-reference.md#time-and-system), so a failure status would come only from your own argument policy. |

### What the grader checks

> [!NOTE]
> **Local tests, not MIT's.** The handout leaves the optional challenges ungraded. The four `uptime` tests in [`grade-lab-util`](../../grade-lab-util) were added in this repository, total **20 points**, and define the output format and argument policy in the [contract](#contract).

| Test                                | Points | Commands                       | Positive assertion                                                 | Negative assertion           |
| ----------------------------------- | ------ | ------------------------------ | ------------------------------------------------------------------ | ---------------------------- |
| **`uptime, prints ticks`**          | 5      | `uptime`, then `echo OK`       | Lines matching `^Uptime: [0-9]+ ticks$` and `^OK$`                 | `exec .* failed`             |
| **`uptime, makes syscall`**         | 5      | `uptime`, then `echo FAIL`     | `\$ uptime`, with `stop_breakpoint('sys_uptime')` installed        | `FAIL`, `^Uptime:`           |
| **`uptime, advances across sleep`** | 5      | `uptime`, `sleep 10`, `uptime` | Exactly two `Uptime: N ticks` lines, the second at least 10 larger | No separate pattern          |
| **`uptime, rejects arguments`**     | 5      | `uptime extra`, then `echo OK` | Lines matching `^Usage: uptime$` and `^OK$`                        | `exec .* failed`, `^Uptime:` |

The syscall test forbids `^Uptime:` on purpose. [The sleep note](01-util-sleep.md#what-the-grader-does-not-prove) records that its own breakpoint test cannot tell a breakpoint stop from a timeout; here, a program that never enters `sys_uptime()` prints its line and fails. The lower bound of 10 is a guarantee, not a tolerance: `sys_pause()` samples `ticks` after the first reading and returns only after 10 more have elapsed.

### What the grader does not prove

| Evidence                          | What remains unchecked                                                                                     |
| --------------------------------- | ---------------------------------------------------------------------------------------------------------- |
| **The breakpoint stops the test** | That the printed number is the handler's return value; the stop comes before anything is printed.          |
| **The second reading is larger**  | An upper bound or the unit; only `clockintr()` in [`kernel/trap.c`](../../kernel/trap.c) defines one tick. |
| **`Usage: uptime` appears**       | The exit status, or that the message went to fd 2; the console shows both descriptors alike.               |

## Prerequisites

| When                            | Read                                                                                                         | Why it matters here                                                                                   |
| ------------------------------- | ------------------------------------------------------------------------------------------------------------ | ----------------------------------------------------------------------------------------------------- |
| **Read first**                  | [`01-util-sleep.md`](01-util-sleep.md)                                                                       | The same four layers, the same `ticks` counter, and the trap path, already explained for `pause`.     |
| **Read first**                  | [`02-build-boot-and-usage.md`](../02-build-boot-and-usage.md#adding-and-running-a-user-exercise)             | User-program layout, headers, and `UPROGS` registration.                                              |
| **Use when checking semantics** | [`05-syscall-reference.md`](../05-syscall-reference.md#time-and-system)                                      | The `uptime` row: declaration, unit, and failure behavior.                                            |
| **Use when choosing a format**  | [`05-syscall-reference.md`](../05-syscall-reference.md#printf-conversions-and-the-missing-bounded-string)    | Which `printf` conversions exist in this tree and which argument type each reads.                     |
| **Use for the trap mechanism**  | [The foundation section of `01-util-sleep.md`](01-util-sleep.md#the-foundation-a-c-call-that-becomes-a-trap) | Holds the trap-path material on loan until [ch04](../book/ch04-traps-and-system-calls.md) is written. |

> [!NOTE]
> Host `uptime(1)` prints wall-clock time and load averages read from `/proc/uptime`. That is a different program on a different kernel; the xv6 exercise is a tick count from one system call.

## The code to read first

| File                                         | Contributes                                                                                              | Does not contribute                                  |
| -------------------------------------------- | -------------------------------------------------------------------------------------------------------- | ---------------------------------------------------- |
| [`user/user.h`](../../user/user.h)           | Declares `int uptime(void)`: no arguments, and the result is the return value.                           | No unit, no conversion helper.                       |
| [`user/usys.pl`](../../user/usys.pl)         | `entry("uptime")` generates the stub in `user/usys.S` that loads `SYS_uptime` (14) and executes `ecall`. | No policy; nothing to edit for this exercise.        |
| [`kernel/sysproc.c`](../../kernel/sysproc.c) | `sys_uptime()`, quoted below.                                                                            | No formatting; the kernel returns a bare integer.    |
| [`kernel/trap.c`](../../kernel/trap.c)       | `clockintr()`, which increments `ticks` and schedules the next timer interrupt.                          | No wall-clock time; there is no real-time clock API. |
| [`user/printf.c`](../../user/printf.c)       | The `%d` and `%u` conversions.                                                                           | No field width or padding.                           |

The stub, `a7`, and dispatch are identical to `pause`'s; [the sleep note](01-util-sleep.md#the-four-existing-layers) walks them once, and [docs 06](../06-build-artifacts.md#the-four-line-stub-decoded) owns the stub itself.

## `sys_uptime()` line by line

This is the complete, unchanged handler from [`kernel/sysproc.c`](../../kernel/sysproc.c):

```c
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}
```

| Line or value                   | Consequence                                                                                                                                   |
| ------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------- |
| **No `argint()` call**          | The handler reads no saved argument register, so anything a caller might pass is ignored.                                                     |
| **`acquire(&tickslock)`**       | The same lock `clockintr()` holds while it increments `ticks`, so the read never observes a half-finished update.                             |
| **`xticks = ticks`**            | A snapshot. By the time the user program prints it, more ticks may have elapsed.                                                              |
| **`uint` returned as `uint64`** | `syscall()` in [`kernel/syscall.c`](../../kernel/syscall.c) stores the result in the saved `a0`; the user declaration then reads it as `int`. |

Compare `sys_pause()` in the [sleep note](01-util-sleep.md#syspause-line-by-line): both take `tickslock` and read `ticks`, but `uptime` returns immediately instead of sleeping on `&ticks`.

## The concept underneath

| Constant or fact            | Value in this tree                                    | Source                                                                                                                |
| --------------------------- | ----------------------------------------------------- | --------------------------------------------------------------------------------------------------------------------- |
| **System-call number**      | `SYS_uptime` = 14                                     | [`kernel/syscall.h`](../../kernel/syscall.h)                                                                          |
| **Who increments `ticks`**  | Hart 0 only, in `clockintr()`                         | [`kernel/trap.c`](../../kernel/trap.c)                                                                                |
| **Timer interval**          | `1000000` timebase units, "about a tenth of a second" | `clockintr()` in [`kernel/trap.c`](../../kernel/trap.c) and `timerinit()` in [`kernel/start.c`](../../kernel/start.c) |
| **Kernel type of `ticks`**  | `uint`                                                | [`kernel/defs.h`](../../kernel/defs.h)                                                                                |
| **User type of the result** | `int`                                                 | [`user/user.h`](../../user/user.h)                                                                                    |

> [!CAUTION]
> The tenth-of-a-second figure is an observation about the current timer constant and QEMU's timebase, not part of the interface. Print ticks, as the handout asks.

The counter starts at zero when the kernel boots, so a fresh `make qemu` followed by `uptime` reports a small number that includes boot and the time you took to type. Restarting QEMU resets it; starting a new shell with `Ctrl-d` does not, because the kernel keeps running.

The `uint`-to-`int` boundary is the one portability edge. Values at or above 2^31 ticks would print as negative with `%d`; at the current interval that is years of uptime, so it is a type observation rather than a practical failure. Choosing `%d` or `%u` is a decision for the [derivation](#deriving-it), and [docs 05](../05-syscall-reference.md#printf-conversions-and-the-missing-bounded-string) records which argument type each reads.

## Deriving it

**Contract:** use the [requirements above](#contract); settle each question from the named source.

| Decision for `user/uptime.c` | Source-backed question to settle                                                                                                                                                    |
| ---------------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **Obtain the value**         | What does [`user/user.h`](../../user/user.h) say `uptime` takes and returns?                                                                                                        |
| **Print it**                 | Which [`printf` conversion](../05-syscall-reference.md#printf-conversions-and-the-missing-bounded-string) matches the declared type, and which descriptor should normal output use? |
| **Handle extra arguments**   | How will the program reject them, as the [contract](#contract) requires, when nothing in `sys_uptime()` reads an argument?                                                          |
| **Finish**                   | Which of `exit(0)` and `return 0` matches [the repository convention](../02-build-boot-and-usage.md#house-style-for-a-new-user-program)?                                            |

| Trap                                            | Observable consequence                                                                                                                                                            | Source to inspect                                                        |
| ----------------------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ------------------------------------------------------------------------ |
| **Printing seconds as if guaranteed**           | The output is wrong on any build that changes the timer constant.                                                                                                                 | [`kernel/trap.c`](../../kernel/trap.c)                                   |
| **Expecting host `uptime` output**              | Effort spent on load averages or hours that xv6 cannot supply.                                                                                                                    | [`user/user.h`](../../user/user.h)                                       |
| **Calling `pause()` to "measure" time**         | The program sleeps instead of reporting; `pause` waits for ticks, it does not return them.                                                                                        | [`05-syscall-reference.md`](../05-syscall-reference.md#time-and-system)  |
| **Leaving `user/uptime.c` empty**               | The Makefile entry already exists, so `make` fails: `STRICT_CFLAGS` rejects an empty translation unit, and without them the linker reports `start()` calling an undefined `main`. | [`Makefile`](../../Makefile), [`user/ulib.c`](../../user/ulib.c)         |
| **Declaring `argv` but never using it**         | `STRICT_CFLAGS` adds `-Wextra`, so `make` fails with `unused parameter 'argv' [-Werror=unused-parameter]`.                                                                        | [`Makefile`](../../Makefile)                                             |
| **Including a host header such as `<stdio.h>`** | The freestanding build fails; `printf` comes from `user/user.h`.                                                                                                                  | [docs 06](../06-build-artifacts.md#why-you-cannot-use-the-hosts-headers) |

## Verifying it

1. Build and boot:

    ```bash
    make qemu
    ```

2. In the guest, run it twice around a known wait, then with an extra word; the difference should be at least the `pause` count:

    ```text
    uptime
    sleep 10
    uptime
    uptime extra
    ```

3. Quit QEMU with `Ctrl-a x`, then run the four focused tests and the full suite:

    ```bash
    ./grade-lab-util uptime
    make grade
    ```

These are checks to run, not a transcript. The difference in step 2 exceeds 10 by however long the second command took to type. Use [the evidence limits](#what-the-grader-does-not-prove) when reading a pass. The focused failure logs are `xv6.out.uptime`, `xv6.out.uptime_syscall`, `xv6.out.uptime_advances`, and `xv6.out.uptime_args`; [docs 03](../03-lab-workflow.md#grading) owns transcript handling, and `make grade` fails if another QEMU already holds `fs.img`.

To observe the system call itself rather than its printed result, stop in the handler:

1. Start the halted guest with one hart in terminal 1:

    ```bash
    make CPUS=1 qemu-gdb
    ```

2. Connect in terminal 2, set the breakpoint, and continue through boot:

    ```bash
    gdb-multiarch -x .gdbinit kernel/kernel
    ```

    ```gdb
    break sys_uptime
    continue
    ```

3. Run `uptime` in the guest. When GDB stops, inspect the counter and the caller's saved call number, then finish the handler to see the return value:

    ```gdb
    print ticks
    frame 1
    print p->trapframe->a7
    finish
    ```

Saved `a7` should be 14. `finish` prints the handler's return value, which is the `ticks` snapshot; time spent stopped in GDB makes it a poor wall-clock comparison. [Docs 09](../09-debugging-xv6.md#manually-debugging-an-xv6-user-program) explains stopping in user code instead.

## Questions

1. Which source line in [`kernel/sysproc.c`](../../kernel/sysproc.c) shows that `uptime` takes no arguments, and what happens to a value a caller leaves in `a0`?
2. Why does `sys_uptime()` take `tickslock` to read a single integer, and which function holds the same lock while writing it?
3. On a four-hart boot, how many harts increment `ticks`, and why does the count not run four times faster?
4. If you replaced the timer constant `1000000` with `100000`, what would `uptime` print after the same wall-clock wait, and would your program need to change?
5. Failure scenario: `user/uptime.c` exists but is empty. Which flag in the [Makefile](../../Makefile) makes compilation fail first, and which function in [`user/ulib.c`](../../user/ulib.c) would fail to link without it?
6. Failure scenario: a program prints `uptime()` with `%x`. Is the number wrong, or only its base?
7. Why does `Ctrl-d` at the shell prompt not reset the value `uptime` reports?
8. Why does the `uptime, makes syscall` test forbid `^Uptime:`, and what would pass without that pattern?

## Solution walkthrough

> [!CAUTION]
> **Spoiler.** This section explains the finished [`user/uptime.c`](../../user/uptime.c), which passes the four local `uptime` tests. Stop here if you are still deriving it from the sections above.

| Step in `main()`     | What [`user/uptime.c`](../../user/uptime.c) does                               | Why                                                                                       |
| -------------------- | ------------------------------------------------------------------------------ | ----------------------------------------------------------------------------------------- |
| **Check `argc`**     | Anything other than `1` prints `Usage: <argv[0]>` on fd 2 and calls `exit(1)`. | `sys_uptime()` would ignore the words; printing `argv[0]` also uses `argv` for `-Wextra`. |
| **Read the counter** | Calls `uptime()` once, directly inside the `printf` argument list.             | One snapshot; there is nothing to validate, because the call cannot fail.                 |
| **Print**            | `printf("Uptime: %d ticks\n", ...)` on fd 1.                                   | `%d` matches the `int` declaration in `user/user.h`; the unit stays in the output.        |
| **Finish**           | `exit(0)`.                                                                     | The repository's success convention.                                                      |

The program deliberately prints no seconds. A conversion would hard-code the `1000000` timer constant from `clockintr()`, which the [caution above](#the-concept-underneath) says is an observation, not an interface. The `uint`-to-`int` narrowing is accepted rather than worked around with `%u`: it matters only beyond 2^31 ticks, and `%d` keeps the format string consistent with the declared return type.

| Decision                   | What `user/uptime.c` does                                 |
| -------------------------- | --------------------------------------------------------- |
| **Obtain the value**       | `uptime()`, declared `int uptime(void)` in `user/user.h`. |
| **Print it**               | `%d` to fd 1, labelled with its unit.                     |
| **Handle extra arguments** | Rejects them with a usage line on fd 2 and status `1`.    |
| **Finish**                 | `exit(0)`, matching `user/sleep.c`.                       |
