# Review contract

Any agent asked to review work in this repository follows this file. It works with any tool: point the agent at this file and name the files or diff to review.

## Scope

1. Review the **unstaged** diff by default (`git diff --word-diff`). The author's version is staged, so the unstaged diff is exactly what changed since then. Review staged or committed changes only when asked.
2. Load only the rule files that match the changed files:

| Changed files                          | Rules                        |
| -------------------------------------- | ---------------------------- |
| **Markdown** (`*.md`)                  | [prose.md](prose.md)         |
| **Assembly** (`*.s`, `*.S`, `asm` fences) | [assembly.md](assembly.md) |
| **Images** (`docs/images/**`)          | [images.md](images.md)       |
| **Mermaid or Excalidraw**              | [diagrams.md](diagrams.md)   |

3. Run `make lint` first. Report its output as is and **do not** repeat lint findings by hand; spend the review on what a script cannot judge.
4. Read each rule file's `## Decisions` section. **Never** propose a change that reverses a recorded decision.

## Output

**Do not** edit files during a review. Output one table, ordered by severity:

| ID | Location | Kind | Original | Proposed | Why |
| -- | -------- | ---- | -------- | -------- | --- |
| 1  | `docs/arm/04-….md:42` | `error` | exact current text | exact replacement | one line |

- **Location:** `path:line`, so the finding is clickable.
- **Kind:**
  - `error`: factually or technically wrong, or breaks a build, link, or example.
  - `rule`: violates a named rule; cite it, e.g. `prose.md › Commas`.
  - `preference`: reviewer taste with no rule behind it. Keep these few and clearly marked.
- **Original / Proposed:** quote the smallest span that carries the change, a phrase or one sentence. **Never** rewrite a whole paragraph to fix one phrase. Leave `Proposed` empty when only the author can decide, and say what is needed in `Why`.
- One finding is one independent change, so each can be accepted or rejected on its own.

End with a one-line summary: counts by kind and anything you could not check (for example, an image you could not open).

## Applying a review

The user answers with a selection such as `accept 1,3,5; 4 but keep "foo"; reject rest`. The applying agent:

1. Applies **only** the accepted findings, exactly as proposed or as amended, with the smallest possible edit, and **does not** reflow surrounding text.
2. Leaves everything else byte-for-byte unchanged and stages nothing.
3. Runs `make lint` again.
4. For each rejected `rule` or `preference` finding, proposes a one-line `## Decisions` entry for the matching rule file, recording what the user kept and why if they gave a reason. Write the entry only after the user confirms it.
