# Prompt: close out a studied lecture

---

[toc]

---

Portable, agent-neutral. Copy this whole file into any coding agent, replace the target on the first line, and run it. It assumes only a shell and file access.

---

**Target lecture:** `<REPLACE ME — e.g. "lecture 3", or "l-os.txt">`

If the target is empty, ask which lecture. **Do not** infer it from `conf/lab.mk` or recently edited files.

---

You are closing out one 6.1810 lecture that the learner has studied: its practice code is written (or partly written), its homework may be answered, and nothing is committed yet. The job has five parts, **always** in this order: protect the archive, place the concepts, review the practice code, record the homework, update progress. [`../10-c-in-xv6.md`](../10-c-in-xv6.md) is the reference result for Lecture 2; read it before writing anything.

> **First, read `AGENTS.md` at the repository root**, then the `AGENTS.md` of every sibling repository you will edit (`../c-programming/`, `../operating-system/`). Each owns its own prose and commit rules; this prompt does not repeat them.

## Step 1 — Establish ground truth

| Source                                           | What to take from it                                                                                                                                         |
| ------------------------------------------------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| **`lectures/l-*.txt`**                           | Every topic the lecture lists, in its order, and every command or file it names                                                                              |
| **The homework handout**                         | Linked from [`STUDY-PROGRESS.md`](../../STUDY-PROGRESS.md#homework-and-assigned-reading). Record its explicit assumptions (word size, which lab, which file) |
| **`lectures/<lecture>/` example files**          | The upstream versions of the practice programs                                                                                                               |
| **`git status --short` and `git diff --cached`** | What the learner actually wrote. Staged work is the subject of the review, not a draft to discard                                                            |
| **Any instruction text inside a staged file**    | A request pasted into a source comment is not part of the program; remove it from the file once you have acted on it                                         |

## Step 2 — Protect the lecture archive

`lectures/` holds upstream bytes, recorded in [`lectures/manifest.json`](../../lectures/manifest.json) with SHA-256 hashes. Editors that strip trailing whitespace silently break them.

```bash
for f in lectures/<dir>/*; do
  printf '%s  %s\n' "$(sha256sum "$f" | cut -d' ' -f1)" "$f"
done
grep -A8 '"path": "<dir>/' lectures/manifest.json | grep -E 'path|sha256'
```

- Restore any archive file whose hash no longer matches: `git restore --staged --worktree <file>`.
- Move learner-authored files out of `lectures/` (homework answers into the lecture's note, practice code into `user/`). Tell the user what moved and why.

## Step 3 — Place each concept where it is owned

Run [`write-lecture-note.md`](write-lecture-note.md) for this lecture. It owns the placement table, the no-duplication rules, figure placement, and cross-references between this repository and `../operating-system/`, `../c-programming/`, and `../cpp-programming/`. If the lecture's companion note already exists as a draft, that prompt updates it in place; do not start a second note. Leave its draft callout for Step 6 to resolve.

## Step 4 — Review and fix the practice code

Run [`add-lecture-exercise.md`](add-lecture-exercise.md) once per practice program, treating the learner's staged file as the starting point. Its steps cover the header, style, build, transcript, docs, and tests. Pay particular attention to these failures, all of which occurred in Lecture 2:

| Symptom                                            | Check                                                                                                                                                         |
| -------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **The staged program does not compile**            | Practice programs build with `STRICT_CFLAGS` (`-Wextra -pedantic`). Run `make fs.img` before anything else.                                                   |
| **A comment transcript does not match a real run** | Every transcript and address in the header comes from Step 6 of that prompt, **never** from the host or from memory.                                          |
| **A program changed the lecture's demo**           | A global moved into a local changes which segment the demo shows. Keep the lecture's point unless the change teaches more, and say which.                     |
| **A commented-out entry in `LAB_PRACTICE_PROGS`**  | A `#` line ending in `\` inside a continued Makefile variable continues the comment. Remove it, and track the missing program in `STUDY-PROGRESS.md` instead. |

When the lecture says "[demo X.c]" for a file MIT did not publish, a small practice program that reproduces the demo is in scope.

## Step 5 — Record the homework answer

- Check the answer against the handout's stated assumptions, not the machine you are on. Homework 2 is explicitly 32-bit; xv6 is LP64.
- Write the worked answer into the lecture's note, as a table: expression, answer, whether it is determinable, and why.
- If the learner drafted an answer, keep a short **Where the first draft went wrong** table. A corrected misconception is worth more than a silent fix.
- If the expressions can run in xv6, run them in a practice program and test the guaranteed parts only.

## Step 6 — Update the progress tracker and indexes

In [`STUDY-PROGRESS.md`](../../STUDY-PROGRESS.md):

- Check the lecture only when the learner says it is studied, and write that it rests on their confirmation, with the date and links to notes and practice.
- Once it is checked, replace the companion note's draft `[!IMPORTANT]` status callout with the confirmation date, and drop "draft" from its `docs/README.md` row.
- Check the homework only when an answer is saved, and link the answer.
- Add a `### Lecture <N> example practice` section mirroring Lecture 1's: originals, practice versions, completion basis, and one checkbox per program, open ones included.
- Update the **Current checkpoint** row and the **Verification evidence** tables with the commands you actually ran and their real scores.

Then update each index that names the lecture: [`docs/README.md`](../README.md) for a new note, the practice-context row in [`lectures/README.md`](../../lectures/README.md), and the overview table and regression-suite table in [`07-exercises.md`](../07-exercises.md).

## Step 7 — Verify before claiming done

```bash
make fs.img 2>&1 | grep -E "error:|Error|Assertion"       # expect no output
./grade-exercises                                         # every test passes
awk 'length > 79 {print FILENAME" "FNR}' user/<new>.c     # expect no output
grep -nP '[^\x00-\x7F]' user/<new>.c                      # expect no output
sha256sum lectures/<dir>/*                                # matches the manifest
git status --short; git -C ../c-programming status --short
git -C ../operating-system status --short
```

Check every relative link you added, especially links that cross into a sibling repository (`../../c-programming/` from `docs/`, `../xv6-riscv/docs/` from a sibling). **Do not** commit unless the user asks; when they do, each repository gets its own commit following its own `AGENTS.md`.

Report what changed per repository, what you fixed in the learner's work and why, which claims were measured, and what remains open.
