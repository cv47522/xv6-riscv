# Lab Notes

---

[toc]

---

One note per lab exercise that needed more thinking than typing: what the exercise actually asks for, which existing code you have to understand before writing anything, and the decisions the implementation comes down to.

| Note                                                             | Lab    | Exercise                                                                                                     |
| ---------------------------------------------------------------- | ------ | ------------------------------------------------------------------------------------------------------------ |
| **[01-util-sleep.md](01-util-sleep.md)**                         | `util` | `sleep` (easy) — the first system call, and the foundation for later labs                                    |
| **[02-util-sixfive.md](02-util-sixfive.md)**                     | `util` | `sixfive` (moderate) — streaming decimal tokens across exact file boundaries                                 |
| **[03-util-memdump.md](03-util-memdump.md)**                     | `util` | `memdump` (moderate) — typed values, byte views, pointers, and structure layout                              |
| **[04-util-find.md](04-util-find.md)**                           | `util` | `find` — directory records, pathnames, and recursive search                                                  |
| **[05-util-find-exec.md](05-util-find-exec.md)**                 | `util` | `exec` — running a command for each matched path with fork/exec/wait                                         |
| **[06-util-uptime.md](06-util-uptime.md)**                       | `util` | `uptime` (optional, easy) — reading the tick counter through one system call                                 |
| **[07-util-find-regex.md](07-util-find-regex.md)**               | `util` | `find` regular expressions (optional, easy) — reusing grep's matcher and choosing anchoring                  |
| **[08-util-sh-prompt.md](08-util-sh-prompt.md)**                 | `util` | shell: no `$ ` from a file (optional, moderate) — telling a person from a script; owns shared shell material |
| **[09-util-sh-wait.md](09-util-sh-wait.md)**                     | `util` | shell: `wait` (optional, easy) — why `&` orphans its job, and who may wait for it                            |
| **[10-util-sh-tab-completion.md](10-util-sh-tab-completion.md)** | `util` | shell: tab completion (optional, easy) — the console line discipline that hides Tab                          |
| **[11-util-sh-history.md](11-util-sh-history.md)**               | `util` | shell: history (optional, moderate) — what one command line is, and which process owns it                    |

## What belongs here, and what does not

The mechanics shared by every lab — selecting a lab in `conf/lab.mk`, merging starter code, running `make grade`, reading a failed transcript — belong to [03-lab-workflow.md](../03-lab-workflow.md) and are not repeated in a lab note. The mechanics shared by every user program — where the source goes, the `UPROGS` entry, the house C style — belong to [02-build-boot-and-usage.md](../02-build-boot-and-usage.md#adding-and-running-a-user-exercise). A note here covers only the one exercise it is named after.

The split against the other collections:

| Collection                                   | Owns                                                                                      |
| -------------------------------------------- | ----------------------------------------------------------------------------------------- |
| **[book/](../book/)**                        | The xv6 book, chapter by chapter — the kernel mechanisms themselves.                      |
| **[../07-exercises.md](../07-exercises.md)** | The lecture example programs under `user/`, which are read rather than derived.           |
| **labs/** (this directory)                   | MIT lab exercises, from the handout's requirements to a checklist you can implement from. |

> [!IMPORTANT]
> **No solutions.** A lab note explains code that already exists in the tree and states the contract the new file has to satisfy; it stops at the point where writing the program begins. That boundary is deliberate — see the collaboration habit in [03-lab-workflow.md](../03-lab-workflow.md#how-to-approach-a-lab). Once an exercise passes its grader tests, the note may end with one `## Solution walkthrough` section behind a `[!CAUTION]` spoiler callout, explaining the finished source under `user/` with diagrams; every section before it stays a derivation aid. See [Lab Notes in AGENTS.md](../../AGENTS.md#lab-notes).

## Writing the next one

Follow the shape of [01-util-sleep.md](01-util-sleep.md):

- **Open with one contract**, distinguishing handout requirements from repository conventions, then document what each grader test actually asserts. A passing score may leave part of the contract unchecked; [the sleep note](01-util-sleep.md#what-the-grader-does-not-prove) shows how to record that boundary.
- **Map prerequisites before the walkthrough.** Separate material to read first from references to consult when needed, and link an owning concept note before individual example files.
- **Explain only the existing code the exercise forces you to read.** Link out for anything another document already owns; a second explanation of `ecall` is a second thing to keep correct. For the trap path, trapframe, register roles, or harts, link to [the foundation section of 01-util-sleep.md](01-util-sleep.md#the-foundation-a-c-call-that-becomes-a-trap), which holds that material on loan until [ch04](../book/ch04-traps-and-system-calls.md) is written.
- **Layer dense explanations for scanning.** Lead with the smallest useful callout, table, list, or diagram, then retain nuanced causal detail below it; visual structure must improve retrieval rather than decorate the page.
- **Choose figures by what they teach.** Follow [Lab figure formats](../../AGENTS.md#lab-figure-formats): Mermaid for logical relationships and flow; Excalidraw when manual spatial layout carries meaning. Preserve an effective existing figure rather than converting it for uniformity.
- **End with decisions, not code** — link back to the contract, give a numbered checklist where each item names the file that answers it, then a trap table and verification commands. Keep optional grader internals beside verification.

Use [derive-lab-exercise.md](../prompts/derive-lab-exercise.md) for the writing procedure and [AGENTS.md](../../AGENTS.md#lab-notes) for the authoritative rules. When revising an existing note, keep its technical evidence and incoming links while consolidating repeated explanations; the sleep note's extra concept loans are not a requirement to repeat that background in every lab.
