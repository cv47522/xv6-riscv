# Terminology

---

[toc]

---

Every acronym and piece of jargon you meet in xv6, the [xv6 book](https://mit-pdos.github.io/xv6-riscv-book/), and the 6.1810 labs — expanded, and explained in terms of where it appears in this repository.

This is the hub the other guides link into. When a book chapter introduces a new term, add it here once rather than re-explaining it in chapter notes.

## The big three

**xv6** — "**x**86 **v**ersion **6**". A re-implementation of Dennis Ritchie and Ken Thompson's **Unix Version 6** (1975), written at MIT for teaching. The name kept its `x86` origin even after the port to RISC-V, which is why this repository is `xv6-riscv` rather than something tidier. It is deliberately small: around 9,000 lines, small enough that one person can read all of it.

**RISC-V** — "**R**educed **I**nstruction **S**et **C**omputer, **f**ive", pronounced "risk five". The fifth RISC design from UC Berkeley. Unlike x86 or ARM it is an open standard: the specification is free to implement, which is why it is a good teaching target. xv6 targets **RV64GC** — 64-bit, with the G and C extension bundles.

**QEMU** — "**Q**uick **EMU**lator". Emulates a whole computer in software. `qemu-system-riscv64` pretends to be a RISC-V machine so xv6 can boot on your x86 laptop. xv6 runs on QEMU's `virt` board, a synthetic machine whose memory layout is documented and hard-coded into `kernel/memlayout.h`.

## RISC-V architecture

| Term                         | Expansion                                                     | Meaning in xv6                                                                                                                                                                                                       |
| ---------------------------- | ------------------------------------------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **hart**                     | **har**dware **t**hread                                       | RISC-V's word for a CPU core. The boot messages `hart 1 starting` come from `kernel/main.c`. `NCPU` in `kernel/param.h` caps how many xv6 supports.                                                                  |
| **RV64GC**                   | RISC-**V** **64**-bit, **G** + **C**                          | The target ISA. **G** is shorthand for IMAFD: **I**nteger, **M**ultiply/divide, **A**tomic, single-precision **F**loat, **D**ouble. **C** is compressed 16-bit instructions. Set by `-march=rv64gc` in the Makefile. |
| **ISA**                      | **I**nstruction **S**et **A**rchitecture                      | The contract between hardware and software: what instructions exist and what they do.                                                                                                                                |
| **CSR**                      | **C**ontrol and **S**tatus **R**egister                       | Special registers holding privileged state. `kernel/riscv.h` is mostly accessors for these.                                                                                                                          |
| **satp**                     | **S**upervisor **A**ddress **T**ranslation and **P**rotection | The CSR holding the root page table's physical address. Writing it switches address spaces.                                                                                                                          |
| **stvec**                    | **S**upervisor **T**rap **VEC**tor                            | Where the CPU jumps on a trap. Set to `kernelvec` or `trampoline` depending on the current mode.                                                                                                                     |
| **sepc**                     | **S**upervisor **E**xception **P**rogram **C**ounter          | The PC at the moment a trap occurred, so execution can resume.                                                                                                                                                       |
| **scause**                   | **S**upervisor **CAUSE**                                      | Why the trap happened — system call, page fault, timer interrupt.                                                                                                                                                    |
| **sstatus**                  | **S**upervisor **STATUS**                                     | Interrupt-enable bits and the mode the CPU came from.                                                                                                                                                                |
| **ecall**                    | **e**nvironment **call**                                      | The instruction that makes a system call. `user/usys.S` loads the syscall number into `a7`, then executes `ecall`.                                                                                                   |
| **Sv39**                     | **S**upervisor **v**irtual, **39**-bit                        | The paging scheme: 39-bit virtual addresses, three levels of page table, 4 KiB pages.                                                                                                                                |
| **PTE**                      | **P**age **T**able **E**ntry                                  | One mapping from a virtual page to a physical one, plus permission bits.                                                                                                                                             |
| **PPN**                      | **P**hysical **P**age **N**umber                              | A physical address with the low 12 bits (the offset) dropped.                                                                                                                                                        |
| **TLB**                      | **T**ranslation **L**ookaside **B**uffer                      | The CPU's cache of recent translations. Flushed with `sfence.vma` after changing page tables.                                                                                                                        |
| **M-mode / S-mode / U-mode** | **M**achine, **S**upervisor, **U**ser                         | The three privilege levels. xv6 boots in M-mode (`kernel/start.c`), drops to S-mode for the kernel, and runs user programs in U-mode.                                                                                |
| **PLIC**                     | **P**latform-**L**evel **I**nterrupt **C**ontroller           | Routes device interrupts (UART, disk) to harts. `kernel/plic.c`.                                                                                                                                                     |
| **CLINT**                    | **C**ore-**L**ocal **INT**erruptor                            | Provides timer interrupts, which drive preemptive scheduling.                                                                                                                                                        |

## Toolchain

| Term                  | Expansion                                                   | Role                                                                                                                                          |
| --------------------- | ----------------------------------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------- |
| **GCC**               | **G**NU **C**ompiler **C**ollection                         | The compiler. Here in cross form: `riscv64-linux-gnu-gcc`.                                                                                    |
| **binutils**          | **bin**ary **utils**                                        | `ld`, `as`, `objdump`, `objcopy`, `nm`.                                                                                                       |
| **GDB**               | **G**NU **D**e**B**ugger                                    | `gdb-multiarch` can debug non-native architectures, which plain `gdb` cannot.                                                                 |
| **ELF**               | **E**xecutable and **L**inkable **F**ormat                  | The binary format for `kernel/kernel` and every user program. `kernel/elf.h` defines the headers; `kernel/exec.c` parses them.                |
| **cross-compilation** | —                                                           | Building on one architecture for another. Nothing built here runs natively on your x86 machine.                                               |
| **TOOLPREFIX**        | —                                                           | The Makefile variable holding the cross-tool prefix, e.g. `riscv64-linux-gnu-`.                                                               |
| **freestanding**      | —                                                           | A C environment with no operating system or standard library beneath it — exactly what a kernel is. Hence `-ffreestanding` and `-nostdlib`.   |
| **PIE**               | **P**osition **I**ndependent **E**xecutable                 | A binary that can load at any address. xv6 must _not_ be one, since it is linked to a fixed address with no dynamic loader. Hence `-fno-pie`. |
| **medany**            | **med**ium-**any**                                          | The RISC-V code model allowing PC-relative addressing within ±2 GiB, needed because the kernel lives at `0x80000000`.                         |
| **virtio**            | **virt**ual **I/O**                                         | A standard for efficient emulated devices. xv6's only disk is a virtio block device; `kernel/virtio_disk.c` drives it.                        |
| **MMIO**              | **M**emory-**M**apped **I/O**                               | Talking to devices by reading and writing special memory addresses rather than dedicated instructions.                                        |
| **UART**              | **U**niversal **A**synchronous **R**eceiver/**T**ransmitter | The serial port. With `-nographic`, this is your terminal. `kernel/uart.c`.                                                                   |
| **KCSAN**             | **K**ernel **C**oncurrency **SAN**itizer                    | A data-race detector, enabled with `make KCSAN=1`.                                                                                            |

## Operating system concepts

| Term                      | Meaning in xv6                                                                                                                                                                                       |
| ------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **kernel**                | The privileged core. Everything in `kernel/`.                                                                                                                                                        |
| **user space**            | Unprivileged programs. Everything in `user/`.                                                                                                                                                        |
| **system call**           | The controlled entry from user space into the kernel. `user/usys.S` → `ecall` → `kernel/syscall.c`.                                                                                                  |
| **trap**                  | Any transfer to the kernel: system call, exception, or interrupt. `kernel/trap.c`.                                                                                                                   |
| **trampoline**            | A page mapped at the same virtual address in every address space, so the switch between user and kernel page tables can happen without the mapping vanishing mid-instruction. `kernel/trampoline.S`. |
| **trapframe**             | Per-process saved register state at trap time.                                                                                                                                                       |
| **context switch**        | Swapping one process's registers for another's. `kernel/swtch.S`.                                                                                                                                    |
| **spinlock**              | A lock that busy-waits. `kernel/spinlock.c`.                                                                                                                                                         |
| **sleeplock**             | A lock that yields the CPU while waiting; usable when the wait may be long, such as disk I/O. `kernel/sleeplock.c`.                                                                                  |
| **inode**                 | The on-disk record describing a file: type, size, and where its blocks are. The number in `ls` output.                                                                                               |
| **superblock**            | Block 1, describing the filesystem's layout.                                                                                                                                                         |
| **logging / journalling** | Writing changes to a log before applying them, so a crash mid-update is recoverable. `kernel/log.c`.                                                                                                 |
| **buffer cache**          | In-memory cache of disk blocks. `kernel/bio.c`.                                                                                                                                                      |
| **page fault**            | A trap raised when a virtual address has no valid mapping. The basis of the cow and mmap labs.                                                                                                       |
| **copy-on-write**         | Sharing pages between parent and child after `fork`, copying only when one writes.                                                                                                                   |
| **demand paging**         | Allocating a page only when it is first touched.                                                                                                                                                     |
| **zombie**                | A process that has exited but whose parent has not yet called `wait`.                                                                                                                                |
| **orphan**                | A process whose parent exited first; re-parented to `init`.                                                                                                                                          |

## Course and repository

| Term                   | Meaning                                                                               |
| ---------------------- | ------------------------------------------------------------------------------------- |
| **6.1810**             | MIT's Operating System Engineering course. Previously numbered 6.828.                 |
| **PDOS**               | **P**arallel and **D**istributed **O**perating **S**ystems, the MIT group behind xv6. |
| **`fs.img`**           | The disk image holding xv6's root filesystem, built by `mkfs/mkfs`.                   |
| **`mkfs`**             | **m**a**k**e **f**ile**s**ystem. Host-compiled, since it runs on your machine.        |
| **`UPROGS`**           | The Makefile list of user programs that go into `fs.img`.                             |
| **`conf/lab.mk`**      | One line selecting the active lab. See [03-lab-workflow.md](03-lab-workflow.md).      |
| **`gradelib.py`**      | MIT's grading harness — boots QEMU, drives the shell, matches output.                 |
| **hart 0 / boot hart** | The first CPU to reach `main()`; it initialises everything while the others wait.     |

## Naming oddities worth knowing

Small things that cause confusion the first time:

- **`printk` vs `printf`** — the kernel's is `printk` (`kernel/printk.c`), user space has its own `printf` (`user/printf.c`). Upstream renamed the kernel one from `printf.c` recently, which is why the 2025 lab branches still refer to the old name.
- **The `_` prefix** — `user/_ls` is the linked executable, `user/ls.o` the object file. The underscore keeps the Makefile's pattern rules unambiguous; inside xv6 the program is just `ls`.
- **`v6` in "xv6"** — refers to Unix Version 6, the 1975 system it re-implements, not a version of xv6 itself.
- **"virt"** — QEMU's generic board name, not related to virtio, though the virt board is where the virtio devices live.
