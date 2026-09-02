# Lab Notes

---

[toc]

---

One note per lab exercise that needed more thinking than typing: what the exercise actually asks for, which existing code you have to understand before writing anything, and the decisions the implementation comes down to.

| Note                                   | Lab    | Exercise                                                                        |
| -------------------------------------- | ------ | ------------------------------------------------------------------------------- |
| **[util-sleep.md](util-sleep.md)**     | `util` | `sleep` (easy) — the first system call a program of your own ever makes         |
| **[util-sixfive.md](util-sixfive.md)** | `util` | `sixfive` (moderate) — streaming decimal tokens across exact file boundaries    |
| **[util-memdump.md](util-memdump.md)** | `util` | `memdump` (moderate) — typed values, byte views, pointers, and structure layout |

## What belongs here, and what does not

The mechanics shared by every lab — selecting a lab in `conf/lab.mk`, merging starter code, running `make grade`, reading a failed transcript — belong to [03-lab-workflow.md](../03-lab-workflow.md) and are not repeated in a lab note. The mechanics shared by every user program — where the source goes, the `UPROGS` entry, the house C style — belong to [02-build-boot-and-usage.md](../02-build-boot-and-usage.md#adding-and-running-a-user-exercise). A note here covers only the one exercise it is named after.

The split against the other collections:

| Collection                                   | Owns                                                                                      |
| -------------------------------------------- | ----------------------------------------------------------------------------------------- |
| **[book/](../book/)**                        | The xv6 book, chapter by chapter — the kernel mechanisms themselves.                      |
| **[../07-exercises.md](../07-exercises.md)** | The lecture example programs under `user/`, which are read rather than derived.           |
| **labs/** (this directory)                   | MIT lab exercises, from the handout's requirements to a checklist you can implement from. |

> [!IMPORTANT]
> **No solutions.** A lab note explains code that already exists in the tree and states the contract the new file has to satisfy; it stops at the point where writing the program begins. That boundary is deliberate — see the collaboration habit in [03-lab-workflow.md](../03-lab-workflow.md#how-to-approach-a-lab). Once an exercise is written and graded, the finished source under `user/` is its own record, and the note stays a derivation aid rather than growing into a walkthrough of the answer.

## Writing the next one

Follow the shape of [util-sleep.md](util-sleep.md):

- **Open with what the handout asks**, paraphrased, plus the grader's tests for that exercise, since the tests are the operational definition of "done".
- **Map prerequisites before the walkthrough.** Separate material to read first from references to consult when needed, and link an owning concept note before individual example files.
- **Explain only the existing code the exercise forces you to read.** Link out for anything another document already owns; a second explanation of `ecall` is a second thing to keep correct.
- **Layer dense explanations for scanning.** Lead with the smallest useful callout, table, list, or diagram, then retain nuanced causal detail below it; visual structure must improve retrieval rather than decorate the page.
- **End with decisions, not code** — a numbered checklist where each item names the file that answers it, then a trap table, then verification commands.
