# Prompt: sync book chapter notes from the upstream book

---

[toc]

---

Portable, agent-neutral. Copy this whole file into any coding agent, replace the target on the first line, and run it. It assumes only a shell, `git`, and file access.

---

**Target chapters:** `<REPLACE ME — e.g. "ch3", "3-4", "next", or "all">`

If the target is empty, ask. `next` means the chapters assigned to the first open lecture in [`STUDY-PROGRESS.md`](../../STUDY-PROGRESS.md) (Step 2 resolves it); `all` means every chapter the upstream book changed since the last sync plus every chapter still a scaffold. **Do not** infer the target from `conf/lab.mk` or recently edited files.

---

You are refreshing the read-only book checkout, saving every figure the target chapters use, and creating or updating their notes under [`docs/book/`](../book/README.md). The chapter notes describe **this tree's** realization of the book; they are not a paraphrase of the book.

> **First, read `AGENTS.md` at the repository root**, then [`docs/book/README.md`](../book/README.md) (ownership, the three cases, note shape, figure captions), [`docs/book/00-ostep-concordance.md`](../book/00-ostep-concordance.md), and [`docs/book/fig/README.md`](../book/fig/README.md). This prompt does not restate their rules.

## Step 1 — Refresh the book checkout without modifying it

`~/personal/xv6-riscv-book` is a fork whose branch tracks upstream `mit-pdos/xv6-riscv-book`. It is **input only**: bring in upstream commits by fast-forward and nothing else.

```bash
B=~/personal/xv6-riscv-book
UP=git@github.com:mit-pdos/xv6-riscv-book.git
git -C "$B" status --porcelain                  # expect no output; else STOP
OLD=$(git -C "$B" rev-parse HEAD)
BR=$(git ls-remote --symref "$UP" HEAD | sed -n 's|^ref: refs/heads/\(.*\)\tHEAD|\1|p')
git -C "$B" fetch "$UP" "$BR"
git -C "$B" merge --ff-only FETCH_HEAD          # fails rather than merging
git -C "$B" log --oneline "$OLD"..HEAD
git -C "$B" diff --stat "$OLD"..HEAD -- '*.tex' fig/
```

> [!CAUTION]
> **Never** edit, commit, rebase, push, add a remote, or run the book's `make` inside `$B`. `make` writes `latex.out/`, `book.aux`, and other build products into the checkout. If the worktree is dirty or the fast-forward fails, stop and report the output; do not stash, reset, or merge. Fetching by URL keeps even `.git/config` unchanged.

Read the last-synced commit from the `Synced against` line in [`docs/book/README.md`](../book/README.md); if the line is missing, treat every chapter as changed. `git -C "$B" diff <synced>..HEAD -- <file>.tex` is the change list a note **must** account for.

## Step 2 — Resolve the target from `STUDY-PROGRESS.md`

| Chapter | Source          | Chapter | Source      |
| ------- | --------------- | ------- | ----------- |
| **1**   | `unix.tex`      | **8**   | `sched.tex` |
| **2**   | `first.tex`     | **9**   | `sleep.tex` |
| **3**   | `mem.tex`       | **10**  | `fs.tex`    |
| **4**   | `trap.tex`      | **11**  | `log.tex`   |
| **5**   | `pgfault.tex`   | **12**  | `lock2.tex` |
| **6**   | `interrupt.tex` | **13**  | `sum.tex`   |
| **7**   | `lock.tex`      |         |             |

`book.tex`'s `\input` order is the authority if this table ever disagrees with it.

- For `next`, take the first unchecked lecture under `## Lectures`, then the `**Book:** Chapter N` line of the homework with the same number under `## Homework and assigned reading` (Lecture 5 reads Chapter 7, for example). A lecture with no book reading has no chapter target; say so and stop.
- Writing or refreshing a note is **not** evidence of reading. **Never** check a book or chapter box in `STUDY-PROGRESS.md`; record only that the note exists or changed, as the tracker's Book reading row already does for scaffolds.

## Step 3 — Save every figure the chapter uses

List the chapter's figures straight from its source:

```bash
grep -n 'includegraphics\|input{fig/' "$B"/<file>.tex
ls "$B"/fig/
```

Every hit **must** end up in exactly one of these states, recorded in [`docs/book/fig/README.md`](../book/fig/README.md):

| Upstream form                            | What to do                                                                                                                                                                                                                  |
| ---------------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **`.svg` beside the referenced file**    | Copy through [`prep-figures.py`](../book/fig/prep-figures.py) (crop plus white backdrop), never by hand. Re-run it for an existing copy whose upstream file changed in Step 1's diff.                                       |
| **TikZ `fig/*.tex` or PDF-only**         | If `pdflatex` and `pdf2svg` exist, render with upstream's `fig/tikz-to-svg.sh <abs path> <docs/book/fig/name.svg>` (or `pdf2svg` for a PDF), then run `prep-figures.py` on the result. Otherwise keep or update the redraw. |
| **Redraw already in a note**             | Compare it against the changed upstream source and fix any label, field, or order that drifted.                                                                                                                             |

```bash
cd docs/book/fig
python3 prep-figures.py ../../../../xv6-riscv-book/fig . <name.svg> [...]
```

- `tikz-to-svg.sh` writes only to its temporary directory and the output path you give it, so it does not touch `$B`. Point the output inside `docs/book/fig/`, never inside `$B/fig/`.
- Keep upstream leftovers that no `.tex` includes (`bufrace.svg`, `deadlock.svg`) out, as the figure README explains.
- Update both inventory tables in the figure README and the `python3 prep-figures.py` reproduction command whenever a file is added, replaced, or converted from a redraw.
- A figure whose subject is portable (a race, SMP, a microkernel) may additionally be used by a sibling note; that copy is governed by [`write-lecture-note.md`](write-lecture-note.md#step-4--place-figures), not here.

## Step 4 — Write or update the chapter note

Read the chapter's `.tex` in full, then the source it cites in **this** tree. The book describes upstream xv6; this tree renames and changes things.

1. **Classify each concept** with the concordance: linked, loan, or xv6-only. Add a concordance row for any concept the chapter introduces that has none, and keep the note's opening blockquote matched to its case.
2. **Verify every symbol before citing it.** `grep -n` the function or constant under `kernel/` and `user/`; cite `kernel/<file>.c` plus the symbol, and a line number only from the current file. Names known to differ: `kfork`/`kexec`/`kwait`/`kexit`, `pause` (the clock-tick system call), and `kernel/printk.c`.
3. **Fill the standard sections** from [`docs/book/README.md`](../book/README.md#shape): `Figures` first, then `What xv6 actually does`, `Divergences`, `Code walked through`, `Questions I had`, and `Lab connection`. Omit a section the chapter has nothing for; leave a chapter the learner has not read as a scaffold plus figures rather than inventing questions on their behalf.
4. **Caption each figure in the three moves** the book README requires: the book's caption in quotes, what is drawn, and why this note's reader looks at it. Cite by source and label, never by figure number.
5. **Record divergences** where this tree differs from the book text, with both sides and the file that shows it. When the Step 1 diff changed a passage this note relies on, update the note and mention it in the report.
6. **Do not copy book prose.** Summarize in your own words and quote only a caption or a short definition where the exact wording matters.
7. **Do not explain portable concepts here.** A concept whose OSTEP note is written gets a link; one still a placeholder gets a `## Concept on loan` block per the book README. Generic C goes to `../c-programming/`. When you notice a sibling note missing something the chapter makes clear, list it in the report instead of editing the sibling repository; [`write-lecture-note.md`](write-lecture-note.md) is where sibling edits happen.

Then update the chapter row in [`docs/book/README.md`](../book/README.md#chapters) if its case changed, and set its `Synced against` line (add it under the opening paragraph if absent):

```markdown
Synced against upstream book commit `<short hash>` on <YYYY-MM-DD>.
```

## Step 5 — Verify before claiming done

```bash
bash docs/book/check-notes.sh                         # links, anchors, symbols, cases, backlinks
git -C ~/personal/xv6-riscv-book status --porcelain   # expect no output
git status --short docs/book
grep -L '<svg' docs/book/fig/*.svg                    # expect no output
```

Open each new or replaced figure (or render it) and confirm it is cropped, sits on a white plate, and keeps its labels. Confirm no new hard line breaks inside paragraphs. **Do not** commit unless the user asks.

Report: the upstream commits pulled, each figure added, re-processed, rendered, or still redrawn (and why), each chapter note created or changed, every divergence found, and sibling gaps left for `write-lecture-note.md`.
