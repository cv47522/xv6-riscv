# Chapter 1: Operating system interfaces

---

[toc]

---

> **Theory:** [OSTEP — The Process Abstraction][ostep]. Read that first; this note does not re-explain it.

## What xv6 actually does

The book chapter is a tour of the interface. This note is a tour of the code sitting behind it, in roughly the order the shell exercises it: make a process, replace its image, rewire its descriptors, wire two of them together. Every claim below is anchored to a symbol you can jump to.

### Creating a process: `kfork()`

`kfork()` in `kernel/proc.c` is about forty-five lines, and every one of them is doing one of five jobs.

- **Claim a slot.** `allocproc()` linear-scans the fixed `proc[NPROC]` array for a `UNUSED` entry and returns it with `p->lock` still held. It assigns a fresh pid, `kalloc()`s the trapframe page, builds an empty user page table with `proc_pagetable()` — which maps only the trampoline and trapframe pages, both without `PTE_U` — and points `p->context.ra` at `forkret`. The kernel stack is not allocated here: `procinit()` hands every slot a permanent `KSTACK(i)` at boot, so a slot's kernel stack outlives the processes that pass through it.
- **Duplicate the memory.** `uvmcopy()` in `kernel/vm.c` steps through the parent's address space one `PGSIZE` at a time, `walk()`s the parent PTE, `kalloc()`s a fresh physical page, `memmove()`s 4096 bytes into it, and `mappages()` it into the child at the *same* virtual address with the *same* flag bits. The copy is eager and byte-for-byte; there is no copy-on-write anywhere in the base tree, which is exactly the gap the `cow` lab fills ([ch05](ch05-page-faults.md)). Note that it `continue`s past PTEs that are absent or lack `PTE_V` rather than treating them as errors, and that failure part-way through unwinds with `uvmunmap(new, 0, i / PGSIZE, 1)` — the child never survives a partial address space.
- **Duplicate the register state.** `*(np->trapframe) = *(p->trapframe)` copies the saved user registers wholesale, then `np->trapframe->a0 = 0` overwrites the return-value register. This single assignment is the whole of "returns twice" as a mechanism: there is no second return anywhere, only two saved register sets, one of which has had a zero written into the slot the other will find the new pid in. Neither process returns until the trap path restores its own trapframe.
- **Duplicate the descriptor table.** A loop over `NOFILE` entries calls `filedup()` on each non-null `p->ofile[i]`, and `idup()` on `p->cwd`. Both are reference-count bumps, not copies — see below.
- **Publish it.** The parent link is set under the global `wait_lock` rather than `np->lock`, and the state flips to `RUNNABLE` last, so the scheduler cannot pick the child up before it is complete.

### Replacing the image: `kexec()`

`kexec()` in `kernel/exec.c` is written so that it can fail safely, and the structure follows from that. It never touches `p->pagetable` until the very end. Everything — the ELF header read by `readi()`, each program header fed to `loadseg()`, the stack — is built inside a *second* page table obtained from `proc_pagetable(p)`. Only at the "commit to the user image" comment does it swap `p->pagetable`, `p->sz`, `p->trapframe->epc`, and `p->trapframe->sp`, then free the old table. Any failure before that point jumps to `bad:`, frees the half-built table, and returns -1 into a process whose image was never disturbed. That is why a failed `exec` can return at all, and why `user/sh.c` can print `exec %s failed` on the line after the call.

Three details are specific to this tree:

- The stack is `USERSTACK + 1` pages allocated above the program image, and `uvmclear()` strips `PTE_U` from the lowest of them to make a guard page. `USERSTACK` is 1 normally and **2** under `LAB_UTIL` (`kernel/param.h`), so the active lab silently changes the shape of every process's stack.
- Argument strings are `copyout()` onto the new stack one at a time, and alignment precedes each copy rather than following it: `kexec()` (`kernel/exec.c:102-106`) does `sp -= strlen(argv[argc]) + 1` then `sp -= sp % 16` to round the new bottom down to a 16-byte boundary, checks it against `stackbase`, and only then `copyout()`s the string — so every string lands already aligned, as the RISC-V ABI requires. The `ustack[]` array of pointers to them is pushed afterwards and its address left in `a1`.
- `kexec()` returns `argc`, not 0. `sys_exec()` passes that straight through, and the trap return puts it in `a0` — which is the first argument to the new program's `main`. The kernel function genuinely returns; what makes `exec` "not return" is that the code it returns into is a different program.

What `kexec()` deliberately does **not** touch is `p->ofile` and `p->cwd`. That omission is a feature, and the next section is what it buys.

### Descriptors: two levels of indirection

There are two tables, not one, and the split is where all the interesting behaviour lives.

| Level | Where | Contents |
| ----- | ----- | -------- |
| Per-process | `struct file *ofile[NOFILE]` in `struct proc` (`kernel/proc.h`), `NOFILE` = 16 | pointers, indexed by the descriptor number itself |
| System-wide | `ftable.file[NFILE]` in `kernel/file.c`, `NFILE` = 100 | the `struct file` objects, each with `ref`, `readable`, `writable`, a type tag, and `off` |

`fdalloc()` in `kernel/sysfile.c` scans `ofile` from index 0 and takes the first null slot. The "lowest unused descriptor" rule the book leans on so heavily is that four-line loop; nothing else enforces it. `filealloc()` takes `ftable.lock` and returns the first entry whose `ref` is zero, which is why a process can exhaust the *system's* 100 open files without exhausting its own 16.

The consequences fall out of where `off` lives. It sits in the shared `struct file`, not in the per-process slot, so `filedup()` — which only increments `f->ref` under the table lock — leaves parent and child pointing at one offset. Two descriptors share a position in a file precisely when they were reached from a common `struct file`, and `open()`ing the same path twice gets two `struct file` entries and therefore two offsets. `kexit()` closes the loop from the other end: it walks `ofile` calling `fileclose()` on every entry, so a reference count reaching zero at process death is what tears the underlying object down.

Put `kfork()`, `kexec()`, and `fdalloc()` together and shell redirection needs no kernel support whatsoever. The child begins with pointer-identical `ofile` entries; it `close()`s one index, which nulls that slot; the following `open()` calls `fdalloc()`, which hands back the same index because it is now the lowest free one; and `kexec()` then replaces the address space while leaving `ofile` exactly as the child arranged it. No kernel path knows that a redirection happened.

### Pipes

`struct pipe` in `kernel/pipe.c` is one `kalloc()`ed page holding a `PIPESIZE` (512-byte) `data` array, a spinlock, two monotonically increasing counters `nread` and `nwrite`, and the `readopen` / `writeopen` flags. The counters never wrap: indexing is `pi->data[pi->nread % PIPESIZE]`, "empty" is `nread == nwrite`, and "full" is `nwrite == nread + PIPESIZE`.

`pipealloc()` takes two `struct file` from `filealloc()` and points both at the same `struct pipe`, tagging one `FD_PIPE` with `readable = 1` and the other with `writable = 1`. So one buffer becomes two independently reference-counted objects, and `sys_pipe()` merely `fdalloc()`s both and `copyout()`s the pair of integers into the user's `int p[2]`.

That structure is what makes end-of-file work. `piperead()` sleeps while `nread == nwrite && pi->writeopen`, so it returns 0 only when `writeopen` has been cleared — and `pipeclose()` clears it when the *writable* `struct file`'s last reference goes away, freeing the page only once both flags are down. Every process holding a copy of the write descriptor therefore keeps the reader blocked, which is why the shell's pipeline code closes descriptors so aggressively. This tree splits the classic blocking primitive into `sleep_prepare(&pi->nread)`, a lock release, and then `sleep()`; the reasoning belongs to [ch09](ch09-sleep-and-wakeup.md).

### The shell is just another program

`user/sh.c` compiles into `_sh`, lands in `fs.img` beside `_cat` and `_ls`, and links against `user/ulib.c` like any other user program. It holds no privilege and gets no special treatment; `user/init.c` starts it with the same `exec` any program would use.

`main()` opens `"console"` in a loop until the returned descriptor is 3 or higher, then closes that one. The idiom guarantees that 0, 1, and 2 exist without the shell having to know which of them were already open — a point that matters for the `util` lab, where the grader runs `sh < findtest.sh` and fd 0 is a file rather than the console. Then the loop: `getcmd()` writes the `$ ` prompt to fd **2** and reads a line via `gets()` from fd 0, `fork1()` splits, the child runs `runcmd(parsecmd(cmd))`, and the parent `wait(0)`s.

`cd` is handled in `main()` before the fork, and the comment says why: `chdir` in a child would replace that child's `p->cwd` inode pointer and then die with it. Nothing about `cd` requires kernel involvement; it requires only *not forking*.

`runcmd()` is declared `__attribute__((noreturn))` and every arm ends by falling through to `exit(0)`, so calling it consumes the current process. Each command type is a handful of lines:

- **EXEC** — `exec(ecmd->argv[0], ecmd->argv)`, then an error message that only executes if the call came back.
- **REDIR** — `close(rcmd->fd)`, `open(rcmd->file, rcmd->mode)`, then recurse into the wrapped command. Three lines, resting entirely on `fdalloc()`'s lowest-free rule.
- **PIPE** — `pipe(p)` and two `fork1()`s; the left child does `close(1); dup(p[1])`, the right does `close(0); dup(p[0])`, both then close the raw `p[0]` and `p[1]`, and the parent closes both and waits twice. Because each child recurses into `runcmd()`, `a | b | c` nests: the right-hand child forks two more children of its own, and every non-leaf process exists only to wait.
- **LIST** — fork the left side, wait, then recurse into the right side in the current process.
- **BACK** — fork and do not wait.

The parser (`parsecmd()` and its helpers) is the larger half of the file and is entirely user-space; the kernel has no notion of the shell's grammar, of `|`, or of `>`.

## Divergences

- **Naming.** The kernel-side implementations here are `kfork()`, `kexec()`, `kwait()`, and `kexit()`; the names user programs call come from `user/user.h` and are unprefixed. See [the naming table](00-ostep-concordance.md#system-call-naming).
- **Architecture and vintage.** OSTEP's Figure 4.5 `struct proc` and Figure 6.4 `swtch` come from the x86 `xv6-public` tree, which predates the 2019 RISC-V port; see [the register table](00-ostep-concordance.md#context-switch-registers).
- **Signatures.** This revision's `copyout()` takes the process size as an extra second argument, so call sites read `copyout(p->pagetable, p->sz, ...)` where the published book shows four arguments. Expect the same offset when comparing any code fragment in the book against `kernel/vm.c`.

## Code walked through

| File | Symbol | What it does |
| ---- | ------ | ------------ |
| `kernel/proc.c` | `allocproc()` | Claims an `UNUSED` slot, allocates the trapframe page and page table, arms `context.ra` for `forkret` |
| `kernel/proc.c` | `kfork()` | Copies memory, trapframe, descriptors, and cwd; zeroes the child's `a0`; publishes as `RUNNABLE` |
| `kernel/proc.c` | `kexit()` | `fileclose()`s every `ofile` entry, reparents children to init, becomes a `ZOMBIE` |
| `kernel/vm.c` | `uvmcopy()` | Page-by-page eager copy of the parent address space into the child's page table |
| `kernel/vm.c` | `uvmclear()` | Drops `PTE_U` on the stack guard page during exec |
| `kernel/exec.c` | `kexec()` | Builds the new image in a spare page table, then swaps it in as one commit |
| `kernel/exec.c` | `loadseg()` | Reads one ELF program-header segment from the inode into the new page table |
| `kernel/file.c` | `filealloc()` | Hands out one of the 100 system-wide `struct file` entries |
| `kernel/file.c` | `filedup()` / `fileclose()` | Bump and drop `f->ref`; the drop to zero releases the pipe or inode |
| `kernel/sysfile.c` | `fdalloc()` | The lowest-unused-descriptor rule, as a scan of `p->ofile` |
| `kernel/sysfile.c` | `sys_pipe()` | Allocates two descriptors for `pipealloc()`'s pair and copies them out |
| `kernel/pipe.c` | `pipealloc()` | One page of buffer exposed as two oppositely-permissioned `struct file` |
| `kernel/pipe.c` | `piperead()` / `pipeclose()` | Blocking on empty, and turning the last write-end close into EOF |
| `user/sh.c` | `main()` | Guarantees fds 0–2, reads a line, forks, waits; special-cases `cd` |
| `user/sh.c` | `runcmd()` | One arm per command type; never returns |

## Questions I had

- **Why does `uvmcopy()` skip unmapped pages instead of panicking?** The upstream version panics on a missing PTE. Skipping is what lets a process with holes in its address space — which is what lazy allocation and demand paging produce — be forked at all, so the `continue` looks like preparation for [ch05](ch05-page-faults.md) rather than mere tolerance.
- **Why is the parent pointer protected by a global `wait_lock` rather than by `np->lock`?** `kexit()` has to wake a parent it can only reach through that pointer, and `kwait()` has to scan for children, so the link is read by processes that hold neither end's lock. A single global lock sidesteps the ordering problem; [ch07](ch07-locking.md) is where that reasoning belongs.
- **What happens if `fdalloc()` succeeds for the read end of a pipe and fails for the write end?** `sys_pipe()` nulls `p->ofile[fd0]` by hand before closing both files, rather than calling the ordinary close path — worth remembering as the shape of xv6's unwind code generally.
- **Is `NOFILE` = 16 ever actually limiting?** A deep pipeline forks per stage rather than accumulating descriptors in one process, so the shell never approaches it. It would bite a program that opened many files itself.

## Lab connection

`conf/lab.mk` currently reads `LAB=util`, so this chapter is the one directly under the fingertips — see [../03-lab-workflow.md](../03-lab-workflow.md) for the merge and grading mechanics.

- `USERSTACK` becomes 2 pages under `-DLAB_UTIL`, so `kexec()`'s stack arithmetic is already running in its lab configuration.
- `grade-lab-util`'s `sleep` tests break on the `sys_pause` breakpoint, not `sys_sleep`: the clock-tick call is named `pause` in this tree, and `sleep` is the user program you write on top of it.
- The two `find ... -exec` tests are a `fork`/`exec`/`wait` exercise in user space — `find` has to do per-match what `runcmd()`'s `EXEC` arm does per command.
- `sh < findtest.sh` is the descriptor argument above, graded: the shell reads its commands from fd 0 without a single line of code that knows it is not the console.

[ostep]: ../../../operating-system/The_Process_Abstraction.md
