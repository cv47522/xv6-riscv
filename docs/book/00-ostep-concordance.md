# OSTEP Concordance

---

[toc]

---

One concept, mapped across every source that covers it. **Read this before writing a chapter note**: it tells you what is already explained elsewhere and who owns it.

Statuses follow the ownership rules in [README.md](README.md#how-a-chapter-note-is-written):

| Status       | Meaning                                                                              |
| ------------ | ------------------------------------------------------------------------------------ |
| **linked**   | The OSTEP note is written, and this repository links to it.                          |
| **loan**     | The OSTEP note is still a placeholder, so an xv6 note holds the concept temporarily. |
| **xv6-only** | No OSTEP counterpart exists, so the xv6 note owns the concept permanently.           |

## Concepts

| Concept                           | OSTEP note                              | xv6 ch | xv6 source                                                   | Exercise                                                      | Status   |
| --------------------------------- | --------------------------------------- | ------ | ------------------------------------------------------------ | ------------------------------------------------------------- | -------- |
| **fork/exec/wait**                | `The_Process_Abstraction.md` §7         | 1      | `kernel/proc.c` `kfork()`, `kernel/exec.c` `kexec()`         | `codes/` `cpu-process-api/fork_*.c`                           | linked   |
| **process states, PCB**           | `The_Process_Abstraction.md` §4–5       | 1      | `kernel/proc.c` `allocproc()`                                | `codes/` `cpu-process-intro/`                                 | linked   |
| **fds, pipes, shell**             | `The_Process_Abstraction.md` §7         | 1      | `kernel/pipe.c` `pipealloc()`, `user/sh.c` `runcmd()`        | `codes/` `cpu-process-api/homework/07_parent_children_pipe.c` | linked   |
| **kernel/user split**             | `Introduction_to_Operating_Systems.md`  | 2      | `kernel/main.c`, `kernel/proc.c` `allocproc()`               | —                                                             | linked   |
| **machine mode, boot**            | —                                       | 2      | `kernel/entry.S`, `kernel/start.c`                           | —                                                             | xv6-only |
| **paging, Sv39**                  | `Paging.md`                             | 3      | `kernel/vm.c` `walk()`, `uvmalloc()`                         | `ostep-homework/vm-paging/`                                   | linked   |
| **address spaces**                | `Address_Spaces_And_Translation.md`     | 3      | `kernel/vm.c` `uvmcopy()`                                    | `ostep-homework/vm-mechanism/`                                | linked   |
| **memory layout, segments**       | `Memory_Management.md`                  | 3      | `kernel/memlayout.h`, `user/user.ld`                         | `ostep-homework/vm-freespace/`                                | linked   |
| **limited direct execution**      | `The_Process_Abstraction.md` §8         | 4      | `kernel/trap.c` `usertrap()`, `kerneltrap()`                 | `codes/` `cpu-process-mechanisms/lde_cost.c`                  | linked   |
| **trampoline, trapframe**         | —                                       | 4      | `kernel/trampoline.S`                                        | lab: traps                                                    | xv6-only |
| **page faults, COW**              | `Swapping_And_VM_Systems.md`            | 5      | `kernel/trap.c` `usertrap()`, `kernel/vm.c` `uvmcopy()`      | lab: cow                                                      | loan     |
| **device drivers, PLIC**          | `IO_Devices_And_Disks.md`               | 6      | `kernel/uart.c` `uartintr()`, `kernel/plic.c` `plic_claim()` | `ostep-homework/file-devices/`                                | loan     |
| **spinlocks, races**              | `Concurrency_Threads_And_Locks.md`      | 7      | `kernel/spinlock.c` `acquire()`, `release()`                 | `ostep-homework/threads-locks/`                               | loan     |
| **context switch**                | `CPU_Scheduling.md`                     | 8      | `kernel/swtch.S`, `kernel/proc.c` `sched()`, `scheduler()`   | `ostep-homework/cpu-sched/`                                   | linked   |
| **sleep/wakeup, cond vars**       | `Condition_Variables_And_Semaphores.md` | 9      | `kernel/proc.c` `sleep()`, `wakeup()`                        | `ostep-homework/threads-cv/`                                  | loan     |
| **inodes, buffer cache**          | `File_Systems.md`                       | 10     | `kernel/fs.c` `ialloc()`, `kernel/bio.c` `bread()`           | `ostep-homework/file-implementation/`                         | loan     |
| **journaling, crash consistency** | `Crash_Consistency_And_Journaling.md`   | 11     | `kernel/log.c` `begin_op()`, `write_log()`                   | `ostep-homework/file-journaling/`                             | loan     |
| **memory ordering, fences**       | `Concurrency_Bugs_And_Events.md`        | 12     | `kernel/spinlock.c`, `kernel/virtio_disk.c` `io_fence()`     | —                                                             | loan     |
| **summary**                       | —                                       | 13     | —                                                            | —                                                             | xv6-only |

OSTEP notes live in `../../../operating-system/`. The `Exercise` column spans three repositories: `codes/` is `../../../operating-system/codes/src/main/virtualization/`, `ostep-homework/` is `../../../ostep-homework/`, and `lab:` names a lab in this tree.

> [!WARNING]
> `../../../ostep-projects/` also contains six xv6-based projects — `initial-xv6`, `initial-xv6-tracer`, `scheduling-xv6-lottery`, `vm-xv6-intro`, `concurrency-xv6-threads`, and `filesystems-checker`. All six target **`xv6-public`**, the older x86 tree, and none of them build against this repository. They are deliberately absent from the table above.

## Appendix: divergence between the two xv6 trees

OSTEP's xv6 material describes [`mit-pdos/xv6-public`](https://github.com/mit-pdos/xv6-public), the 32-bit x86 tree. This repository is [`mit-pdos/xv6-riscv`](https://github.com/mit-pdos/xv6-riscv). They are one lineage, not rivals: this repository's history begins at the same `import` commit of 2006-06-12, and the RISC-V port lands in 2019. OSTEP's figures simply predate it.

A survey of the OSTEP notes bounds the divergence to one affected file:

| OSTEP note                                                                | xv6 material                             | Interpretation                                                                          |
| ------------------------------------------------------------------------- | ---------------------------------------- | --------------------------------------------------------------------------------------- |
| `Paging.md`, `Address_Spaces_And_Translation.md`, and `CPU_Scheduling.md` | None.                                    | Their x86 content is OSTEP's teaching architecture and is unrelated to either xv6 tree. |
| `Memory_Management.md`                                                    | One mention, already naming RISC-V `sp`. | No translation is needed.                                                               |
| `The_Process_Abstraction.md`                                              | Figure 4.5 and Figure 6.4.               | This is the only affected note; the correspondence is recorded below.                   |

> [!IMPORTANT]
> That note is **not wrong and is not to be corrected.** It faithfully reproduces the figures of the book it takes notes on. Rewriting them to RISC-V would desync the note from its source and invent material OSTEP does not contain. Under the ownership rule the translation is this repository's job, and this appendix is where it lives.

### Context switch registers

OSTEP's Figure 4.5 shows the x86 `struct context`; this tree's is in `kernel/proc.h`.

| Role                    | `xv6-public` (x86, 32-bit)               | `xv6-riscv` (this tree) |
| ----------------------- | ---------------------------------------- | ----------------------- |
| **Return address / PC** | `eip`                                    | `ra`                    |
| **Stack pointer**       | `esp`                                    | `sp`                    |
| **Callee-saved**        | `ebx`, `ecx`, `edx`, `esi`, `edi`, `ebp` | `s0`–`s11`              |
| **Width**               | 32-bit                                   | 64-bit (`uint64`)       |

The difference is structural, not cosmetic: RISC-V has no dedicated frame pointer register in the ABI sense, and its callee-saved set is twelve registers wide, so the saved context is larger and flatter. The switch itself is `kernel/swtch.S`, called from `kernel/proc.c` `sched()`.

### System call naming

This revision prefixes the kernel-side implementations, where both the book prose and OSTEP use the bare POSIX names.

| Book prose and OSTEP | This tree | Defined in      |
| -------------------- | --------- | --------------- |
| `fork`               | `kfork()` | `kernel/proc.c` |
| `exec`               | `kexec()` | `kernel/exec.c` |
| `wait`               | `kwait()` | `kernel/proc.c` |
| `exit`               | `kexit()` | `kernel/proc.c` |

The user-facing names in `user/user.h` are unprefixed, so a program calls `fork()` and the kernel implements `kfork()`.
