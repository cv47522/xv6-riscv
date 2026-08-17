# xv6-riscv Study Notes

---

[toc]

---

Personal notes and guides for learning [xv6-riscv](https://github.com/mit-pdos/xv6-riscv) alongside the [xv6 book](https://mit-pdos.github.io/xv6-riscv-book/) and MIT's [6.1810](https://pdos.csail.mit.edu/6.1810/2026/) labs.

## Reading order

If you are setting this up for the first time, read these in order. Each one assumes the previous.

| Guide                                                            | What it covers                                                                                                                                                              |
| ---------------------------------------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **[00-repo-workflow.md](00-repo-workflow.md)**                   | How this repository is wired: remotes, branches, tags, and the exact commands to sync, push, and recover. Read this first — it explains _where_ everything lives.           |
| **[01-environment-setup.md](01-environment-setup.md)**           | Installing the toolchain, and what each `apt` package actually does.                                                                                                        |
| **[02-build-boot-and-usage.md](02-build-boot-and-usage.md)**     | `make qemu`, reading the boot output, using the xv6 shell, and the debugging toolkit: gdb, `kernel.asm`, `addr2line`, and the QEMU monitor.                                 |
| **[03-lab-workflow.md](03-lab-workflow.md)**                     | Running the 6.1810 labs from this repo: `conf/lab.mk`, `make grade`, merging each lab branch, and how to approach a lab.                                                    |
| **[04-terminology.md](04-terminology.md)**                       | Glossary. Every acronym expanded — xv6, RISC-V, QEMU, hart, PLIC, and the rest.                                                                                             |
| **[05-syscall-reference.md](05-syscall-reference.md)**           | All 22 system calls by group — signature, return value, failure mode, and implementing file. xv6 has no man pages; this is the lookup. Not read in order; consulted.        |
| **[06-build-artifacts.md](06-build-artifacts.md)**               | What `.o`, `.d`, `.asm`, `.sym`, `.S`, `.pl`, and `.ld` files are, which are generated, and why a kernel needs file types an ordinary C project does not.                   |
| **[book/](book/)**                                               | Chapter-by-chapter notes from the xv6 book, added as they are read.                                                                                                         |
| **[book/00-ostep-concordance.md](book/00-ostep-concordance.md)** | One concept per row, mapped across the xv6 book, the OSTEP notes in `../operating-system/`, this tree's source, and the exercises. Read it before writing any chapter note. |

> [!TIP]
> The glossary is the hub. Rather than re-explaining a term in each guide, the other documents link into [04-terminology.md](04-terminology.md). When a chapter introduces a new term, add it there once and link to it.

## How this repo is arranged

This is a single consolidated repository. It holds the upstream xv6 source, these notes, MIT's grading harness, and lab implementations — all on one branch, so nothing requires a branch switch to read or edit.

```
xv6-riscv/
  kernel/          the xv6 kernel
  user/            user programs (the contents of fs.img)
  mkfs/            host-side tool that builds the filesystem image
  conf/lab.mk      selects the active lab (LAB=util)
  grade-lab-*      MIT grading scripts
  gradelib.py      the grading harness they run on
  docs/            these notes
  Makefile         the build, heavily commented as a teaching aid
```

Branches and remotes are covered in [00-repo-workflow.md](00-repo-workflow.md). The short version: `riscv` is an untouched mirror of upstream, and `study` is where all work happens.

## Adding chapter notes

All thirteen chapter files already exist under `book/`, one per chapter of the xv6 book; writing a note means filling a stub in, not creating a file.

Read [book/00-ostep-concordance.md](book/00-ostep-concordance.md) first. It lists every concept the two note sets cover, and for each one it names the OSTEP note, the xv6 chapter, the source symbols, and the exercise — so you can see what is already explained before explaining it again.

The rule the concordance encodes: **`../operating-system/` owns concepts; `docs/book/` owns xv6's realization of them.** A concept whose OSTEP note is not written yet sits on loan in the xv6 note until it is, which is why each concordance row carries a status.

[book/README.md](book/README.md) has the full procedure — the three-case table, the note shape, and the blockquote variant that goes with each case. Do not restate any of it here; that file is the single authority.

Keep notes linked to the source they describe — `kernel/vm.c:57` style references are clickable in most editors — and add any new vocabulary to the glossary rather than defining it inline.
