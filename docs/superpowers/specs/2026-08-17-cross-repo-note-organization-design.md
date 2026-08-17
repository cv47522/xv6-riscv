# Cross-Repo Note Organization Design

## Goal

Stop the xv6 book notes in `xv6-riscv/docs/book/` from duplicating the OSTEP notes in `../operating-system/`, without merging the repositories or making either one depend on a build step. The immediate trigger is xv6 book Chapter 1 (`unix.tex`), which covers processes, `fork`/`exec`/`wait`, file descriptors, pipes, and the shell — very nearly the same ground as `The_Process_Abstraction.md` §1–7 (OSTEP Ch 4–6).

## Context

Four sources cover the same material from different angles, and nothing currently connects them.

| Source | Location | Angle |
| ------ | -------- | ----- |
| **OSTEP notes** | `../operating-system/*.md` | Portable concepts, worked arithmetic, measurements on this machine |
| **OSTEP demos** | `../operating-system/codes/src/main/virtualization/` | Runnable C and the official simulators |
| **xv6 book** | `../xv6-riscv-book/*.tex` | One concrete system, explained by its authors |
| **xv6 source and labs** | `kernel/`, `user/`, `grade-lab-*` | The implementation itself |

Two constraints shape the design. The repositories live on different hosts (`operating-system` on GitLab, `xv6-riscv` on GitHub), so no relative cross-repo link can render on both web UIs. And both repositories already carry hub conventions that were built independently: the OSTEP notes have `Prerequisites` / `Related Notes` / `Terminology` sections, while `docs/README.md` nominates `04-terminology.md` as its glossary hub. Nothing bridges them.

## Approach

### The ownership rule

`operating-system/` owns concepts. `xv6-riscv/docs/book/` owns xv6's realization of them.

The test when placement is unclear: **would this sentence still be true on Linux?** Yes means it belongs in the OSTEP repository; no means it belongs here. The operative corollary is that an xv6 chapter note may not define a concept — if it needs one defined, it links. A chapter note that explains what `fork` *is* has violated the rule.

This is the existing `AGENTS.md` rule ("distil the source, never transcribe it") applied across a repository boundary rather than within one.

### The concordance

A new file, `docs/book/00-ostep-concordance.md`, maps one concept across all four sources:

| Concept | OSTEP note § | Demo (`codes/`) | xv6 ch | xv6 source | Lab |
| ------- | ------------ | --------------- | ------ | ---------- | --- |
| **fork/exec/wait** | `The_Process_Abstraction.md` §7 | `cpu-process-api/fork_*.c` | 1 `unix.tex` | `proc.c` `kfork()`, `exec.c` `kexec()` | util |
| **process states, PCB** | `The_Process_Abstraction.md` §4–5 | `cpu-process-intro/` | 1 `unix.tex` | `proc.c` `allocproc()` | — |
| **limited direct execution** | `The_Process_Abstraction.md` §8 | `cpu-process-mechanisms/lde_cost.c` | 4 `trap.tex` | `trap.c` `usertrap()` | traps |
| **context switch** | `CPU_Scheduling.md` | — | 7 `sched.tex` | `swtch.S`, `proc.c` `sched()` | — |
| **paging, Sv39** | `Paging.md` | `vm-paging/` | 3 `mem.tex` | `vm.c` `walk()` | pgtbl |
| **file descriptors, pipes** | `The_Process_Abstraction.md` §7 | `cpu-process-api/homework/parent_children_pipe.c` | 1 `unix.tex` | `pipe.c` `pipealloc()`, `user/sh.c` `runcmd()` | util |

Its purpose is procedural rather than referential: it is consulted **before** a chapter note is written, and it answers "is this already explained somewhere?" The seed rows above cover the concepts whose OSTEP notes are already written; further rows are added as chapters are read.

Symbol names lead and line numbers follow, because `proc.c` `kfork()` survives an edit above it while `proc.c:259` does not.

The concordance does not overlap the chapter checklist already in `docs/book/README.md`. That table tracks progress per chapter; this one tracks one concept across four sources. Because they share no columns, they cannot drift into contradicting each other.

### Chapter-note shape

The "Suggested shape" block in `docs/book/README.md` is replaced by one that enforces the ownership rule structurally:

```markdown
# Chapter N: Title

> **Theory:** [OSTEP — …][ostep-x]. Read that first; this note does not re-explain it.

## What xv6 actually does
## Naming and framing differences
## Code walked through
## Questions I had
## Lab connection
```

`What xv6 actually does` is the only mandatory section. The opening blockquote is load-bearing: it is the note declaring itself deliberately incomplete, so that a future reader does not mistake a gap for an omission.

`Naming and framing differences` exists because this xv6 revision renames the kernel-side system call implementations to `kfork`, `kexec`, `kwait`, and `kexit` (`kernel/proc.c:259`, `kernel/exec.c:28`), while both the book prose and OSTEP say `fork`, `exec`, and `wait`. Divergences of that kind belong in the bridge and nowhere else.

### Backlinks

Each OSTEP note gains a single line in its **existing** `## Related Notes` section. No new sections and no new files are added to that repository.

```markdown
- [xv6 Ch 1 — Operating system interfaces](../xv6-riscv/docs/book/ch01-operating-system-interfaces.md) — the same `fork`/`exec`/`wait`, implemented in ~6k readable lines (kernel-side: `kfork`/`kexec`/`kwait`).
```

### Terminology

The two glossaries stay separate but stop competing. `04-terminology.md` holds xv6, RISC-V, and toolchain vocabulary (hart, PLIC, Sv39, `ecall`); the OSTEP notes' `## Terminology` sections hold portable concepts (process, PCB, limited direct execution). The overlap is small — roughly *context switch*, *trap*, and *process* — and the same rule resolves it: the definition lives on the OSTEP side, and `04-terminology.md` may carry a one-line gloss plus a link but never a second definition.

### Link mechanics

Cross-repo links use relative paths: `../../operating-system/…` from `docs/book/`, and `../xv6-riscv/…` from the OSTEP repository root. These are clickable in the editor, survive renames, and 404 on both web UIs by design. `docs/book/README.md` states this explicitly, along with the assumption that both repositories sit side by side under `~/personal/`, so the dead web links are not later "fixed" into absolute URLs.

## Scope

In this pass:

- Add `docs/book/00-ostep-concordance.md` with the seed rows.
- Update `docs/book/README.md` with the new chapter-note shape, the link convention, and a pointer to the concordance.
- Write `docs/book/ch01-operating-system-interfaces.md` under the rule, as the proof that the rule produces a useful note.
- Add one backlink line to `../operating-system/The_Process_Abstraction.md`.

Deliberately excluded: backlinks in the remaining OSTEP notes. Each is added when its chapter is reached, so no link ever points at a note that does not exist yet.

## Constraints

- Do not hard-wrap prose, and do not reflow paragraphs outside the lines being changed.
- Restate rather than transcribe; the xv6 book text is not copied into the notes.
- Reference kernel code by symbol name first, line number second.
- Add new vocabulary to a glossary rather than defining it inline.
- Changes to `../operating-system/` are limited to appending backlink lines within existing sections.

## Accepted trade-off

Chapter notes stop being standalone. Reading `ch01-operating-system-interfaces.md` alone leaves gaps that only the OSTEP note fills. This is the correct trade for a personal repository whose author and only reader are the same person, and it would be the wrong trade for a shared wiki.

## Verification

- Confirm every relative cross-repo path resolves from its containing file.
- Confirm every heading anchor referenced by a cross-repo link exists in the target note.
- Confirm every symbol named in the concordance exists at the cited file, by grep rather than by memory.
- Confirm `ch01-operating-system-interfaces.md` defines no concept that the ownership rule assigns to the OSTEP repository.
- Inspect the diff for accidental prose reflow and newly introduced hard wraps.
