# xv6-riscv Study Notes

---

[toc]

---

Personal notes and guides for learning [xv6-riscv](https://github.com/mit-pdos/xv6-riscv) alongside the [xv6 book](https://mit-pdos.github.io/xv6-riscv-book/) and MIT's [6.1810](https://pdos.csail.mit.edu/6.1810/2026/) labs.

## Reading order

If you are setting this up for the first time, read these in order. Each one assumes the previous.

| Guide                                                        | What it covers                                                                                                                                                    |
| ------------------------------------------------------------ | ----------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **[00-repo-workflow.md](00-repo-workflow.md)**               | How this repository is wired: remotes, branches, tags, and the exact commands to sync, push, and recover. Read this first — it explains _where_ everything lives. |
| **[01-environment-setup.md](01-environment-setup.md)**       | Installing the toolchain, and what each `apt` package actually does.                                                                                              |
| **[02-build-boot-and-usage.md](02-build-boot-and-usage.md)** | `make qemu`, reading the boot output, using the xv6 shell, and debugging with gdb.                                                                                |
| **[03-lab-workflow.md](03-lab-workflow.md)**                 | Running the 6.1810 labs from this repo: `conf/lab.mk`, `make grade`, and merging each lab branch.                                                                 |
| **[04-terminology.md](04-terminology.md)**                   | Glossary. Every acronym expanded — xv6, RISC-V, QEMU, hart, PLIC, and the rest.                                                                                   |
| **[book/](book/)**                                           | Chapter-by-chapter notes from the xv6 book, added as they are read.                                                                                               |

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

Create one file per chapter under `book/`, named for the chapter:

```
docs/book/ch01-operating-system-interfaces.md
docs/book/ch03-page-tables.md
```

Keep them linked to the source they describe — `kernel/vm.c:57` style references are clickable in most editors — and add any new vocabulary to the glossary rather than defining it inline.
