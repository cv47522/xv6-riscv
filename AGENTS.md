# AGENTS.md

## Documentation Visual Readability

- Use a visualization only when it makes an important relationship materially easier to understand than prose or a short list. Do not add one merely because a section is long.
- Preserve every technical detail when restructuring documentation. A visual may reorganize or front-load information, but it must not silently simplify away invariants, failure behavior, exceptions, values, or source references.
- Layer complex explanations when necessary: lead with a compact visual overview, then retain the detailed explanation immediately below it.
- Keep nuanced causal reasoning in prose. Do not cram paragraph-sized explanations into Mermaid nodes or oversized table cells.
- Prefer the smallest visual that matches the information shape:

| Information shape                                                      | Preferred presentation                |
| ---------------------------------------------------------------------- | ------------------------------------- |
| **Control flow, lifecycle, or state transition**                       | Mermaid flowchart or sequence diagram |
| **Pointers, ownership, sharing, or reference counts**                  | Object graph                          |
| **Conditions, branches, and outcomes**                                 | Decision or state table               |
| **Ordered operations without meaningful branching**                    | Numbered list                         |
| **Repeated fields, alternatives, naming, or architecture differences** | Comparison table                      |
| **Hierarchy, nesting, or parent/child relationships**                  | Tree or flowchart                     |
| **Critical invariant or deliberate exception**                         | GitHub-style callout                  |

- Use callouts deliberately: `NOTE` for context or invariants, `TIP` for practical workflow advice, `IMPORTANT` for decisions readers must preserve, `WARNING` for likely mistakes, and `CAUTION` for destructive or difficult-to-recover consequences. Do not use callouts as decoration or as a substitute for ordinary exposition.
- In tables that compare or summarize named items, bold the first-column value unless it is a raw identifier, command, path, URL, or value readers need to copy exactly.
- In Mermaid diagrams, label every phase and outcome so meaning survives monochrome rendering. Use color consistently, and never make color the only carrier of meaning.
- Use the repository's light-theme Mermaid palette consistently: request `#fff0f0`, decision or routing `#f0f0ff`, data movement `#f0fff0`, processing `#fffff0`, commit `#f0ffff`, and stall or failure `#ffd9d9`.
- Keep diagrams focused. If a diagram needs dense prose, many cross-links, or more than a quick glance to decode, split it or return the detail to prose.
- Preserve existing effective comprehension aids. Tables, examples, summaries, and callouts are not redundant when they help readers orient, compare, or retain information.

## C Source Style

- Match `.clang-format`, which is what `make fmt` enforces: the return type of a top-level function goes on its own line (`BreakAfterReturnType: TopLevelDefinitions`) and its opening brace on the next (`BraceWrapping.AfterFunction: true`). Writing `int main(void) {` is not a style choice here; it is a change clang-format reverses.
- Never run `make fmt` to fix one file. It reformats every source under `kernel/`, `user/`, and `mkfs/`, and a whole-tree reformat makes the next lab merge conflict on nearly every file.
- There is no host C library. The Makefile compiles with `-ffreestanding -nostdlib`, and `user/user.h` is the complete set of functions a user program can call, so `<stdlib.h>`, `EXIT_SUCCESS`, and `FILE *` streams do not exist.
- `return 0` and `exit(0)` are equivalent in user programs: `start()` in `user/ulib.c` does `exit(main(argc, argv))`. Prefer `exit()` on error paths, where its `__attribute__((noreturn))` declaration carries information, and match the surrounding file otherwise.
- Keep new user programs and their build products directly under `user/`. Nested directories break `mkfs` imports, the `-include user/*.d` dependency glob, and `clean`'s single-level wildcards — see [docs/02-build-boot-and-usage.md](docs/02-build-boot-and-usage.md).
- Do not restructure the build for tidiness. The Makefile is merged from the lab branches, so layout changes convert every future lab merge into a conflict on it.

## Source Comment Formatting

The documentation visual-readability rules above apply inside `.c` and `.h` comments too. C has no Markdown renderer, so the same information shapes are built out of ASCII. `.clang-format` sets `ReflowComments: Never`, which means hand alignment survives `make fmt` — and equally, that nothing will fix an over-long line for you.

- Choose the smallest structure that matches the information shape, exactly as in Markdown: an aligned table for repeated fields or per-argument rules, a numbered or bulleted list for ordered steps, a top-to-bottom ASCII diagram for a data path, and prose only for the two or three points that genuinely need causal reasoning.
- Give a long teaching header banner rules and an ALL-CAPS section title, matching the style already used in the [Makefile](Makefile):

    ```c
    // ---------------------------------------------------------------------------
    //  SECTION TITLE
    // ---------------------------------------------------------------------------
    ```

- Build tables from column-aligned text with a `----` rule under the header. Two leading spaces inside the comment body sets them off from the surrounding prose:

    ```c
    //   Return    Means                       What this program does
    //   --------  --------------------------  ---------------------------------
    //    > 0      bytes actually transferred  copy exactly that many, not 64
    //      0      EOF: no more will arrive    leave the loop and return 0
    //     -1      the call failed             report on fd 2 and return 1
    ```

- Draw diagrams top-to-bottom rather than left-to-right. A vertical flow fits the column budget, and it matches the order the statements below it execute.
- Keep every line at 79 columns or fewer, including the `//`. Verify with `awk 'length > 79 {print FNR": "length}' <file>` rather than by eye.
- Use ASCII only — no box-drawing characters or other Unicode. The same comment has to stay readable in `objdump -S` output, in gdb's `layout src`, and in a terminal without UTF-8.
- Anchor claims to a file, the way the notes do: write `kernel/sysfile.c` or `p->ofile[NOFILE] in kernel/proc.h`, never "the kernel does this somewhere".
- Split the labour between the banner comment and `docs/`. The header is the quick reference someone reads with the code open; the note under `docs/` is the long form. Cross-link by path instead of duplicating text, so the two cannot drift.
- Keep comments inside a function body short and about the _why_. The banner block carries the teaching; a body comment explains one decision the code cannot state itself.

## Book Notes

- Apply the visual-readability rules to `docs/book/*.md` while preserving the ownership model in `docs/book/README.md`: OSTEP notes own portable concepts, and xv6 notes own this tree's implementation.
- Keep each chapter's standard sections unless the chapter has no relevant content for one: `What xv6 actually does`, `Divergences`, `Code walked through`, `Questions I had`, and `Lab connection`.
- Use one or two purposeful visuals per substantive chapter by default. Add more only when the chapter contains additional relationships that cannot be scanned effectively in prose.
- Preserve detailed source-symbol explanations near their visual overview. A chapter diagram is an entry point into the explanation, not a replacement for implementation evidence.
- Leave placeholder chapters structurally minimal until substantive content exists; do not decorate empty scaffolding.

## Lab Notes

`docs/labs/<lab>-<exercise>.md` holds one note per 6.1810 lab exercise that needs deriving rather than transcribing. `docs/prompts/derive-lab-exercise.md` is the procedure; the rules below are what a note has to satisfy however it was produced.

- **Never write the solution, in any form, including after the exercise is graded.** Every fenced `c` block in a lab note must be a contiguous, verbatim quotation from an existing non-target source and must cite that source. The exercise's target file gets a contract table, a decision checklist, and a trap table instead. The check below catches unmatched lines, but it cannot prove contiguity or exclude a completed solution, so inspect every block manually too:

    ````bash
    awk '/^```c$/{f=1;next} /^```/{f=0} f && NF' docs/labs/<note>.md |
      sed 's/^[[:space:]]*//' | sort -u |
      while IFS= read -r line; do
        grep -rqF --include='*.c' --include='*.h' -e "$line" kernel user mkfs ||
          echo "NOT IN TREE: $line"
      done
    ````

- Treat `grade-lab-<lab>` as the exercise's executable specification. Read each relevant test body and record its positive assertions, negative assertions, and breakpoints without overstating what they prove.
- **Link instead of restating.** `docs/02` owns program mechanics and C style, `docs/03` owns lab and grading mechanics, `docs/05` owns system-call semantics, `docs/06` owns `ecall`, `a7`, and the stub, and `docs/book/ch*.md` owns kernel mechanisms. A lab note owns only the one exercise it is named after. Where the owning document is still a placeholder, hold the material on loan and say so, the way `docs/book/` does.
- Keep the section order: what the exercise asks, the code to read first, a walkthrough of the function it turns on, the concept underneath, the derivation, verification, then questions. Add an analogy only when it materially helps, and name where it breaks down.
- Two diagrams is the working budget — one for which file contributes what, one for the control flow being explained — and both use the repository Mermaid palette with the legend stated once beneath the first.
- Verify names against this tree before citing them. The clock-tick system call is `pause`, not `sleep`, and `kernel/printf.c` is `kernel/printk.c`; a note that sends the reader grepping for an upstream symbol has failed at its one job.
- Register every new note in `docs/labs/README.md`, and run `docs/book/check-notes.sh` before claiming it is done.
