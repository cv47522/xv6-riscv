# Reusable Prompts

---

[toc]

---

Task prompts for repeated work in this repository, written to be **agent-neutral**: plain Markdown, no tool-specific front matter, no vendor syntax. Copy one whole file into whatever agent you are using, fill in the target on the first line, and run it.

| Prompt                                                 | Use it when                                                                                               |
| ------------------------------------------------------ | --------------------------------------------------------------------------------------------------------- |
| **[add-lecture-exercise.md](add-lecture-exercise.md)** | Adding a new `user/ex*.c` from a lecture example, or bringing an existing one up to the teaching standard |

## Why one file per prompt

Select-all and paste is the whole interaction. A single combined `Prompts.md` would force you to select a region instead, and it gets worse with every prompt added — so each prompt is a standalone file, and this README is the index.

## Using them

**Claude Code** — already wired up. `.claude/commands/exercise.md` is a thin stub that points here, so `/exercise ex5forkexec` runs the prompt with its arguments. Adding a prompt means adding a matching stub if you want a slash command for it.

**Codex** — reads `AGENTS.md` at the repository root automatically, which is where the C style and comment-formatting rules live. Paste the prompt body on top of that.

**Cursor** — paste into the chat, or save a copy under `.cursor/rules/` if you want it always loaded. Cursor does not read `AGENTS.md` on its own, and every prompt here opens by telling the agent to read it, so keep that line.

**Anything else** — the prompts assume only a shell and file access. Nothing depends on a particular tool's API.

## Writing a new one

Follow the shape of the existing prompt:

- **Open with the target**, on its own line, marked as the thing to replace. An agent that receives no target should ask rather than guess.
- **Point at `AGENTS.md` instead of restating it.** Duplicated style rules drift apart, and `AGENTS.md` is the file both humans and Codex already read.
- **Number the steps**, and make ground truth step 1. Every prompt here starts by reading the authoritative sources, because the failure mode in this repository is a fluent, plausible, wrong claim about xv6.
- **Give verification commands, not verification advice.** `awk 'length > 79'` is checkable; "keep lines short" is not. End with a step that runs them and reports real output.
- **Say what not to do**, with the reason. The scope note at the end of `add-lecture-exercise.md` exists because "reorganize into subdirectories" is a plausible-sounding change that breaks three things silently.

> [!NOTE]
> These are prompts, not documentation. They tell an agent how to _do_ a task; [07-exercises.md](../07-exercises.md) and the rest of `docs/` describe what the results _are_. When a prompt and a document overlap, the document is the reference and the prompt should link to it.
