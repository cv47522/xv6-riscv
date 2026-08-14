# Book Notes

---

[toc]

---

Chapter notes from [xv6: a simple, Unix-like teaching operating system](https://mit-pdos.github.io/xv6-riscv-book/), added as each chapter is read.

## Chapters

Tick these off as you go, and link each to its notes file once written.

| #      | Chapter                       | Mostly about                             | Notes |
| ------ | ----------------------------- | ---------------------------------------- | ----- |
| **1**  | Operating system interfaces   | Processes, files, pipes, the shell       | —     |
| **2**  | Operating system organization | Kernel/user split, privilege modes, boot | —     |
| **3**  | Page tables                   | Sv39, address spaces, `kernel/vm.c`      | —     |
| **4**  | Traps and system calls        | `ecall`, trampoline, trapframe           | —     |
| **5**  | Interrupts and device drivers | UART, PLIC, virtio disk                  | —     |
| **6**  | Locking                       | Spinlocks, races, deadlock               | —     |
| **7**  | Scheduling                    | Context switching, sleep/wakeup          | —     |
| **8**  | File system                   | Inodes, logging, the buffer cache        | —     |
| **9**  | Concurrency revisited         | Lock-free tricks, memory ordering        | —     |
| **10** | Summary                       | —                                        | —     |

## Naming

One file per chapter, numbered so they sort correctly:

```
ch01-operating-system-interfaces.md
ch03-page-tables.md
```

## Suggested shape

Nothing enforced, but notes are more useful later if they connect the reading to the code:

```markdown
# Chapter N: Title

## Key ideas

What the chapter is actually arguing, in your own words.

## Code walked through

- `kernel/vm.c:57` — walk() descends the three-level page table
- `kernel/proc.c:112` — allocproc() sets up a new process

## Questions I had

Things that were not obvious, and what resolved them.

## Connections

Which lab this chapter prepares you for; which earlier chapter it builds on.
```

> [!TIP]
> Add new vocabulary to [../04-terminology.md](../04-terminology.md) rather than defining it inline. That keeps one authoritative definition and lets chapter notes stay short.

File paths written as `kernel/vm.c:57` are clickable in most editors, which makes them far more useful than prose descriptions of where something lives.
