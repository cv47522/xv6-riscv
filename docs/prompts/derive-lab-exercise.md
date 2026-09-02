# Prompt: write a lab derivation note

Portable and agent-neutral. Supply one target and follow every step.

---

**Target exercise:** `<REPLACE ME - e.g. "util: find" or "syscall: trace">`

If the target is empty, ask for the lab and exercise. Do not infer it from `conf/lab.mk`; one lab contains several exercises.

---

Create `docs/labs/<lab>-<exercise>.md`. Teach enough to derive the exercise, then stop before solution code or pseudocode. Read `AGENTS.md`, `docs/labs/README.md`, and `docs/labs/util-sleep.md` first; they own the detailed style, scope, and no-solution rules.

## Step 1 - Establish ground truth

Read these sources before writing:

| Source                                                       | Extract                                                                                       |
| ------------------------------------------------------------ | --------------------------------------------------------------------------------------------- |
| **The matching 6.1810 handout**                              | Requirements, difficulty, and hints; paraphrase rather than copy                              |
| `grade-lab-<lab>`                                            | Every relevant test body, including positive assertions, negative assertions, and breakpoints |
| **The existing source named by the exercise**                | Actual identifiers, constants, control flow, and failure behavior                             |
| `../xv6-riscv-book/*.tex` and `docs/book/ch*.md`             | The mechanism and what already has an owner                                                   |
| `docs/02`, `03`, `05`, `06`, `07`, and `docs/book/README.md` | Material to link instead of repeat                                                            |

Use this only to enumerate test names and points, then inspect the matching Python bodies manually:

```bash
sed -n 's/^@test(\([0-9]*\), "\(.*\)").*/\1 pts  \2/p' grade-lab-<lab>
```

Verify every symbol and behavioral claim against this tree. Record meaningful upstream divergences, such as `pause` here where upstream uses `sleep`; do not rely on remembered xv6 names.

## Step 2 - Define ownership

Name the note `docs/labs/<lab>-<exercise>.md`, using lowercase handout terms. Link shared material to its owner:

| Topic                                        | Owner                             |
| -------------------------------------------- | --------------------------------- |
| **Program layout, `UPROGS`, and C style**    | `docs/02-build-boot-and-usage.md` |
| **Lab selection, grading, and transcripts**  | `docs/03-lab-workflow.md`         |
| **System-call signatures and failures**      | `docs/05-syscall-reference.md`    |
| **`ecall`, registers, stubs, and artifacts** | `docs/06-build-artifacts.md`      |
| **Kernel mechanisms**                        | The matching `docs/book/ch*.md`   |
| **Terminology**                              | `docs/04-terminology.md`          |

When an owning document is still a placeholder, say that the note holds the material temporarily.

## Step 3 - Write the note

Use applicable sections in this order:

1. **What the exercise asks for:** paraphrase the handout and list each relevant grader test, its points, its exact checks, and the limits of what those checks prove.
2. **Prerequisites:** add a compact map that separates material to read first from references to use when needed. Link to the owning xv6 note and, when the concept is already taught in a sibling study repository, link to that top-level note before linking individual example sources. State when a hosted-C example teaches a concept but is not an xv6 API template.
3. **Naming or setup divergences:** include only differences that would otherwise send readers to a missing symbol.
4. **The code to read first:** show which existing files contribute what and what each deliberately does not do.
5. **The key existing function:** quote one contiguous function verbatim, cite its source, and explain its consequential lines and control flow.
6. **The concept underneath:** use source-backed constants and distinguish observations from guarantees.
7. **Deriving it:** give a contract table, source-anchored decision questions, and a trap table. Do not prescribe the target program's ordered statements.
8. **Verifying it:** give the build, guest checks, focused grader command, and a gdb recipe only when kernel behavior is worth observing.
9. **Questions:** provide five to eight source-answerable questions, including at least two failure scenarios.

For every explanation that would otherwise begin with a dense paragraph, first choose the smallest scan layer that matches the information: a callout for one invariant, a table for repeated fields or alternatives, a numbered list for ordered operations, or a diagram for relationships and branching. Keep nuanced causal reasoning and every technical qualification in prose immediately below the scan layer; never force paragraph-sized explanations into table cells or Mermaid nodes merely to make the page more colorful.

Use at most two purposeful diagrams by default. If an analogy materially helps, state where it stops matching the implementation. Apply all formatting and visual rules from `AGENTS.md` without repeating them here. Before finishing, perform a scan audit: using only headings, emphasized terms, callouts, tables, lists, diagrams, and code blocks, a reader should be able to locate the contract, prerequisites, source ownership, state or control-flow branches, invariants, failure modes, and verification commands. Add or revise a visual aid only when that audit exposes a retrieval problem; do not decorate sections that already scan well.

## Step 4 - Register related changes

- Add the note to `docs/labs/README.md`.
- Update `docs/05-syscall-reference.md` if the exercise changes a system call.
- Do not change `docs/README.md` or `docs/03-lab-workflow.md` for each new note; they index the collection.

## Step 5 - Audit the no-solution boundary

Read the note as someone about to implement the exercise. Remove anything that can be copied as the answer, replace missing choices with source-anchored questions, and cut explanations already owned elsewhere.

Every fenced `c` block must be a contiguous, verbatim quotation from an existing non-target file and must cite that file. Inspect this manually. The following smoke check finds unmatched nonblank lines, but it cannot prove order, contiguity, or that the target solution was excluded:

````bash
awk '/^```c$/{f=1;next} /^```/{f=0} f && NF' docs/labs/<note>.md |
  sed 's/^[[:space:]]*//' | sort -u |
  while IFS= read -r line; do
    grep -rqF --include='*.c' --include='*.h' -e "$line" kernel user mkfs ||
      echo "NOT IN TREE: $line"
  done
````

## Step 6 - Verify

Run and report the actual output:

```bash
bash docs/book/check-notes.sh
sed -n 's/^@test(\([0-9]*\), "\(.*\)").*/\1 pts  \2/p' grade-lab-<lab>
git status --short
```

Manually compare every grader claim with the corresponding test body; the `sed` command validates only names and point values. Generate any quoted transcript by running its command rather than predicting its output.

Do not create nested `docs/labs/<lab>/` directories, restructure `docs/`, or add the solution after grading.
