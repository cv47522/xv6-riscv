# Reusable Prompts

---

[toc]

---

Task prompts for repeated work in this repository, written to be **agent-neutral**: plain Markdown, no tool-specific front matter, no vendor syntax. Copy one whole file into whatever agent you are using, fill in the target on the first line, and run it.

| Prompt                                                 | Use it when                                                                                                                                                                                          |
| ------------------------------------------------------ | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **[add-lecture-exercise.md](add-lecture-exercise.md)** | Adding a new `user/ex*.c` from a lecture example, or bringing an existing one up to the teaching standard                                                                                            |
| **[derive-lab-exercise.md](derive-lab-exercise.md)**   | Writing the `docs/labs/` note for a 6.1810 lab exercise: what to read, what to link, and where to stop                                                                                               |
| **[close-lecture.md](close-lecture.md)**               | Closing out a studied lecture: protect the archive, place its concepts across this repo and the sibling notes, review the practice code and homework, and update `STUDY-PROGRESS.md`                 |
| **[write-lecture-note.md](write-lecture-note.md)**     | Drafting or updating a lecture's `docs/<NN>-*.md` companion note from `lectures/`, `docs/book/`, and this tree, keeping generic material and figures in the sibling notes with cross-links both ways |
| **[sync-book-chapter.md](sync-book-chapter.md)**       | Fast-forwarding the read-only `~/personal/xv6-riscv-book` checkout from upstream, saving every figure a chapter uses into `docs/book/fig/`, and writing or updating its `docs/book/ch*.md` note      |

## Why one file per prompt

Select-all and paste is the whole interaction. A single combined `Prompts.md` would force you to select a region instead, and it gets worse with every prompt added — so each prompt is a standalone file, and this README is the index.

## Using them

Every prompt is a plain Markdown file that assumes only a shell and file access, so **copy the whole file into any agent's chat** and it works. The per-agent wiring below only saves you that copy-paste; it **never** changes what the prompt says.

[setup-agent-commands.sh](setup-agent-commands.sh) generates all of it from the files in this directory:

```bash
docs/prompts/setup-agent-commands.sh            # repo-local mirrors
docs/prompts/setup-agent-commands.sh --codex    # plus the user-level Codex prompts
docs/prompts/setup-agent-commands.sh --check    # verify nothing has drifted, write nothing
docs/prompts/setup-agent-commands.sh --check --codex  # include installed Codex prompts
```

| Agent             | How the rules load                                                        | Command location                   | Committed?        |
| ----------------- | ------------------------------------------------------------------------- | ---------------------------------- | ----------------- |
| **Claude Code**   | Each generated command points to `AGENTS.md` through its canonical prompt | `.claude/commands/`                | Yes               |
| **Cursor**        | The always-on rule below points to `AGENTS.md`                            | `.cursor/commands/`                | Yes               |
| **Codex**         | Loads `AGENTS.md` automatically                                           | `$CODEX_HOME/prompts/`, user-level | No; run `--codex` |
| **Anything else** | Read `AGENTS.md`, then paste the prompt                                   | None                               | No                |

So `/exercise ex5forkexec`, `/lab util: find`, `/lecture lecture 3`, `/lecture-note next`, and `/book next` work in Claude Code and Cursor after a clone. Codex custom prompts use `/prompts:exercise ex5forkexec` and `/prompts:lab util: find`; after running `--codex`, restart the CLI session or open a new chat so Codex reloads them.

### Example invocations

Type the command and its target as one chat message. Each one reads its canonical prompt, so the target is the only thing you supply.

| Command         | Example                    | What it does                                                                                                                     |
| --------------- | -------------------------- | -------------------------------------------------------------------------------------------------------------------------------- |
| `/book`         | `/book next`               | Syncs the book chapter assigned to the first open lecture in `STUDY-PROGRESS.md`: pulls upstream, saves figures, writes the note |
| `/book`         | `/book ch3`                | Syncs one chapter's note and figures, whatever the tracker says                                                                  |
| `/book`         | `/book 3-4`                | Syncs a range of chapters in one run                                                                                             |
| `/book`         | `/book all`                | Syncs every chapter the upstream book changed since the last recorded sync, plus every chapter still a scaffold                  |
| `/lecture-note` | `/lecture-note next`       | Drafts the companion note for the first unchecked lecture, marked as a draft                                                     |
| `/lecture-note` | `/lecture-note lecture 4`  | Drafts or updates Lecture 4's `docs/<NN>-*.md`, adding only missing generic material to the sibling repositories                 |
| `/lecture-note` | `/lecture-note l-vm.txt`   | Same, naming the lecture by its archived text instead of its number                                                              |
| `/exercise`     | `/exercise ex5forkexec`    | Adds or upgrades one practice program under `user/`, with its docs and tests                                                     |
| `/exercise`     | `/exercise lecture 2, ex3` | Same, naming the exercise by lecture and position                                                                                |
| `/lab`          | `/lab util: find`          | Writes the solution-free derivation note for one lab exercise under `docs/labs/`                                                 |
| `/lecture`      | `/lecture lecture 3`       | Closes a lecture you have studied: reviews the practice and homework, then checks it off in `STUDY-PROGRESS.md`                  |

A typical cycle for a new lecture runs these in order:

```text
/book next              # chapter note and figures for the assigned reading
/lecture-note next      # draft companion note built on that chapter
/exercise lecture 4, ex1
                        # study the lecture, write the practice, answer the homework
/lecture lecture 4      # close it out once you have studied it
```

> [!TIP]
> Add a sentence after the target to narrow a run, for example `/book ch7 — figures only, leave the note alone` or `/lecture-note lecture 4 — do not edit ../operating-system yet`. The prompt still runs its verification step.

In Codex, prefix the same targets with `prompts:`, for example `/prompts:book next`. In any other agent, paste the prompt file and replace its **Target** line with the same text.

> [!NOTE]
> Codex custom prompts are deprecated in favor of skills, but the user-level mirrors remain useful for matching the repository's Claude and Cursor commands while Codex still supports them.

**Cursor needs one extra thing.** It does not read `AGENTS.md` on its own, so [`.cursor/rules/xv6-house-rules.mdc`](../../.cursor/rules/xv6-house-rules.mdc) is a hand-written always-on rule that points at it and repeats only the constraints that get violated most often. It is the one file here that is **not** generated — edit it directly when the house rules change.

> [!IMPORTANT]
> The generated stubs are pointers, never copies. The alias table in `setup-agent-commands.sh` maps command names to canonical prompt files. Update that table when adding or renaming a command, then rerun the script; it updates current mirrors and removes obsolete generated mirrors. `--check` reports missing, stale, and orphaned mirrors without writing.

## Writing a new one

Follow the shape of the existing prompt:

- **Open with the target**, on its own line, marked as the thing to replace. An agent that receives no target should ask rather than guess.
- **Point at `AGENTS.md` instead of restating it.** Duplicated style rules drift apart, and `AGENTS.md` is the file both humans and Codex already read.
- **Number the steps**, and make ground truth step 1. Every prompt here starts by reading the authoritative sources, because the failure mode in this repository is a fluent, plausible, wrong claim about xv6.
- **Give verification commands, not verification advice.** `awk 'length > 79'` is checkable; "keep lines short" is not. End with a step that runs them and reports real output.
- **Say what not to do**, with the reason. The scope note at the end of `add-lecture-exercise.md` exists because "reorganize into subdirectories" is a plausible-sounding change that breaks three things silently.

> [!NOTE]
> These are prompts, not documentation. They tell an agent how to _do_ a task; [07-exercises.md](../07-exercises.md) and the rest of `docs/` describe what the results _are_. When a prompt and a document overlap, the document is the reference and the prompt should link to it.
