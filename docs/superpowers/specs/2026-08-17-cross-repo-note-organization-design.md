# Cross-Repo Note Organization Design

## Goal

Establish a repeatable discipline for reading all thirteen chapters of the xv6 book without duplicating the OSTEP notes in `../operating-system/`, and without merging the repositories or introducing a build step. Chapter 1 (`unix.tex`) is the first application of the discipline, not its subject: it is simply the chapter where the overlap is currently visible, because it covers processes, `fork`/`exec`/`wait`, file descriptors, pipes, and the shell — very nearly the ground of `The_Process_Abstraction.md` §1–7 (OSTEP Ch 4–6).

The design has to hold for the other twelve chapters too, and most of them are harder than Chapter 1 for a reason given below: **seven of the thirteen OSTEP counterparts are unwritten placeholders**, so "link to the concept instead of restating it" has nowhere to point.

## Context

Six sources cover overlapping material from different angles, and nothing currently connects them.

| Source | Location | Angle |
| ------ | -------- | ----- |
| **OSTEP notes** | `../operating-system/*.md` | Portable concepts, worked arithmetic, measurements on this machine |
| **OSTEP demos** | `../operating-system/codes/src/main/` | Runnable C written for these notes |
| **OSTEP homework** | `../ostep-homework/` | The book's official simulators |
| **OSTEP projects** | `../ostep-projects/` | Course projects, six of them xv6-based |
| **xv6 book** | `../xv6-riscv-book/*.tex` | One concrete system, explained by its authors |
| **xv6 source and labs** | `kernel/`, `user/`, `grade-lab-*` | The implementation itself |

Three constraints shape the design.

The repositories live on different hosts (`operating-system` on GitLab, `xv6-riscv` on GitHub), so no relative cross-repo link can render on both web UIs.

Both repositories already carry hub conventions built independently: the OSTEP notes have `Prerequisites` / `Related Notes` / `Terminology` sections defined by `templates/NOTE_TEMPLATE.md`, while `docs/README.md` nominates `04-terminology.md` as its glossary hub. Nothing bridges them.

**`ostep-projects` and the OSTEP notes target a different xv6.** `INSTALL-xv6.md` clones `mit-pdos/xv6-public`, the older x86 tree, and `The_Process_Abstraction.md` links to that same repository in its Useful Links. This tree is `xv6-riscv`. The two differ in architecture, in file layout, and in naming — this revision calls the kernel-side system call implementations `kfork`, `kexec`, `kwait`, and `kexit` (`kernel/proc.c:259`, `kernel/exec.c:28`) where both the book prose and OSTEP say `fork`, `exec`, and `wait`. Every cross-reference must therefore say *which* xv6 it means.

## Approach

### The ownership rule

`operating-system/` owns concepts. `xv6-riscv/docs/book/` owns xv6's realization of them.

The test when placement is unclear: **would this sentence still be true on Linux?** Yes means it belongs in the OSTEP repository; no means it belongs here. The operative corollary is that an xv6 chapter note may not define a concept — if it needs one defined, it links.

This is the existing `AGENTS.md` rule ("distil the source, never transcribe it") applied across a repository boundary rather than within one.

### Three cases, because the rule alone does not scale

The rule as stated assumes the concept note exists. For Chapter 1 it does. For most chapters it does not, and a rule that says "link to `File_Systems.md`" when `File_Systems.md` is a 286-byte placeholder produces a chapter note that explains nothing and links to nothing. Every concept therefore falls into one of three cases, and the concordance records which.

| Case | When | Where the explanation goes |
| ---- | ---- | -------------------------- |
| **Linked** | The OSTEP counterpart is written | The xv6 note links and defines nothing. |
| **On loan** | The OSTEP counterpart is a placeholder | The xv6 note carries a short, explicitly marked `Concept on loan` block. |
| **xv6-only** | No OSTEP counterpart exists or can | The xv6 note owns it permanently. |

**On loan** is the case that makes the design survive contact with the reading order. Concepts are borrowed, not annexed: the block is capped at a few paragraphs, marked with a fixed heading so it is greppable, and carries the name of the OSTEP note it is destined for. When that note is written, the block moves there and a link replaces it. The concordance status flips from `loan` to `linked`, which is what makes the debt visible rather than forgotten.

**xv6-only** is not a failure of the mapping — it is most of what makes xv6 worth reading. RISC-V machine mode and the boot path in `entry.S`, the trampoline page, `swtch.S`, the `fence` instructions in `spinlock.c:70` and `virtio_disk.c:277`, and the whole of Chapter 13 have no OSTEP counterpart and are never on loan. They are the xv6 notes' own material.

### The concordance

A new file, `docs/book/00-ostep-concordance.md`, maps one concept across every source. It is consulted **before** a chapter note is written, and it answers "is this already explained somewhere, and if not, who owns it?"

| Concept | OSTEP note | xv6 ch | xv6 source | Exercise | Status |
| ------- | ---------- | ------ | ---------- | -------- | ------ |
| **fork/exec/wait** | `The_Process_Abstraction.md` §7 | 1 | `proc.c` `kfork()`, `exec.c` `kexec()` | `cpu-process-api/fork_*.c` | linked |
| **process states, PCB** | `The_Process_Abstraction.md` §4–5 | 1 | `proc.c` `allocproc()` | `cpu-process-intro/` | linked |
| **fds, pipes, shell** | `The_Process_Abstraction.md` §7 | 1 | `pipe.c` `pipealloc()`, `user/sh.c` `runcmd()` | `parent_children_pipe.c` | linked |
| **kernel/user split** | `Introduction_to_Operating_Systems.md` | 2 | `main.c`, `proc.c` `allocproc()` | — | linked |
| **machine mode, boot** | — | 2 | `entry.S`, `start.c` | — | xv6-only |
| **paging, Sv39** | `Paging.md` | 3 | `vm.c` `walk()`, `uvmalloc()` | `vm-paging/` | linked |
| **address spaces** | `Address_Spaces_And_Translation.md` | 3 | `vm.c` `uvmcopy()` | `vm-mechanism/` | linked |
| **memory layout, segments** | `Memory_Management.md` | 3 | `kernel/memlayout.h`, `user/user.ld` | `vm-freespace/` | linked |
| **limited direct execution** | `The_Process_Abstraction.md` §8 | 4 | `trap.c` `usertrap()`, `kerneltrap()` | `lde_cost.c` | linked |
| **trampoline, trapframe** | — | 4 | `trampoline.S` | lab: traps | xv6-only |
| **page faults, COW** | `Swapping_And_VM_Systems.md` | 5 | `trap.c` `usertrap()`, `vm.c` `uvmcopy()` | lab: cow | loan |
| **device drivers, PLIC** | `IO_Devices_And_Disks.md` | 6 | `uart.c` `uartintr()`, `plic.c` `plic_claim()` | `file-devices/` | loan |
| **spinlocks, races** | `Concurrency_Threads_And_Locks.md` | 7 | `spinlock.c` `acquire()`, `release()` | `threads-locks/` | loan |
| **context switch** | `CPU_Scheduling.md` | 8 | `swtch.S`, `proc.c` `sched()`, `scheduler()` | `cpu-sched/` | linked |
| **sleep/wakeup, cond vars** | `Condition_Variables_And_Semaphores.md` | 9 | `proc.c` `sleep()`, `wakeup()` | `threads-cv/` | loan |
| **inodes, buffer cache** | `File_Systems.md` | 10 | `fs.c` `ialloc()`, `bio.c` `bread()` | `file-implementation/` | loan |
| **journaling, crash consistency** | `Crash_Consistency_And_Journaling.md` | 11 | `log.c` `begin_op()`, `write_log()` | `file-journaling/` | loan |
| **memory ordering, fences** | `Concurrency_Bugs_And_Events.md` | 12 | `spinlock.c:70`, `virtio_disk.c` `io_fence()` | — | loan |
| **summary** | — | 13 | — | — | xv6-only |

Symbol names lead and line numbers follow, because `proc.c` `kfork()` survives an edit above it while `proc.c:259` does not. Line numbers appear only where the reference is to a comment or a specific instruction rather than a function.

The `Exercise` column spans `codes/`, `../ostep-homework/`, and `../ostep-projects/` — three repositories, distinguished by path when written out in full. The six xv6-based entries in `ostep-projects` are annotated as targeting `xv6-public`, so they are never mistaken for labs runnable in this tree.

The concordance does not overlap the chapter checklist in `docs/book/README.md`. That table tracks progress per chapter; this one tracks one concept across six sources. They share no columns, so they cannot drift into contradicting each other.

### Chapter-note shape

The "Suggested shape" block in `docs/book/README.md` is replaced by one that enforces the ownership rule structurally, and accommodates all three cases:

```markdown
# Chapter N: Title

> **Theory:** [OSTEP — …][ostep-x]. Read that first; this note does not re-explain it.

## What xv6 actually does
## Concept on loan: <topic>        <- only while the OSTEP note is a placeholder
## Naming and framing differences
## Code walked through
## Questions I had
## Lab connection
```

`What xv6 actually does` is the only mandatory section. The opening blockquote is load-bearing: it is the note declaring itself deliberately incomplete, so a future reader does not mistake a gap for an omission. In an `xv6-only` chapter the blockquote instead states that no OSTEP counterpart exists, so the note stands alone by design.

`Concept on loan:` is a fixed, greppable heading. Its presence is a debt, and `grep -rn '^## Concept on loan' docs/book/` lists every one outstanding.

`Naming and framing differences` carries the `kfork`/`kexec`/`kwait` divergence in Chapter 1, and in later chapters the equivalent gaps between the book's vocabulary, OSTEP's, and this tree's. Divergences of that kind belong in the bridge and nowhere else.

### Backlinks

Each OSTEP note with an xv6 counterpart gains a single line in its `## Related Notes` section, naming the tree explicitly so it is not confused with the `xv6-public` links already present:

```markdown
- [xv6 Ch 1 — Operating system interfaces](../xv6-riscv/docs/book/ch01-operating-system-interfaces.md) — the same `fork`/`exec`/`wait` in the RISC-V tree (kernel-side: `kfork`/`kexec`/`kwait`), not the `xv6-public` tree linked above.
```

Thirteen OSTEP notes have a counterpart, in three groups:

| Group | Notes | Change |
| ----- | ----- | ------ |
| **Has `## Related Notes`** | `The_Process_Abstraction.md`, `Paging.md`, `Address_Spaces_And_Translation.md`, `CPU_Scheduling.md` | Append one line |
| **Written, section missing** | `Introduction_to_Operating_Systems.md`, `Memory_Management.md` | Insert the section, then the line |
| **Placeholder** | `Swapping_And_VM_Systems.md`, `IO_Devices_And_Disks.md`, `Concurrency_Threads_And_Locks.md`, `Condition_Variables_And_Semaphores.md`, `File_Systems.md`, `Crash_Consistency_And_Journaling.md`, `Concurrency_Bugs_And_Events.md` | Append the section after the placeholder blockquote |

Creating the section is not a deviation. `templates/NOTE_TEMPLATE.md` already lists `## Related Notes` and orders it `Useful Links` → `Prerequisites` → `Related Notes` → `Terminology`, so in the two written notes it is inserted between `## Useful Links` and `## Terminology` — exactly where the four notes that already have it keep it. Placeholder blockquotes and their pointers to `TODO.md#note-map` are left untouched.

For a placeholder note, the backlink does double duty: it tells future-you, at the moment you finally write that note, that an xv6 chapter note already exists and may be holding a concept on loan that belongs here.

Five OSTEP notes get no backlink, because no xv6 chapter corresponds: `SSDs_And_Data_Integrity.md`, `Distributed_Systems.md`, `Registers_Caches_And_Buffers.md`, `C_Library_Reference.md`, and `Linux_Reference.md`.

### Terminology

The two glossaries stay separate but stop competing. `04-terminology.md` holds xv6, RISC-V, and toolchain vocabulary (hart, PLIC, Sv39, `ecall`); the OSTEP notes' `## Terminology` sections hold portable concepts (process, PCB, limited direct execution). The overlap is small — roughly *context switch*, *trap*, and *process* — and the same rule resolves it: the definition lives on the OSTEP side, and `04-terminology.md` may carry a one-line gloss plus a link but never a second definition. Where the concept is `xv6-only`, `04-terminology.md` owns the definition outright.

### Link mechanics

Cross-repo links use relative paths: `../../operating-system/…` from `docs/book/`, and `../xv6-riscv/…` from the OSTEP repository root. They are clickable in the editor, survive renames, and 404 on both web UIs by design. `docs/book/README.md` states this explicitly, along with the assumption that the repositories sit side by side under `~/personal/`, so the dead web links are not later "fixed" into absolute URLs.

## Chapter map

The book has **thirteen** chapters, which `docs/book/README.md` currently gets wrong: its table lists ten, omits *Page faults*, *Sleep and Wakeup*, and *Logging*, and misnumbers every chapter from five onward. The mapping below is taken from the `\chapter{}` line of each source file in the order `book.tex` includes them, and correcting the README table is part of this work.

| # | Chapter | Source | OSTEP counterpart | Case |
| - | ------- | ------ | ----------------- | ---- |
| 1 | Operating system interfaces | `unix.tex` | `The_Process_Abstraction.md` | linked |
| 2 | Operating system organization | `first.tex` | `Introduction_to_Operating_Systems.md` | mixed |
| 3 | Page tables | `mem.tex` | `Paging.md`, `Address_Spaces_And_Translation.md`, `Memory_Management.md` | linked |
| 4 | Traps and system calls | `trap.tex` | `The_Process_Abstraction.md` §8 | mixed |
| 5 | Page faults | `pgfault.tex` | `Swapping_And_VM_Systems.md` | loan |
| 6 | Interrupts and device drivers | `interrupt.tex` | `IO_Devices_And_Disks.md` | loan |
| 7 | Locking | `lock.tex` | `Concurrency_Threads_And_Locks.md` | loan |
| 8 | Scheduling | `sched.tex` | `CPU_Scheduling.md` | linked |
| 9 | Sleep and Wakeup | `sleep.tex` | `Condition_Variables_And_Semaphores.md` | loan |
| 10 | File system | `fs.tex` | `File_Systems.md` | loan |
| 11 | Logging | `log.tex` | `Crash_Consistency_And_Journaling.md` | loan |
| 12 | Concurrency revisited | `lock2.tex` | `Concurrency_Bugs_And_Events.md` | loan |
| 13 | Summary | `sum.tex` | — | xv6-only |

Seven of thirteen chapters are `loan` and two are `mixed`, which is the quantitative reason the three-case rule is load-bearing rather than an edge case. The seven match the seven placeholder OSTEP notes exactly, as they must.

## Per-chapter procedure

The discipline is a loop, run once per chapter as it is read. This is the part that scales; everything above is the structure it operates on.

1. **Read the concordance first.** Find the rows for this chapter and note their status.
2. **For `linked` rows**, write only what xv6 does, and link the theory. Define nothing.
3. **For `loan` rows**, write the `## Concept on loan` block: a few paragraphs, capped, naming the OSTEP note it is destined for. Set the row's status to `loan` if it was not already.
4. **For `xv6-only` rows**, write freely — this note is the permanent home. Add the vocabulary to `04-terminology.md`.
5. **Add any new concept rows** the chapter introduced, with their case assigned.
6. **Fill the chapter's row** in the `docs/book/README.md` checklist.

And a second loop, run whenever an OSTEP placeholder note is finally written:

1. `grep -rn '^## Concept on loan' docs/book/` to find blocks destined for that note.
2. Move each block into the OSTEP note, restating it in that note's voice.
3. Replace the block in the xv6 note with a link, and flip the concordance status to `linked`.

## Scope

In this pass:

- Add `docs/book/00-ostep-concordance.md` with the nineteen seed rows above.
- Update `docs/book/README.md`: correct the chapter table to the thirteen chapters, and add the new chapter-note shape, the link convention, the three-case rule, and a pointer to the concordance.
- Stub all thirteen chapter notes under `docs/book/`, each carrying its title, its `> **Theory:**` line (or the `xv6-only` variant), the section headings from the shape above, and a placeholder marker in the style the OSTEP repository already uses.
- Write `docs/book/ch01-operating-system-interfaces.md` in full, as the worked example of a `linked` chapter.
- Add the backlink line to all thirteen counterpart OSTEP notes, creating `## Related Notes` in the nine that lack it.

Stubbing every chapter up front keeps the concordance and both link directions complete and symmetric from the first commit, so no link ever dangles. The cost is thirteen mostly-empty files; the benefit is that the structure is visible as a whole, which is what made the three missing chapters obvious.

Not in this pass: any `loan` block. Those are written when their chapter is read, by the procedure above. Chapter 1 is `linked`, so this pass produces no debt.

## Constraints

- Do not hard-wrap prose, and do not reflow paragraphs outside the lines being changed.
- Restate rather than transcribe; no xv6 book text is copied into the notes.
- Reference kernel code by symbol name first, line number second.
- Add new vocabulary to a glossary rather than defining it inline.
- Every cross-reference names which xv6 it means when ambiguity is possible.
- Changes to `../operating-system/` are limited to the backlink line and, where absent, the `## Related Notes` heading that holds it. No existing prose there is edited, reordered, or reflowed, and it commits separately because it is a separate repository.

## Accepted trade-offs

**Chapter notes stop being standalone.** Reading `ch01-operating-system-interfaces.md` alone leaves gaps only the OSTEP note fills. Correct for a personal repository whose author and only reader are the same person; wrong for a shared wiki.

**`loan` blocks are deliberate, tracked duplication.** They will need moving later, and some will be forgotten. The fixed greppable heading and the concordance status column are the entire mitigation, and they are enough only because both are checked by the second loop above.

## Verification

- Confirm every relative cross-repo path resolves from its containing file, in both directions, by testing the paths rather than reading them.
- Confirm the corrected chapter table matches the `\chapter{}` lines of `../xv6-riscv-book/*.tex` in `book.tex` include order.
- Confirm every symbol named in the concordance exists at the cited file, by grep rather than by memory.
- Confirm each of the thirteen stubs exists, is reachable from the corrected README table, and carries a theory line whose target resolves.
- Confirm every concordance row has a case assigned, and that no row is `linked` to a placeholder note.
- Confirm `ch01-operating-system-interfaces.md` defines no concept the ownership rule assigns to the OSTEP repository.
- Inspect the diff for accidental prose reflow and newly introduced hard wraps.
