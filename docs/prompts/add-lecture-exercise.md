# Prompt: add or upgrade a lecture exercise

Portable, agent-neutral. Copy this whole file into any coding agent, replace the target on the first line, and run it. It assumes only a shell and file access.

---

**Target exercise:** `<REPLACE ME — e.g. ex5forkexec, or "lecture 2, ex3">`

If the target is empty, ask which exercise, or offer to audit every `user/ex*.c` against this checklist and report which ones fail.

---

You are working in an xv6-riscv study repository. The exercises under `user/` are teaching artifacts, not production code: their job is to make one lecture idea inspectable, and the comment block is the primary deliverable. `user/ex1copy.c` is the reference implementation of the style — read it before writing anything.

> **First, read `AGENTS.md` at the repository root.** It owns the C source style, the source-comment formatting rules, and the visual-readability conventions this prompt depends on and does not repeat. Some agents load it automatically; if yours does not, read it explicitly.

## Step 1 — Establish ground truth before writing

Read all four sources. Do not write from memory of Unix; xv6 diverges from POSIX in ways that matter, and a plausible-sounding claim that is wrong for xv6 is worse than no comment.

| Source                                           | What to take from it                                                                       |
| ------------------------------------------------ | ------------------------------------------------------------------------------------------ |
| `docs/lectures/l-*.txt`                          | The exercise's number, the lecture's own wording for what it does, and the teaching points |
| `../xv6-riscv-book/*.tex` (`unix.tex` for ch. 1) | The long-form authority. Precise semantics, and the _why_ behind interface choices         |
| The kernel source the exercise touches           | The actual implementation. Quote real identifiers and real constants                       |
| `user/ex1copy.c`                                 | The house style you are matching                                                           |

**Verify every factual claim against the source tree.** Grep for each constant, function, and struct field you plan to name. `DIRSIZ`, `PIPESIZE`, `NOFILE`, `MAXARG`, `NPROC`, and `NDEV` all have specific values in `kernel/param.h`, `kernel/fs.h`, and `kernel/pipe.c` — cite them, do not guess. If you cannot find something you were about to assert, cut the assertion.

## Step 2 — Name the file

The rule is `ex<N><verb>.c`:

- `N` is the **lecture's** number for the example, not the order files were added. Lecture 1's ex5 is `fork` + `exec` + `wait`, so it is `ex5forkexec.c` even if it was written third.
- The verb comes from the lecture's own phrasing ("copy input to output" → `copy`; "redirect the output of a command" → `redirect`).
- **The guest command name must be at most 14 bytes.** `DIRSIZ` in `kernel/fs.h` is 14, and `mkfs/mkfs.c` asserts on it — a longer name aborts the image build. Count before committing to a name.
- Avoid words xv6 does not have. `stdin`/`stdout` name C `FILE *` streams that do not exist here, and the repo's own docs warn against them, so a filename built from them contradicts the house rules.

Renaming an existing file: use `git mv`, then update `Makefile` `UPROGS`, `docs/07-exercises.md`, `docs/02-build-boot-and-usage.md`, any `grade-*` script, and the `fprintf`/`printf` message prefixes inside the source itself. Grep for the old name afterwards and expect zero hits outside `docs/superpowers/`.

## Step 3 — Write the comment block

Match `user/ex1copy.c`'s shape. Sections are `// ---` banner rules with an ALL-CAPS title; prose is two-space indented under `//`. Include, in this order, whichever apply:

1. **One-line header** — `// ex5forkexec.c: fork() a child, exec() a program in it, wait() for it.`
2. **Provenance** — the lecture it comes from, the upstream URL, and the host `man` page if one exists.
3. **`RUNNING IT`** — literal transcripts, with the `%` host prompt and `$` guest prompt distinguished. Include the _interesting_ invocations, not just the bare one.
4. **A diagram** — ASCII, showing the data path or the before/after state. This is usually the highest-value part of the file.
5. **The interface table** — arguments, meanings, rules; then return values and what the program does with each.
6. **`WHERE EACH PIECE LIVES IN THIS TREE`** — a two-column map from idea to file, ending with `docs/07-exercises.md` and the relevant `docs/book/ch*.md`.
7. **`THREE THINGS WORTH KEEPING`** — the durable takeaways, then a `Still open:` line pointing at the next exercise.

Inline comments carry the rest. Put the explanation **next to the line it explains**; the top block is for what the whole program is about.

### Constraints that are not style preferences

- **79 columns maximum**, including the `//`. Verify mechanically, never by eye:

    ```bash
    awk 'length > 79 {print FILENAME" "FNR": "length}' user/ex*.c
    ```

- **ASCII only.** No box-drawing characters, no typographic quotes, no em dashes. Use `--` and `->`. The comment has to stay readable in `objdump -S` output and in gdb's `layout src`.
- **Draw diagrams top-to-bottom, not left-to-right.** A vertical flow fits the column budget and matches the order the statements below it execute. A side-by-side before/after is the most common way to blow past 79 columns.
- **Never end a `//` line with a backslash.** It continues the comment onto the next line and `-Werror=comment` fails the build. This bites ASCII diagrams that use `\` for a diagonal — redraw with `+`, `|`, and `-`.
- **Answer the questions the code actually raises.** If a line looks arbitrary — `wait((int *)0)`, a trailing `0` in an argv array, a magic `6` — explain it where it sits. These are the highest-value comments in the file.
- **Prefer the mechanism over the summary.** "`fdalloc()` scans `p->ofile[]` from 0 and returns the first empty slot" beats "the kernel picks a free descriptor".

## Step 4 — Apply best practices to the code

The lecture's originals ignore errors and say so: _"these examples ignore errors — don't be this sloppy!"_ Fix that, and comment on what the sloppy version would have done.

- Check every system call that can fail. State what the failure looks like _from the user's side_ when unchecked — usually a symptom surfacing in a later, unrelated command.
- `close()` descriptors even where `exit()` would. For pipes, closing unused ends is **protocol** — EOF is defined as the last write end closing — so say that rather than calling it tidiness.
- `wait()` for children, unless the exercise's whole point is to show unordered output. If it is, say so explicitly so it does not read as an omission.
- Follow the C source style in `AGENTS.md`: return type on its own line, `main(void)`, brace on the next line, no `<stdlib.h>`, no `EXIT_SUCCESS`, no `FILE *`.
- **Never run `make fmt`.** It reformats the whole tree and turns the next lab merge into a conflict on nearly every file. Match style by hand.

## Step 5 — Register and build

Append to `UPROGS` in the `Makefile`, tab-indented, **with a trailing backslash on the last entry** (the blank line after it terminates the variable):

```makefile
	$U/_ex5forkexec\
```

Then:

```bash
make fs.img 2>&1 | grep -E "error:|Error|Assertion"
```

A `mkfs` assertion on `index(shortname, '/') == 0` means a nested path; an assertion on name length means step 2 was skipped.

Match `error:` and `Error`, not a bare case-insensitive `error` — every compile line contains `-Werror`, so the loose pattern reports a failure on every successful build.

## Step 6 — Run it, and check the transcript against your own comments

**This step is not optional.** Boot xv6 and run the exercise. Every transcript in your comment block must match what actually happens — a documented output that differs from the real one is worse than none.

```bash
cat > /tmp/run-ex.py <<'PY'
import sys; sys.path.insert(0, '.')
from gradelib import *
r = Runner(save("xv6.out.exercises"))
@test(1, "exercises")
def t():
    r.run_qemu(shell_script(["ex5forkexec", "echo ALLDONE"]), timeout=90)
run_tests()
PY
python3 /tmp/run-ex.py && sed -n '/ex5forkexec/,/ALLDONE/p' xv6.out.exercises
```

If QEMU fails to start, run `pkill -f qemu-system-riscv64` and retry — a stale instance holds the GDB port.

Then fix the comments to match reality. Real behaviour is frequently _more_ interesting than the prediction: `ex3fork` turned out to interleave mid-word rather than mid-line, because `printf` is not atomic and `uartputc` locks per character — which became the best thing in that file. Look for the surprise and write it up.

## Step 7 — Update the docs

- **`docs/07-exercises.md`** — add a row to the overview table, a node to the Mermaid dependency graph, and a section. Section shape: transcript, then what the lecture uses it to teach, then the questions the source raises. The source comments carry the long form, so the doc must be tighter and must not duplicate them.
- **`docs/02-build-boot-and-usage.md`** — only if the shared mechanics changed. Ordinary exercises need nothing here.
- **`docs/05-syscall-reference.md`** — if this is the first exercise to use a call, check the entry is accurate.
- Markdown rules: **never hard-wrap prose** (one continuous line per paragraph), bold the first column in comparison tables, use GitHub callouts (`> [!NOTE]`, `> [!WARNING]`) where they earn the emphasis, and follow the documented Mermaid band colors.

Optionally add a `grade-<name>` script beside `grade-ex1copy` if the behaviour is worth pinning — one test per claim, driving QEMU through `gradelib.py`.

## Step 8 — Verify before claiming done

Run these and report the actual output. Do not assert success without them.

```bash
make fs.img 2>&1 | grep -E "error:|Error|Assertion"   # expect no output
./grade-ex1copy                                    # existing suite must still pass
awk 'length > 79 {print FILENAME" "FNR}' user/ex*.c   # expect no output
grep -rn '<old-name>' --include='*.md' --include='Makefile' --include='grade-*' .
git status --short
```

After a rename, delete the stale build products (`user/_<oldname>`, `user/<oldname>.o|.d|.asm|.sym`). They are gitignored but survive `make` and confuse the next `ls`.

## Scope note

Do not reorganize `user/` into subdirectories. Four mechanisms assume the flat layout and three of them fail _silently_; `docs/02-build-boot-and-usage.md` has the analysis under "Why sources and build products stay flat". Scalability here comes from the naming convention and the doc structure, not from directories.
