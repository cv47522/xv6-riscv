# xv6 Terminology and Technical Vocabulary

---

[toc]

---

Every acronym and piece of jargon you meet in xv6, the [xv6 book](https://mit-pdos.github.io/xv6-riscv-book/), and the 6.1810 labs — expanded, and explained in terms of where it appears in this repository.

This is the hub the other guides link into. When a book chapter introduces a new term, add it here once rather than re-explaining it in chapter notes.

## xv6, RISC-V, and QEMU: names and roles

**xv6** — "**x**86 **v**ersion **6**". A re-implementation of Dennis Ritchie and Ken Thompson's **Unix Version 6** (1975), written at MIT for teaching. The name kept its `x86` origin even after the port to RISC-V, which is why this repository is `xv6-riscv` rather than something tidier. It is deliberately small: around 9,000 lines, small enough that one person can read all of it.

**RISC-V** — "**R**educed **I**nstruction **S**et **C**omputer, **f**ive", pronounced "risk five". The fifth RISC design from UC Berkeley. Unlike x86 or ARM it is an open standard: the specification is free to implement, which is why it is a good teaching target. xv6 targets **RV64GC** — 64-bit, with the G and C extension bundles.

**QEMU** — "**Q**uick **EMU**lator". Emulates a whole computer in software. `qemu-system-riscv64` pretends to be a RISC-V machine so xv6 can boot on your x86 laptop. xv6 runs on QEMU's `virt` board, a synthetic machine whose memory layout is documented and hard-coded into `kernel/memlayout.h`.

## RISC-V architecture

For reusable definitions and instruction practice, see [the assembly course’s register lesson](../../assembly-language/docs/riscv/01-values-registers-instructions.md). The sibling [assembly glossary](../../assembly-language/docs/general/terminology.md) covers shared vocabulary; the entries below retain xv6-specific symbols and usage.

| Term                         | Expansion                                                         | Meaning in xv6                                                                                                                                                                                                                                                          |
| ---------------------------- | ----------------------------------------------------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **hart**                     | **har**dware **t**hread                                           | One independently executing RISC-V register and control-state set, commonly exposed as a logical CPU. See [harts versus cores](#harts-versus-physical-cores).                                                                                                           |
| **RV64GC**                   | RISC-**V** **64**-bit, **G** + **C**                              | The target ISA. **G** is shorthand for IMAFD: **I**nteger, **M**ultiply/divide, **A**tomic, single-precision **F**loat, **D**ouble. **C** is compressed 16-bit instructions. Set by `-march=rv64gc` in the Makefile.                                                    |
| **ISA**                      | **I**nstruction **S**et **A**rchitecture                          | The contract between hardware and software: what instructions exist and what they do.                                                                                                                                                                                   |
| **CSR**                      | **C**ontrol and **S**tatus **R**egister                           | Special registers holding privileged state. `kernel/riscv.h` is mostly accessors for these.                                                                                                                                                                             |
| **satp**                     | **S**upervisor **A**ddress **T**ranslation and **P**rotection     | The CSR holding the root page table's physical address. Writing it switches address spaces.                                                                                                                                                                             |
| **stvec**                    | **S**upervisor **T**rap **VEC**tor                                | Where the CPU jumps on a trap. Set to `kernelvec` or `trampoline` depending on the current mode.                                                                                                                                                                        |
| **PC**                       | **P**rogram **C**ounter                                           | The address of the instruction currently executing. RISC-V has no general-purpose register holding it; on a trap the hardware copies it into `sepc`. Turning a raw PC back into a function and a line of C is what `kernel/kernel.sym` and `kernel/kernel.asm` are for. |
| **mepc**                     | **M**achine **E**xception **P**rogram **C**ounter                 | The M-mode twin of `sepc`: `mret` jumps to it. `start()` writes `main` there so `mret` lands in `main()`.                                                                                                                                                               |
| **epc** (trapframe field)    | **e**xception **p**rogram **c**ounter                             | Not a CSR: `p->trapframe->epc` holds the user PC that `usertrap()` copies from `sepc`, advancing it by 4 past an `ecall` so the call is not repeated.                                                                                                                   |
| **sepc**                     | **S**upervisor **E**xception **P**rogram **C**ounter              | The PC at the moment a trap occurred, so execution can resume.                                                                                                                                                                                                          |
| **scause**                   | **S**upervisor **CAUSE**                                          | Why the trap happened — system call, page fault, timer interrupt.                                                                                                                                                                                                       |
| **sstatus**                  | **S**upervisor **STATUS**                                         | Interrupt-enable bits and the mode the CPU came from.                                                                                                                                                                                                                   |
| **stval**                    | **S**upervisor **T**rap **VAL**ue                                 | Extra detail about a trap. For a page fault it holds the address that could not be translated, which is usually the bad pointer itself. Printed alongside `scause` and `sepc` when the kernel gives up.                                                                 |
| **mret / sret**              | **M**achine / **S**upervisor **RET**urn                           | Trap-return jumps that can lower privilege: `mret` uses `mepc` and `mstatus.MPP`, `sret` uses `sepc` and `sstatus.SPP`. `start()` ends with `mret`; `userret` ends with `sret`.                                                                                         |
| **mstatus / MPP**            | **M**achine **STATUS** / **M**achine **P**revious **P**rivilege   | `MPP` selects the mode that `mret` enters; `start()` sets it to S-mode.                                                                                                                                                                                                 |
| **mhartid**                  | **M**achine **HART** **ID**                                       | Read-only M-mode CSR holding this hart's number. `_entry` uses it to pick a stack slice; `start()` copies it into `tp` because S-mode cannot read it.                                                                                                                   |
| **medeleg / mideleg**        | **M**achine **E**xception / **I**nterrupt **DELEG**ation          | Bit masks that route traps to S-mode instead of M-mode. `start()` sets both to `0xffff`.                                                                                                                                                                                |
| **stimecmp**                 | **S**upervisor **TIME** **C**o**MP**are                           | The Sstc timer deadline: a timer interrupt fires once `time` reaches it. `timerinit()` in `kernel/start.c` and `clockintr()` in `kernel/trap.c` write it.                                                                                                               |
| **PMP**                      | **P**hysical **M**emory **P**rotection                            | M-mode access rules for physical memory; `start()` opens all of it to S-mode through `pmpaddr0` and `pmpcfg0`.                                                                                                                                                          |
| **sfence.vma**               | **S**upervisor **FENCE** for **V**irtual **M**emory **A**ddresses | Orders page-table writes and flushes stale TLB entries; `kvminithart()` brackets its `satp` write with it.                                                                                                                                                              |
| **ecall**                    | **e**nvironment **call**                                          | The instruction that makes a system call. `user/usys.S` loads the syscall number into `a7`, then executes `ecall`. See [Why the system call number goes in `a7`](06-build-artifacts.md#why-the-system-call-number-goes-in-a7).                                          |
| **Sv39**                     | **S**upervisor **v**irtual, **39**-bit                            | The paging scheme: 39-bit virtual addresses, three levels of page table, 4 KiB pages.                                                                                                                                                                                   |
| **PTE**                      | **P**age **T**able **E**ntry                                      | One mapping from a virtual page to a physical one, plus permission bits.                                                                                                                                                                                                |
| **PPN**                      | **P**hysical **P**age **N**umber                                  | A physical address with the low 12 bits (the offset) dropped.                                                                                                                                                                                                           |
| **TLB**                      | **T**ranslation **L**ookaside **B**uffer                          | The CPU's cache of recent translations. Flushed with `sfence.vma` after changing page tables.                                                                                                                                                                           |
| **M-mode / S-mode / U-mode** | **M**achine, **S**upervisor, **U**ser                             | The three privilege levels. xv6 boots in M-mode (`kernel/start.c`), drops to S-mode for the kernel, and runs user programs in U-mode.                                                                                                                                   |
| **PLIC**                     | **P**latform-**L**evel **I**nterrupt **C**ontroller               | Routes device interrupts (UART, disk) to harts. `kernel/plic.c`.                                                                                                                                                                                                        |
| **CLINT**                    | **C**ore-**L**ocal **INT**erruptor                                | Provides timer interrupts, which drive preemptive scheduling.                                                                                                                                                                                                           |

### Harts versus physical cores

A **hart** is not always a physical core: simultaneous multithreading can expose several harts from one core. QEMU creates `CPUS` virtual CPUs for this machine, and xv6 identifies each as a hart.

- **Boot messages:** `kernel/main.c` reports the harts starting.
- **Supported count:** `NCPU` in `kernel/param.h` caps how many xv6 supports.

### Integer register names: `sp`, `ra`, `tp`, and the ABI groups

**ABI names describe a register's calling-convention role; `x0`–`x31` are the hardware numbers.** The trapframe and `struct context` in `kernel/proc.h` use the ABI names.

| Register      | Expansion                              | Role in xv6                                                                                                                                              |
| ------------- | -------------------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **sp** (`x2`) | **s**tack **p**ointer                  | The current stack top; stacks grow down. `_entry` points it into `stack0` before any C runs, and the trapframe saves the user value at offset 48.        |
| **ra** (`x1`) | **r**eturn **a**ddress                 | Where `ret` jumps. `allocproc()` sets `p->context.ra = forkret`, so the first `swtch()` into a new process "returns" into `forkret()`.                   |
| **tp** (`x4`) | **t**hread **p**ointer                 | Repurposed to hold the hart id: `start()` writes `mhartid` into it and `cpuid()` reads it back.                                                          |
| **a0–a7**     | **a**rgument registers                 | `a0`–`a5` carry system-call arguments, `a7` the system-call number, and `a0` the result.                                                                 |
| **s0–s11**    | **s**aved (callee-saved) registers     | A called function must preserve them, which is why `struct context` holds only these plus `ra` and `sp`. `s0` doubles as the frame pointer `fp`.         |
| **t0–t6**     | **t**emporary (caller-saved) registers | Scratch values a call may destroy; a trap still saves them because it can interrupt any instruction. See the [Q/A in `kernel/proc.h`](../kernel/proc.h). |

## Compilers, debuggers, libraries, and device terms

| Term                    | Expansion                                                   | Role                                                                                                                                                                                                                                                                                                                             |
| ----------------------- | ----------------------------------------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **GCC**                 | **G**NU **C**ompiler **C**ollection                         | The compiler. Here in cross form: `riscv64-linux-gnu-gcc`.                                                                                                                                                                                                                                                                       |
| **binutils**            | **bin**ary **utils**                                        | `ld`, `as`, `objdump`, `objcopy`, `nm`.                                                                                                                                                                                                                                                                                          |
| **GDB**                 | **G**NU **D**e**B**ugger                                    | The debugger client. In the native notes it launches a host process; here it controls QEMU's emulated machine through the remote GDB protocol and labels machine addresses with separately loaded ELF symbols.                                                                                                                   |
| **target / debuggee**   | —                                                           | The execution being controlled by a debugger. In the native notes it is one host process; in this session it is QEMU's RISC-V machine, reached through QEMU's remote stub.                                                                                                                                                       |
| **remote GDB stub**     | —                                                           | The protocol server inside QEMU that exposes registers, memory, execution control, and emulated CPUs. It does not understand xv6 source files, `fs.img`, or processes; GDB supplies those labels from ELF symbols.                                                                                                               |
| **symbol table**        | —                                                           | ELF names mapped to addresses, such as `sys_pause` in `kernel/kernel`. Symbols let GDB name functions and globals; source-level stepping also needs debug information.                                                                                                                                                           |
| **debug information**   | —                                                           | Compiler-emitted DWARF records mapping machine addresses to source files, lines, types, and some variable locations. `-ggdb` adds it, but GDB can use it only after the containing ELF is loaded as a symbol file.                                                                                                               |
| **source breakpoint**   | —                                                           | A request to stop at a source location. It remains unresolved until GDB can map the file and line through loaded debug information.                                                                                                                                                                                              |
| **hardware breakpoint** | —                                                           | A stop implemented by the target's execution machinery rather than by patching an instruction into memory. `hbreak` asks QEMU's remote stub for this form.                                                                                                                                                                       |
| **GDB thread**          | —                                                           | One execution context reported by the debug target. QEMU reports emulated harts this way; the entry is not an xv6 process or a user-level software thread.                                                                                                                                                                       |
| **glibc**               | **GNU** **C** **Lib**rary                                   | Linux's standard C library: ISO C, POSIX, and the pre-`main` runtime. **Never** linked into anything xv6 builds, though its RISC-V headers are installed. See [What glibc actually is](06-build-artifacts.md#what-glibc-actually-is).                                                                                            |
| **ULIB**                | **u**ser **lib**rary                                        | A Makefile variable, not a real library: the four objects (`ulib.o`, `usys.o`, `printf.o`, `umalloc.o`) every user program links against. See [`ULIB` versus glibc](06-build-artifacts.md#ulib-versus-glibc).                                                                                                                    |
| **ELF**                 | **E**xecutable and **L**inkable **F**ormat                  | The binary container for `kernel/kernel` and each user program. The executable sections are loaded into the target; symbol and DWARF sections let tools label their addresses. `kernel/elf.h` defines the headers that `kernel/exec.c` parses. See [What ELF is](06-build-artifacts.md#elf-headers-segments-and-load-addresses). |
| **cross-compilation**   | —                                                           | Building on one architecture for another. Nothing built here runs natively on your x86 machine.                                                                                                                                                                                                                                  |
| **TOOLPREFIX**          | —                                                           | The Makefile variable holding the cross-tool prefix, e.g. `riscv64-linux-gnu-`.                                                                                                                                                                                                                                                  |
| **freestanding**        | —                                                           | A C environment with no operating system or standard library beneath it — exactly what a kernel is. Hence `-ffreestanding` and `-nostdlib`. It is ISO C's own term — see [Hosted versus freestanding](#hosted-versus-freestanding-the-part-of-iso-c-that-xv6-does-obey).                                                         |
| **PIE**                 | **P**osition **I**ndependent **E**xecutable                 | A binary that can load at any address. xv6 **must** _not_ be one, since it is linked to a fixed address with no dynamic loader. Hence `-fno-pie`.                                                                                                                                                                                |
| **medany**              | **med**ium-**any**                                          | The RISC-V code model allowing PC-relative addressing within ±2 GiB, needed because the kernel lives at `0x80000000`. See [Why the kernel is linked at `0x80000000`](06-build-artifacts.md#why-the-kernel-is-linked-at-0x80000000).                                                                                              |
| **TCG**                 | **T**iny **C**ode **G**enerator                             | QEMU's emulator engine: translates blocks of RISC-V guest code into x86 host code. It is the only accelerator in this QEMU build. See [How QEMU runs xv6 on an x86 host](11-os-design-in-xv6.md#how-qemu-runs-xv6-on-an-x86-host-tcg-not-a-board).                                                                               |
| **KVM**                 | **K**ernel-based **V**irtual **M**achine                    | Linux's hardware-virtualization interface (`/dev/kvm`). It runs guests of the host's own ISA natively, so it cannot run RISC-V xv6 on an x86 host.                                                                                                                                                                               |
| **hypervisor**          | —                                                           | Software that runs virtual machines. WSL2 runs inside Microsoft's Hyper-V; QEMU emulates a whole board instead of virtualizing one.                                                                                                                                                                                              |
| **virtio**              | **virt**ual **I/O**                                         | A standard for efficient emulated devices. xv6's only disk is a virtio block device; `kernel/virtio_disk.c` drives it.                                                                                                                                                                                                           |
| **MMIO**                | **M**emory-**M**apped **I/O**                               | Talking to devices by reading and writing special memory addresses rather than dedicated instructions.                                                                                                                                                                                                                           |
| **UART**                | **U**niversal **A**synchronous **R**eceiver/**T**ransmitter | The serial port. With `-nographic`, this is your terminal. QEMU emulates a 16550A-compatible chip, driven by `kernel/uart.c`; the datasheet is [`lectures/16550.pdf`](../lectures/16550.pdf).                                                                                                                                    |
| **KCSAN**               | **K**ernel **C**oncurrency **SAN**itizer                    | A data-race detector, enabled with `make KCSAN=1`.                                                                                                                                                                                                                                                                               |

## Standards and organisations

These names turn up constantly around glibc, `man` pages, and compiler flags, and they are almost **never** expanded. None of them is a piece of software; they are the bodies that write the rules, and the documents those bodies publish.

| Name        | Expansion                                                  | What it is, and where it touches this tree                                                                                                                                                                                                                                                                                     |
| ----------- | ---------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| **ISO**     | **I**nternational **O**rganization for **S**tandardization | The global standards body, founded 1947, based in Geneva. The short form is deliberately _not_ an initialism of the English name — it comes from the Greek _isos_, "equal", so it reads the same in every language.                                                                                                            |
| **IEC**     | **I**nternational **E**lectrotechnical **C**ommission      | The older body (1906) that owns electrical and electronic standards.                                                                                                                                                                                                                                                           |
| **ISO/IEC** | the two of them jointly                                    | ISO and IEC run one shared committee, **JTC 1**, for information technology, so IT standards carry both names. This is why the C standard is _ISO/IEC_ 9899 rather than ISO 9899.                                                                                                                                              |
| **ISO C**   | ISO/IEC 9899                                               | The C language standard itself: the grammar, the semantics, and the standard library. Successive editions are known by year — C89/C90, C99, C11, C17, C23. It defines what `printf` **must** do; it says nothing about `fork`.                                                                                                 |
| **ANSI C**  | **A**merican **N**ational **S**tandards **I**nstitute      | ANSI standardised C first, in 1989; ISO adopted that text essentially unchanged in 1990. "ANSI C", "C89", and "C90" all name the same language. The term survives out of habit, not because two standards exist.                                                                                                               |
| **POSIX**   | **P**ortable **O**perating **S**ystem **I**nterface        | IEEE Std 1003.1, also published as ISO/IEC/IEEE 9945: the specification of what a system must provide to count as Unix-like — `fork`, `exec`, file descriptors, signals, the shell, and the utilities. The trailing **X** evokes Unix; the name was Richard Stallman's suggestion when IEEE asked for something pronounceable. |
| **GNU**     | **GNU**'s **N**ot **U**nix                                 | A recursive acronym, and a project: Stallman's 1983 effort to build a complete free Unix-like system. It produced GCC, glibc, binutils, `make`, `bash`, and coreutils. Its own kernel, Hurd, **never** shipped usably, which is why the ordinary system is a GNU userland on a Linux kernel — "GNU/Linux".                     |

The division of labour between the middle rows is the one worth holding onto, because it is the same split as `man` sections 2 and 3, and the same split as [`ULIB` versus glibc](06-build-artifacts.md#ulib-versus-glibc):

- **ISO C** governs the language and the library. `strlen`, `malloc`, `printf`, `memmove`.
- **POSIX** governs the operating system underneath. `fork`, `open`, `read`, `pipe`, `exec`.

glibc implements both, which is why one library satisfies two unrelated standards bodies. xv6 implements small, incompatible pieces of each and claims neither.

### Hosted versus freestanding: the part of ISO C that xv6 does obey

ISO C anticipates being used without an operating system, and defines two kinds of implementation to say so. That distinction is not a footnote — it is exactly what `-ffreestanding` selects, and it is the standard's own blessing for everything this tree does.

|                              | **Hosted**                                                          | **Freestanding**                                                                                                                                               |
| ---------------------------- | ------------------------------------------------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **Where the program starts** | `main()`, **always**                                                | Wherever the implementation says. Here: `start()` in `user/ulib.c`, and `_entry` for the kernel                                                                |
| **Headers guaranteed**       | All of them — `<stdio.h>`, `<stdlib.h>`, `<string.h>`, and the rest | A short list that declares no functions at all: `<stdarg.h>`, `<stddef.h>`, `<limits.h>`, `<stdint.h>`, `<float.h>` (the exact set varies slightly by edition) |
| **Who supplies them**        | The C library — glibc                                               | The compiler — GCC                                                                                                                                             |
| **In this repository**       | `mkfs/mkfs.c`, built with the host compiler                         | Everything under `kernel/` and `user/`                                                                                                                         |

The proof is short enough to run. Exactly three files in the operating system include a standard header, and all three include the same one:

```bash
$ grep -rn '#include <' kernel/ user/
kernel/console.c:14:#include <stdarg.h>
kernel/printk.c:7:#include <stdarg.h>
user/printf.c:5:#include <stdarg.h>
```

And that header is not glibc's. Asking GCC to report what it opened:

```bash
$ riscv64-linux-gnu-gcc -ffreestanding -nostdlib -H -fsyntax-only t.c
. /usr/lib/gcc-cross/riscv64-linux-gnu/13/include/stdarg.h
```

**The path identifies GCC, rather than glibc, as the header supplier:**

| Include directory                 | Owner                                                                                    |
| --------------------------------- | ---------------------------------------------------------------------------------------- |
| `/usr/lib/gcc-cross/...`          | The compiler; this is where the command found `stdarg.h`.                                |
| `/usr/riscv64-linux-gnu/include/` | The sysroot [installed with glibc](06-build-artifacts.md#which-command-installed-glibc). |

ISO C requires a freestanding implementation to supply that header. The compiler must do so because `va_list` and `va_arg` cannot be written in portable C: traversing variadic arguments depends on the calling convention. The one standard header xv6 needs is therefore precisely one the library could never have supplied.

`mkfs/mkfs.c` is the control case — six angle-bracket includes, all genuinely glibc, because it is a hosted program that runs on your machine rather than inside QEMU.

### What xv6 conforms to

Nothing, on purpose. It is worth being blunt about this, because "Unix-like" invites the assumption that a POSIX manual page is authoritative here, and it is not.

| Standard                | Does xv6 meet it?                                                                                                                                                                                                           |
| ----------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **ISO C, hosted**       | No. There is no `<stdio.h>`, no `errno`, no `exit` status conventions. Only the freestanding subset applies.                                                                                                                |
| **ISO C, freestanding** | Yes, near enough, and by construction: `-ffreestanding -nostdlib` is the request to be judged by that rule instead.                                                                                                         |
| **POSIX**               | No. The system calls share POSIX's names and roughly its shapes, and then diverge in arity, flag values, and error reporting — catalogued in [Divergences from POSIX](05-syscall-reference.md#divergences-from-posix).      |
| **GNU**                 | Not applicable, and worth untangling: the entire toolchain is GNU — `gcc`, `ld`, `objdump`, `gdb`, `make` — while the one GNU component deliberately excluded is glibc. GNU tools all the way down, GNU library not at all. |

## Operating system concepts

| Term                      | Meaning in xv6                                                                                                                                                                                                                                                                                                                                                                                   |
| ------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| **host**                  | The Linux environment running VS Code, GDB, Make, and the QEMU host process. “Host” names the outside of the emulation boundary, not the privileged side of xv6.                                                                                                                                                                                                                                 |
| **guest**                 | The entire emulated xv6 machine. Both the xv6 kernel and its user programs are guest software, so “guest program” is broader than “user program.”                                                                                                                                                                                                                                                |
| **native program**        | A program compiled for and executed directly under the host operating system, such as the selected exercises in the C notes. xv6's RISC-V ELFs are not native to the x86 host.                                                                                                                                                                                                                   |
| **kernel**                | The shared privileged guest code linked into `kernel/kernel` and executed in S-mode for system calls, traps, interrupts, and kernel work. It is not an ordinary “kernel process.”                                                                                                                                                                                                                |
| **user program**          | Guest code linked as a separate ELF under `user/`, installed in `fs.img`, loaded by xv6 `exec`, and executed by an xv6 process in U-mode. It is guest software but not synonymous with all guest software.                                                                                                                                                                                       |
| **user space**            | The unprivileged virtual-address region and execution context in which an xv6 process runs its user program in U-mode. The source is usually under `user/`, but the term describes privilege and address-space context rather than a host directory.                                                                                                                                             |
| **kernel space**          | The privileged mappings and S-mode execution context shared by the xv6 kernel. A system call changes privilege and page-table context; it does not launch a separate kernel program.                                                                                                                                                                                                             |
| **process**               | xv6's software abstraction for one running program and its resources, represented by `struct proc` in `kernel/proc.h`. A process may run on a hart, wait without occupying one, or move between harts; it is not the hart itself.                                                                                                                                                                |
| **address space**         | The virtual addresses and mappings visible under one page table. Separate xv6 user processes reuse the same virtual addresses for different contents, which is why one loaded user ELF's labels can become misleading after a context switch.                                                                                                                                                    |
| **system call**           | The controlled entry from user space into the kernel. `user/usys.S` → `ecall` → `kernel/syscall.c`.                                                                                                                                                                                                                                                                                              |
| **stub**                  | Minimal code standing in for a full implementation: it forwards work elsewhere or holds a name open. See [the three stub roles](#system-call-gdb-and-placeholder-stubs).                                                                                                                                                                                                                         |
| **file descriptor (FD)**  | A small integer indexing the calling process's `ofile[NOFILE]` table in `kernel/proc.h`, naming one already-open file backed by an inode, a pipe, or a device such as the console. By convention 0 is standard input, 1 is standard output, and 2 is standard error. `NOFILE` is 16.                                                                                                             |
| **EOF**                   | **E**nd **o**f **f**ile: `read()` returns 0 for a positive requested length because no bytes remain at the current input endpoint. See [EOF by endpoint](#eof-for-files-pipes-and-the-console).                                                                                                                                                                                                  |
| **filter**                | A program that reads FD 0 and writes FD 1 without adding output of its own, so it composes in a pipeline. `user/ex1copy.c` is the minimal example; `user/wc.c` and `user/grep.c` are real ones.                                                                                                                                                                                                  |
| **pipe**                  | A one-way kernel byte buffer reached through two descriptors, a read end and a write end, created together by `pipe()`. `struct pipe` in `kernel/pipe.c` holds 512 bytes; a reader sleeps while it is empty and gets EOF only after every write descriptor is closed. See [ch01 Pipes](book/ch01-operating-system-interfaces.md#pipes).                                                          |
| **trap**                  | Any transfer to the kernel: system call, exception, or interrupt. `kernel/trap.c`.                                                                                                                                                                                                                                                                                                               |
| **panic**                 | The kernel's assert in `kernel/printk.c`: an invariant broke, and the kernel has no defined way to continue. See [panic scope and output](#panic-output-and-whole-machine-failure).                                                                                                                                                                                                              |
| **trampoline**            | Not an acronym: code you bounce through to land somewhere else. A page mapped at the same virtual address (`TRAMPOLINE`) in every address space, so the switch between user and kernel page tables can happen without the mapping vanishing mid-instruction. `kernel/trampoline.S` holds `uservec` and `userret`; see [kernel names](#kernel-function-and-symbol-names-prefixes-and-expansions). |
| **trapframe**             | Per-process page saving user registers at trap time, plus the `kernel_*` values `uservec` needs; mapped at `TRAPFRAME` just below the trampoline. See the Q/A above `struct trapframe` in `kernel/proc.h`.                                                                                                                                                                                       |
| **context switch**        | Swapping one process's registers for another's. `kernel/swtch.S`.                                                                                                                                                                                                                                                                                                                                |
| **spinlock**              | A lock that busy-waits. `kernel/spinlock.c`.                                                                                                                                                                                                                                                                                                                                                     |
| **sleeplock**             | A lock that yields the CPU while waiting; usable when the wait may be long, such as disk I/O. `kernel/sleeplock.c`.                                                                                                                                                                                                                                                                              |
| **inode**                 | The on-disk record describing a file: type, size, and where its blocks are. The number in `ls` output.                                                                                                                                                                                                                                                                                           |
| **superblock**            | Block 1, describing the filesystem's layout.                                                                                                                                                                                                                                                                                                                                                     |
| **logging / journalling** | Writing changes to a log before applying them, so a crash mid-update is recoverable. `kernel/log.c`.                                                                                                                                                                                                                                                                                             |
| **buffer cache**          | In-memory cache of disk blocks. `kernel/bio.c`.                                                                                                                                                                                                                                                                                                                                                  |
| **page fault**            | A trap raised when address translation fails or a mapping denies the attempted access. A valid, read-only mapping can fault on a write, as in the cow lab; mmap also uses page faults.                                                                                                                                                                                                                                                                                                   |
| **copy-on-write**         | Sharing pages between parent and child after `fork`, copying only when one writes.                                                                                                                                                                                                                                                                                                               |
| **demand paging**         | Allocating a page only when it is first touched.                                                                                                                                                                                                                                                                                                                                                 |
| **zombie**                | A process that has exited but whose parent has not yet called `wait`.                                                                                                                                                                                                                                                                                                                            |
| **orphan**                | A process whose parent exited first; re-parented to `init`.                                                                                                                                                                                                                                                                                                                                      |

### System-call, GDB, and placeholder stubs

**Standing in, rather than doing the full implementation, is the shared role.** This repository uses “stub” for three unrelated things:

| Stub kind            | Where it appears                       | What it stands in for                                                                  |
| -------------------- | -------------------------------------- | -------------------------------------------------------------------------------------- |
| **System-call stub** | Four-line stubs in `user/usys.S`       | Names the call and uses `ecall` to forward it to the kernel.                           |
| **GDB stub**         | QEMU's protocol server                 | Lets `gdb-multiarch` drive an emulated machine whose internals it knows nothing about. |
| **Stub file**        | Empty chapter notes under `docs/book/` | Holds a place for future content.                                                      |

See [The four-line stub, decoded](06-build-artifacts.md#the-four-line-stub-decoded).

### EOF for files, pipes, and the console

**EOF depends on the input endpoint.** A `read()` requesting a positive length returns 0 under these conditions:

| Endpoint         | When EOF occurs                                  |
| ---------------- | ------------------------------------------------ |
| **Regular file** | The read reaches the current inode end.          |
| **Pipe**         | The pipe is drained and all writers have closed. |
| **Console**      | `Ctrl-d` is entered at an empty input position.  |

EOF need not persist forever: later file growth or console input can make a later read produce bytes.

### Panic output and whole-machine failure

**A panic stops the whole machine; a user-space process fault kills only that process.** The 62 `panic()` call sites across 13 kernel files pass a message naming the broken invariant.

`panic("message")` in `kernel/printk.c`:

1. Prints `panic: ` and the message.
2. Sets `panicked`, making every other hart park forever in `uartputc_sync` so it cannot overwrite the message.
3. Spins forever. It is declared `__attribute__((noreturn))` and never returns or reboots.

See [What a panic is, and what it prints](06-build-artifacts.md#panic-invariant-failures-output-and-hart-shutdown).

## Course names, build variables, and repository files

| Term                   | Meaning                                                                                                                                                      |
| ---------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| 6.1810                 | MIT's Operating System Engineering course. Previously numbered 6.828.                                                                                        |
| **PDOS**               | **P**arallel and **D**istributed **O**perating **S**ystems, the MIT group behind xv6.                                                                        |
| `fs.img`               | The disk image holding xv6's root filesystem, built by `mkfs/mkfs`.                                                                                          |
| `fs.img.bk`            | The previous image, rotated aside by the `newfs.img` target on every `make qemu`. Regenerated constantly and git-ignored; `make qemu-fs` skips the rotation. |
| `mkfs`                 | **m**a**k**e **f**ile**s**ystem. Host-compiled, since it runs on your machine.                                                                               |
| `UPROGS`               | The Makefile list of user programs that go into `fs.img`.                                                                                                    |
| `conf/lab.mk`          | One line selecting the active lab. See [03-lab-workflow.md](03-lab-workflow.md).                                                                             |
| `gradelib.py`          | MIT's grading harness — boots QEMU, drives the shell, matches output.                                                                                        |
| **hart 0 / boot hart** | The hart whose id is 0. `main()` picks it by id, not by arrival order, to initialise shared state while the others wait.                                     |

## Why “freestanding” and “stub” are precise terms

Two terms in these guides are precise technical choices rather than casual description, and both are easier to trust once you know where they come from.

### “Freestanding”: the ISO C term for having no host environment

`freestanding` is not an adjective someone reached for. It is the term **ISO C uses itself**: the standard defines a _freestanding implementation_ in contrast to a _hosted_ one, and GCC's flag is named after the standard's word — `-ffreestanding`. Picking a synonym would cut the connection to both.

**“Freestanding” identifies the missing dependency precisely:** a host environment, meaning an underlying operating system that supplies a C library and calls `main`. ISO C specifies which headers remain available and where the program may start; see [Hosted versus freestanding](#hosted-versus-freestanding-the-part-of-iso-c-that-xv6-does-obey).

“Independent” leaves “independent of what?” unanswered. The xv6 kernel still depends on its compiler, RV64GC, QEMU's `virt` board, and correct placement by the linker script.

### “Stub”: a forwarding role, distinct from size and form

**“Stub” describes a role; “snippet” describes an excerpt; “function” and “module” describe structure.** A complete function can be shown as a snippet and serve as a stub at the same time.

**The system-call analogy: a stub is a forwarding address.**

Post arrives under the right name and is sent onward. For the simple system-call stubs in this tree, the analogy maps as follows:

| In the analogy                                                | What it stands for                                                                                                                                 |
| ------------------------------------------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------- |
| **It is a genuine, deliverable address**                      | The stub compiles, links, and occupies a real symbol at a real address. `ld` resolves calls to `fork` because `fork` is really there               |
| **Nothing is kept there. It all goes onward**                 | Three instructions: `li a7`, `ecall`, and `ret`. The kernel implements the service. |
| **An address copied onto a scrap of paper**                   | A **snippet** can show either the complete address or an incomplete fragment. Completeness depends on what was copied. |
| **"It's an address"**                                         | **Function** or **module**. True, and it tells you nothing: the warehouse has an address too                                                       |
| **A depot that opens the parcel and attaches a customs form** | A **wrapper**. `sbrk` adding `SBRK_EAGER` is exactly that form                                                                                     |
| **The warehouse where the goods actually are**                | An **implementation**. `strlen`                                                                                                                    |

**A callable stub needs a resolvable entry point.** `ld` resolves the `fork` symbol because the generated stub defines it. Calling the same code a snippet says how it is presented, not whether it compiles or links.

Size stays a separate question throughout, which is the whole point about "function" and "module". A forwarding address can be a PO box (one function — `sys_sbrk`), a mail-handling office (a module — QEMU's gdb stub), or an empty building registered as a company's office (a file — `docs/book/ch13-summary.md`). None of those sizes makes it more or less a forwarding address.

> [!NOTE]
> The analogy earns exactly one thing: it separates _what a thing does_ from _how big it is_. Push it further and it misleads — nothing in the post is privileged, and `ecall` is not a delivery van but a change of CPU privilege level. For what actually happens on the other side of that call, see [Why the system call number goes in `a7`](06-build-artifacts.md#why-the-system-call-number-goes-in-a7).

**Snippet versus executable stub:** neither term determines the code's size, and only “stub” identifies its role.

|                                              | **Snippet**                                    | **Stub**                                         |
| -------------------------------------------- | ---------------------------------------------- | ------------------------------------------------ |
| **Defined by**                               | Its form — a short fragment of code            | Its role — standing in for a full implementation |
| **Is it complete?**                          | It can show complete code or a partial excerpt. | A callable stub supplies an entry point; it delegates the service. |
| **Does it have a name others link against?** | It may include a named function, including a stub. | The generated xv6 system-call stubs export callable symbols. |

`fork` in `user/usys.S` is a real function at a real address that `ld` resolves references to; calling it a snippet would describe its length and miss its job. It occupies the name `fork` so that C code can call `fork()` — and then does none of the work itself, handing straight over to the kernel.

**Against "function" and "module"** — these name a _granularity_, while "stub" names a _role_. A stub **is** a function, or a module, or a file. The two axes are orthogonal, and this repository demonstrates it by using the word at all three levels at once:

| Thing                                     | Granularity | Role           |
| ----------------------------------------- | ----------- | -------------- |
| `sys_sbrk` in `user/usys.S`               | function    | **stub**       |
| `sbrk` in `user/ulib.c`                   | function    | wrapper        |
| `strlen` in `user/ulib.c`                 | function    | implementation |
| QEMU's gdb stub                           | module      | **stub**       |
| `docs/book/ch13-summary.md`, all 21 lines | file        | **stub**       |

A word that qualifies a function, a module, _and_ a file **cannot** be a replacement for any of them. It is the role-shaped word that says what a thing does; they are the structural words that say what size of thing it is.

**The `sbrk` path combines a wrapper and a stub.** `sbrk(n)` in [`user/ulib.c`](../user/ulib.c) supplies `SBRK_EAGER` to `sys_sbrk(n, SBRK_EAGER)`. The `sys_sbrk` stub generated by [`user/usys.pl`](../user/usys.pl) forwards those arguments through `ecall`; it does not choose eager or lazy mode.

`strlen()` is a separate example of an implementation: it computes the string length in user space. It is not part of the `sbrk` call path.

That distinction explains the special case in `user/usys.pl`: it emits the stub as `sys_sbrk`, leaving the plain `sbrk` name available for the wrapper.

Substitution fails in both directions, which is the test worth applying to any such word:

| Sentence         | With "stub"                                                                              | With "function"                                                     |
| ---------------- | ---------------------------------------------------------------------------------------- | ------------------------------------------------------------------- |
| **On behaviour** | “The xv6 system-call stub preserves `a0`–`a5`” explains argument forwarding in this ABI. | “The function preserves `a0`–`a5`” states a property without identifying the forwarding role. |
| **On counting**  | “22 system-call stubs” identifies the generated forwarding entry points. | “22 functions” does not identify which set; user-library functions are another set. |

"Module" fails on granularity in the opposite direction: `user/usys.S` as a whole file could fairly be called a module, but the four lines named `fork` inside it could not.

**“Stub” has the same standing-in role across several fields:**

- **RPC:** client and server stubs marshal calls across a network.
- **GDB:** the debugger talks to a remote stub inside the target.
- **Testing:** stubs are distinguished from mocks.

The etymology fits: a stub is the short part left when the rest is torn away, like a ticket stub or tree stump. In `usys.S`, the name and entry point survive while the implementation body does not.

## Kernel function and symbol names: prefixes and expansions

**Most cryptic kernel names are a subsystem prefix plus a verb.** Decode the prefix first, then the verb; the tables cover the names this tree actually uses.

### Name prefixes by subsystem

| Prefix or pattern          | Stands for                                        | Examples in this tree                                                                                      |
| -------------------------- | ------------------------------------------------- | ---------------------------------------------------------------------------------------------------------- |
| `sys_`                     | **sys**tem-call handler: fetches arguments        | `sys_fork`, `sys_exec`, `sys_pause` in `kernel/sysproc.c` and `kernel/sysfile.c`                           |
| `k` + system-call name     | **k**ernel implementation behind a handler        | `kfork`, `kexec`, `kwait`, `kexit`, `kkill`                                                                |
| `k` + allocator verb       | **k**ernel physical-page allocator                | `kinit`, `kalloc`, `kfree` in `kernel/kalloc.c`                                                            |
| `uvm`                      | **u**ser **v**irtual **m**emory                   | `uvmcreate`, `uvmalloc`, `uvmdealloc`, `uvmcopy`, `uvmfree`, `uvmunmap`, `uvmclear`                        |
| `kvm`                      | **k**ernel **v**irtual **m**emory                 | `kvmmake`, `kvmmap`, `kvminit`, `kvminithart`                                                              |
| `…init` versus `…inithart` | once on hart 0 versus once per **hart**           | `trapinit`/`trapinithart`, `plicinit`/`plicinithart`, `kvminit`/`kvminithart`                              |
| `i`                        | **i**node                                         | `iget`, `ilock`, `iunlock`, `iput`, `iunlockput`, `idup`, `ialloc`, `iupdate`, `itrunc`, `readi`, `writei` |
| `b`                        | disk-**b**lock **b**uffer                         | `binit`, `bread`, `bwrite`, `brelse`, `bpin`, `bunpin`                                                     |
| `r_` / `w_`                | **r**ead / **w**rite a register or CSR            | `r_satp`, `w_stvec`, `r_tp`, `w_mepc` in `kernel/riscv.h`                                                  |
| `arg` / `fetch`            | system-call **arg**ument / **fetch** from user VA | `argint`, `argaddr`, `argstr`, `fetchaddr`, `fetchstr` in `kernel/syscall.c`                               |
| `copyin` / `copyout`       | copy **in**to the kernel / **out** to user memory | `copyin`, `copyout`, `copyinstr`, `either_copyin`, `either_copyout`                                        |
| `…intr`                    | **int**e**r**rupt handler                         | `devintr` (**dev**ice), `clockintr`, `uartintr`, `consoleintr`                                             |
| `…vec`                     | trap **vec**tor: where `stvec` points             | `uservec`, `kernelvec`                                                                                     |

### Individual kernel names, expanded

| Name                        | Expansion                                 | What it does                                                                                                                                                                         |
| --------------------------- | ----------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| `uvmunmap`                  | **u**ser **v**irtual **m**emory **unmap** | Removes `npages` mappings starting at page-aligned `va`; frees the frames only when `do_free` is set. `proc_freepagetable()` unmaps `TRAMPOLINE` and `TRAPFRAME` with `do_free = 0`. |
| `mappages`                  | **map** **pages**                         | Installs leaf PTEs for a virtual range onto a physical range in `kernel/vm.c`.                                                                                                       |
| `walk`                      | page-table **walk**                       | Descends the three Sv39 levels to a leaf PTE, allocating missing levels when `alloc` is set.                                                                                         |
| `walkaddr`                  | **walk** to physical **addr**ess          | Returns the physical address of a user page, or `0` if it is unmapped, invalid, or lacks `PTE_U`.                                                                                    |
| `freewalk`                  | **free** by **walk**ing                   | Recursively frees page-table pages after all leaf mappings are gone.                                                                                                                 |
| `vmfault`                   | **v**irtual-**m**emory **fault**          | Allocates a lazily reserved page on first touch; returns `0` when the address is invalid.                                                                                            |
| `swtch`                     | **sw**i**tch**, without the vowel         | Saves and restores `struct context` in `kernel/swtch.S`. `switch` is a C keyword, so the Unix V6 spelling survives.                                                                  |
| `sched`                     | **sched**ule                              | Gives the hart back to `scheduler()` from a process that is giving up the CPU.                                                                                                       |
| `forkret`                   | **fork** **ret**urn                       | The first kernel code every new process runs, because `allocproc()` stores it in `context.ra`.                                                                                       |
| `userret`                   | **user** **ret**urn                       | Trampoline code that switches to the user page table, restores registers from the trapframe, and executes `sret`.                                                                    |
| `prepare_return`            | prepare the **return** to user space      | Sets `stvec`, the trapframe's `kernel_*` fields, `sstatus`, and `sepc` before `userret` runs.                                                                                        |
| `push_off` / `pop_off`      | push / pop "interrupts **off**"           | Nestable interrupt disabling. `struct cpu` tracks depth in `noff` and the earlier state in `intena` (**int**errupts **ena**bled).                                                    |
| `namei` / `nameiparent`     | **name** to **i**node                     | Resolve a path to its inode, or to its parent directory's inode; both use the worker `namex`, whose `skipelem` peels one path **elem**ent.                                           |
| `bmap`                      | **b**lock **map**                         | Translates a file's block number to a disk block, allocating it if needed.                                                                                                           |
| `begin_op` / `end_op`       | begin / end a file-system **op**eration   | Brackets one logged transaction in `kernel/log.c`.                                                                                                                                   |
| `PGROUNDUP` / `PGROUNDDOWN` | **p**a**g**e **round** up / down          | Aligns an address to a 4096-byte page boundary.                                                                                                                                      |
| `PTE2PA` / `PA2PTE`         | **PTE** **to** **p**hysical **a**ddress   | Converts between a PTE's PPN field and a physical address.                                                                                                                           |
| `PX(level, va)`             | **p**age-table inde**x**                  | Extracts the 9-bit index of `va` for one page-table level.                                                                                                                           |
| `MAKE_SATP`                 | **make** a `satp` value                   | Combines the Sv39 mode bits with a root page table's PPN.                                                                                                                            |
| `KSTACK(p)`                 | **k**ernel **stack** address              | The virtual address of process slot `p`'s kernel stack, below `TRAMPOLINE`, with an unmapped guard page between stacks.                                                              |

## Naming traps: `printk`, `_name`, xv6, and `virt`

Small things that cause confusion the first time:

- **`printk` vs `printf`** — the kernel's is `printk` (`kernel/printk.c`), user space has its own `printf` (`user/printf.c`). Upstream renamed the kernel one from `printf.c`, which is why the remaining 2025 lab branches still refer to the old name while `labs/util` and `labs/riscv`, now MIT's 2026 publications, already use `printk.c`.
- **The `_` prefix** — `user/_ls` is the linked executable, `user/ls.o` the object file, and `mkfs` strips the underscore so that inside xv6 the program is just `ls`. The reason is written in `mkfs/mkfs.c`: the binaries are named `_rm`, `_cat`, and so on "to keep the build operating system from trying to execute them in place of system binaries like `rm` and `cat`". The prefix protects **your host**, not xv6.
- **`v6` in "xv6"** — refers to Unix Version 6, the 1975 system it re-implements, not a version of xv6 itself.
- **"virt"** — QEMU's generic board name, not related to virtio, though the virt board is where the virtio devices live.
