# Lab syscall answers: Using gdb

---

[toc]

---

Answers to the six questions in the [syscall lab](https://pdos.csail.mit.edu/6.1810/2026/labs/syscall.html)'s _Using gdb_ exercise. `answers-syscall.txt` at the repository root, the file `grade-lab-syscall` reads, is a symlink to this file.

> [!NOTE]
> Measured on 2026-11-04 with `LAB=syscall` and 3 harts. Addresses and registers belong to this build; a rebuild can move them. [The lab note's solution walkthrough](12-syscall-using-gdb.md#solution-walkthrough) has the full transcripts and screenshots.

## Answers at a glance

| #     | Question                                          | Answer                                                 |
| ----- | ------------------------------------------------- | ------------------------------------------------------ |
| **1** | Which function called `syscall`?                  | `usertrap()`, at `kernel/trap.c:76`                    |
| **2** | Value of `p->trapframe->a7`, and what it means    | `15` (`0xf`): `SYS_open`, init's first system call     |
| **3** | The CPU's previous mode                           | User mode: `SPP` (bit 8) of `sstatus` is 0             |
| **4** | Faulting instruction, and the register for `num`  | `lw a3,0(zero)` at `0x80001d58`; `num` is in `a3`      |
| **5** | Why the kernel crashes; does `scause` confirm it? | Address 0 is unmapped in the kernel; yes, `scause=0xd` |
| **6** | Process running at the panic, and its pid         | `init`, pid `1`                                        |

## 1. The caller of `syscall`: `usertrap()`

`bt` at the `b syscall` breakpoint shows frame `#1` as `usertrap()`. Frame `#2` is unnamed because it is the trampoline page, mapped at `TRAMPOLINE` rather than at the address `kernel/kernel` links it at.

```gdb
(gdb) bt
#0  syscall () at kernel/syscall.c:146
#1  0x0000000080001b02 in usertrap () at kernel/trap.c:76
#2  0x0000003ffffff09c in ?? ()
```

## 2. `p->trapframe->a7`: `15`, the number for `SYS_open`

**`a7` holds the system-call number the user stub loaded before `ecall`.** At the first stop after boot, the caller is `init`, and its first call is `open("console", O_RDWR)` in `user/init.c`. The `open` stub in `user/init.asm` executes `li a7,15`, and `kernel/syscall.h` defines `SYS_open` as 15.

```gdb
(gdb) p p->name
$1 = "init", '\000' <repeats 11 times>
(gdb) p /x p->trapframe->a7
$3 = 0xf
```

> [!WARNING]
> `13` (`SYS_pause`) is what a later stop shows, for example after typing `sleep 10`. That stop belongs to the `sleep` process, pid 3, not to the first system call.

## 3. The previous mode: user mode

`p /x $sstatus` printed `0x200000022`. Decode it with the masks in `kernel/riscv.h`:

| Bits           | Field  | Set? | Meaning                                          |
| -------------- | ------ | ---- | ------------------------------------------------ |
| **Bit 8**      | `SPP`  | No   | Previous mode: 0 = User. **This is the answer.** |
| **Bit 5**      | `SPIE` | Yes  | Supervisor Previous Interrupt Enable             |
| **Bit 1**      | `SIE`  | Yes  | Supervisor Interrupt Enable                      |
| **Bits 33:32** | UXL    | `2`  | Read-only: user XLEN is 64 bits                  |

## 4. The faulting instruction and `num`'s register

The console printed `scause=0xd sepc=0x80001d58 stval=0x0`. Looking up `sepc` in `kernel/kernel.asm` gives:

```text
80001d58:	00002683          	lw	a3,0(zero) # 0 <_entry-0x80000000>
```

**`num` is in register `a3`**, the load's destination. `x/i $pc` at `b *0x80001d58` shows the same instruction.

## 5. Why the kernel crashes

1. **The load reads virtual address 0.** `*(int *)0` makes 0 an address and dereferences it.
2. **Address 0 is not mapped in the kernel's address space.** `kvmmake()` in `kernel/vm.c`, drawn as Figure 3.3 in the xv6 book, maps nothing below `PLIC` at `0x0c000000`.
3. **`scause` confirms it.** `0xd` (13) is a load page fault in Table 4.2 of the [RISC-V privileged spec](../riscv-privileged-20211203.pdf), and `stval=0x0` is the faulting address.
4. **The fault is fatal because it happened in S-mode.** It goes to `kerneltrap()`, which prints the CSRs and calls `panic("kerneltrap")`; a user-mode fault would go to `usertrap()`, which kills only the process.

## 6. The process at the panic: `init`, pid 1

At `b *0x80001d58`, the stop just before the faulting load, `syscall()`'s `p` is still in scope:

```gdb
(gdb) p p->name
$1 = "init", '\000' <repeats 11 times>
(gdb) p p->pid
$2 = 1
```

This matches question 2: the first system call after boot comes from `init`.
