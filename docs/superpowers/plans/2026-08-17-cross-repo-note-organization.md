# Cross-Repo Note Organization Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build the cross-reference structure that lets all thirteen xv6 book chapters be noted without duplicating the OSTEP notes in `../operating-system/`.

**Architecture:** A concordance file maps each concept across six sources and assigns it one of three ownership cases (`linked`, `loan`, `xv6-only`). Thirteen chapter stubs and a corrected README carry that structure; Chapter 1 is written in full as the worked example. A shell script checks the structure mechanically, so link rot and stale symbol references fail loudly rather than silently.

**Tech Stack:** Markdown, POSIX shell, `grep`/`sed`/`find`. No build step, no dependencies.

**Spec:** `docs/superpowers/specs/2026-08-17-cross-repo-note-organization-design.md`

## Global Constraints

- **Never hard-wrap Markdown prose.** Each paragraph is one continuous line. Do not reflow paragraphs outside the lines being changed.
- **Restate, never transcribe.** No xv6 book text or OSTEP text is copied into a note.
- **Symbol names lead, line numbers follow.** Write `` `kernel/proc.c` `kfork()` ``, not `kernel/proc.c:259`, except where the reference is to a comment or a specific instruction.
- **All code paths in the concordance are repo-relative from the xv6-riscv root** (`kernel/proc.c`, `user/sh.c`), because `check-notes.sh` resolves them that way.
- **Cross-repo link depth:** `../../../operating-system/…` from `docs/book/` (two levels below root); `../xv6-riscv/…` from the OSTEP repository root (one level).
- **Name which xv6 you mean** whenever `xv6-public` and `xv6-riscv` could be confused.
- **Changes to `../operating-system/` are limited** to the backlink line and, where absent, the `## Related Notes` heading holding it. No existing prose there is edited, reordered, or reflowed.
- **Commit messages follow the repository convention:** `{type}({scope}): {subject}`, at least three body bullets each starting with a past-tense verb and separated by blank lines, body lines wrapped at 74 characters, then `Cursor(%CUR): Yes`, `Effort: N SP`, and `Co-authored-by:`.

> [!WARNING]
> `../operating-system/` has uncommitted work in progress: `Paging.md` is modified and three files under `codes/` are staged. Task 6 runs in that repository. **Stage only the files you edit, by explicit path.** Never run `git add -A`, `git add .`, or `git commit -a` there.

---

## File Structure

| File | Responsibility |
| ---- | -------------- |
| `docs/book/check-notes.sh` | Mechanical verification: link resolution, chapter table, symbol existence, status consistency |
| `docs/book/README.md` | Chapter checklist (corrected to thirteen), note-writing conventions, the three-case rule, link mechanics |
| `docs/book/00-ostep-concordance.md` | Concept-to-source map with ownership status, plus the tree-divergence appendix |
| `docs/book/chNN-<slug>.md` × 13 | One note per chapter; twelve stubs plus Chapter 1 written in full |
| `../operating-system/*.md` × 13 | Gain one backlink line each, in `## Related Notes` |

---

### Task 1: Verification harness

Written first so the remaining tasks have a red/green signal. Markdown has no unit tests, but every structural claim in the spec is mechanically checkable, and this script checks all four.

**Files:**
- Create: `docs/book/check-notes.sh`

**Interfaces:**
- Consumes: nothing.
- Produces: `docs/book/check-notes.sh`, exit 0 when all checks pass and exit 1 otherwise, printing one `FAIL:` line per problem. Later tasks run it as their test step.

- [ ] **Step 1: Write the checker**

Create `docs/book/check-notes.sh`:

```bash
#!/usr/bin/env bash
# Structural checks for the cross-repo note system.
# Usage: docs/book/check-notes.sh
set -uo pipefail

REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BOOK="$REPO/../xv6-riscv-book"
OSTEP="$REPO/../operating-system"
README="$REPO/docs/book/README.md"
CONC="$REPO/docs/book/00-ostep-concordance.md"

FAILS="$(mktemp)"
trap 'rm -f "$FAILS"' EXIT

bad() {
  printf '  FAIL: %s\n' "$*"
  printf '  FAIL: %s\n' "$*" >>"$FAILS"
}

# 1. Every relative markdown link resolves from the file that contains it.
#    docs/superpowers/ is excluded: specs and plans quote example links
#    that are illustrations, not navigation.
check_links() {
  echo "== relative links resolve =="
  find "$REPO/docs" -name '*.md' -not -path '*/superpowers/*' -print0 |
  while IFS= read -r -d '' f; do
    grep -oE '\]\([^)]+\)' "$f" 2>/dev/null |
      sed -E 's/^\]\(//; s/\)$//' |
    while IFS= read -r target; do
      case "$target" in http*|mailto:*|'#'*) continue ;; esac
      path="${target%%#*}"
      [ -z "$path" ] && continue
      [ -e "$(dirname "$f")/$path" ] || bad "${f#"$REPO"/} -> $target"
    done
  done
}

# 2. The README chapter table matches book.tex, in include order.
check_chapters() {
  echo "== README chapter table matches book.tex =="
  [ -f "$README" ] || { bad "missing $README"; return; }
  local n=0 stem title
  while IFS= read -r stem; do
    n=$((n + 1))
    title="$(sed -n 's/^\\chapter{\(.*\)}[[:space:]]*$/\1/p' "$BOOK/$stem.tex" | head -1)"
    [ -n "$title" ] || { bad "no \\chapter found in $stem.tex"; continue; }
    grep -qE "^\| \*\*$n\*\*[^|]*\| *$title *\|" "$README" ||
      bad "chapter $n should read '$title' (from $stem.tex)"
  done < <(sed -n 's#^\\input{latex.out/\([a-z0-9]*\)}#\1#p' "$BOOK/book.tex" | grep -v '^acks$')
}

# 3. Every file and symbol cited in the concordance actually exists.
check_symbols() {
  echo "== concordance code references exist =="
  [ -f "$CONC" ] || { bad "missing $CONC"; return; }
  grep -E '^\|' "$CONC" |
  while IFS= read -r row; do
    file=""
    for tok in $(printf '%s\n' "$row" | grep -oE '`[^` ]+`' | tr -d '`'); do
      case "$tok" in
        *.c|*.h|*.S|*.c:[0-9]*|*.h:[0-9]*|*.S:[0-9]*|*.ld)
          file="${tok%%:*}"
          [ -e "$REPO/$file" ] || bad "no such file: $file"
          ;;
        *'()')
          sym="${tok%'()'}"
          [ -n "$file" ] && [ -e "$REPO/$file" ] || continue
          grep -qE "\b${sym}[[:space:]]*\(" "$REPO/$file" ||
            bad "$sym not found in $file"
          ;;
      esac
    done
  done
}

# 4. No row claims 'linked' while its OSTEP note is still a placeholder.
check_status() {
  echo "== no 'linked' row points at a placeholder note =="
  [ -f "$CONC" ] || return
  grep -E '\| *linked *\|[[:space:]]*$' "$CONC" |
  while IFS= read -r row; do
    for tok in $(printf '%s\n' "$row" | grep -oE '`[^` ]+\.md`' | tr -d '`'); do
      if [ ! -e "$OSTEP/$tok" ]; then
        bad "no such OSTEP note: $tok"
      elif grep -q 'Placeholder — not yet written' "$OSTEP/$tok"; then
        bad "$tok is a placeholder but its row status is 'linked'"
      fi
    done
  done
}

check_links
check_chapters
check_symbols
check_status

echo
if [ -s "$FAILS" ]; then
  echo "FAILED — $(wc -l <"$FAILS") problem(s)."
  exit 1
fi
echo "All checks passed."
```

- [ ] **Step 2: Make it executable and run it to verify it fails**

```bash
chmod +x docs/book/check-notes.sh
docs/book/check-notes.sh
```

Expected: exit 1 with **exactly 10 problems** — this was verified against the current tree while writing the plan:

```
== relative links resolve ==
== README chapter table matches book.tex ==
  FAIL: chapter 5 should read 'Page faults' (from pgfault.tex)
  FAIL: chapter 6 should read 'Interrupts and device drivers' (from interrupt.tex)
  FAIL: chapter 7 should read 'Locking' (from lock.tex)
  FAIL: chapter 8 should read 'Scheduling' (from sched.tex)
  FAIL: chapter 9 should read 'Sleep and Wakeup' (from sleep.tex)
  FAIL: chapter 10 should read 'File system' (from fs.tex)
  FAIL: chapter 11 should read 'Logging' (from log.tex)
  FAIL: chapter 12 should read 'Concurrency revisited' (from lock2.tex)
  FAIL: chapter 13 should read 'Summary' (from sum.tex)
== concordance code references exist ==
  FAIL: missing /home/wahsieh/personal/xv6-riscv/docs/book/00-ostep-concordance.md

FAILED — 10 problem(s).
```

Chapters 1–4 pass because the existing table happens to have them right; 5 onward are misnumbered and 11–13 are absent entirely. The link check is silent because the existing `docs/` links all resolve.

If the chapter check reports *zero* failures, the script is wrong: the README demonstrably lists ten chapters against the book's thirteen. Debug before continuing.

- [ ] **Step 3: Commit**

```bash
git add docs/book/check-notes.sh
git commit -F - <<'MSG'
test(notes): add structural checker for the cross-repo note system

- Added docs/book/check-notes.sh to verify the four structural claims
  the note system rests on, since Markdown has no unit tests and link
  rot is otherwise silent until a reader hits it.

- Checked that every relative link under docs/ resolves from the file
  containing it, which is the only guard on cross-repo paths that are
  deliberately unrenderable on the web.

- Checked the README chapter table against the \chapter lines of
  book.tex in include order, which is what exposed the three missing
  chapters in the first place.

- Checked that every file and symbol cited in the concordance exists,
  and that no row claims linked status while its OSTEP counterpart is
  still a placeholder.

Cursor(%CUR): Yes
Effort: 2 SP
Co-authored-by: Claude Code (claude-opus-5)
MSG
```

---

### Task 2: Correct and extend `docs/book/README.md`

**Files:**
- Modify: `docs/book/README.md` — replace the chapter table at lines 15–26, and the "Naming" and "Suggested shape" sections below it.

**Interfaces:**
- Consumes: `docs/book/check-notes.sh` from Task 1.
- Produces: a chapter table whose thirteen rows are matched by `check_chapters`, and the documented conventions that Tasks 4 and 5 follow.

- [ ] **Step 1: Run the checker to confirm the chapter table is red**

```bash
docs/book/check-notes.sh 2>&1 | grep 'chapter'
```

Expected: several `FAIL: chapter N should read ...` lines.

- [ ] **Step 2: Replace the chapter table**

Replace lines 15–26 of `docs/book/README.md` (the header row through the `**10**` row) with:

```markdown
| #      | Chapter                       | Mostly about                                  | Case     | Notes |
| ------ | ----------------------------- | --------------------------------------------- | -------- | ----- |
| **1**  | Operating system interfaces   | Processes, files, pipes, the shell            | linked   | —     |
| **2**  | Operating system organization | Kernel/user split, machine modes, boot        | mixed    | —     |
| **3**  | Page tables                   | Sv39, address spaces, `kernel/vm.c`           | linked   | —     |
| **4**  | Traps and system calls        | `ecall`, trampoline, trapframe                | mixed    | —     |
| **5**  | Page faults                   | COW, lazy allocation, demand paging           | loan     | —     |
| **6**  | Interrupts and device drivers | UART, PLIC, virtio disk                       | loan     | —     |
| **7**  | Locking                       | Spinlocks, races, deadlock                    | loan     | —     |
| **8**  | Scheduling                    | Context switching, `swtch.S`                  | linked   | —     |
| **9**  | Sleep and Wakeup              | Sleep/wakeup, condition variables             | loan     | —     |
| **10** | File system                   | Inodes, the buffer cache                      | loan     | —     |
| **11** | Logging                       | Journaling, crash consistency                 | loan     | —     |
| **12** | Concurrency revisited         | Memory ordering, fences, lock-free            | loan     | —     |
| **13** | Summary                       | —                                             | xv6-only | —     |
```

- [ ] **Step 3: Run the checker to verify the chapter table is green**

```bash
docs/book/check-notes.sh 2>&1 | grep -c 'FAIL: chapter'
```

Expected: `0`. The concordance failure remains — that is Task 3.

- [ ] **Step 4: Replace the "Suggested shape" section**

Replace the existing `## Suggested shape` section and its code block with:

````markdown
## How a chapter note is written

The rule that keeps these notes short: **`../operating-system/` owns concepts; this directory owns xv6's realization of them.** The test when placement is unclear is *would this sentence still be true on Linux?* Yes means it belongs in the OSTEP note; no means it belongs here. A chapter note that explains what `fork` *is* has violated the rule.

That rule assumes the concept note exists, and for most chapters it does not yet. So every concept falls into one of three cases, recorded in [00-ostep-concordance.md](00-ostep-concordance.md):

| Case | When | Where the explanation goes |
| ---- | ---- | -------------------------- |
| **linked** | The OSTEP counterpart is written | Link to it; define nothing here. |
| **loan** | The OSTEP counterpart is a placeholder | Write a short `## Concept on loan` block here, naming its destination. |
| **xv6-only** | No OSTEP counterpart exists | This note owns it permanently. |

A loan is a debt, not a home. `grep -rn '^## Concept on loan' docs/book/` lists every one outstanding. When the OSTEP note is finally written, move the block there, replace it with a link, and flip the concordance row to `linked`.

### Shape

```markdown
# Chapter N: Title

> **Theory:** [OSTEP — …][ostep-x]. Read that first; this note does not re-explain it.

## What xv6 actually does
## Concept on loan: <topic>        <- only while the OSTEP note is a placeholder
## Divergences
## Code walked through
## Questions I had
## Lab connection

[ostep-x]: ../../../operating-system/<Note>.md#<anchor>
```

`What xv6 actually does` is the only mandatory section. For an `xv6-only` chapter, the blockquote instead states that no OSTEP counterpart exists, so the note stands alone by design.

`Divergences` covers three kinds, each of which would otherwise look like somebody's mistake:

- **Naming** — this tree's `kfork`/`kexec`/`kwait`/`kexit` against `fork`/`exec`/`wait` in both the book prose and OSTEP.
- **Architecture** — RISC-V `ra`/`sp`/`s0`–`s11` against the x86 `eip`/`esp`/`ebx`/`ebp` in OSTEP's figures.
- **Vintage** — OSTEP's xv6 material predates the 2019 RISC-V port and describes `xv6-public`, the older x86 tree.

Only chapters that actually diverge carry the section, and it links the concordance appendix rather than restating the correspondence table.

## Cross-repo links

Links into the OSTEP notes are **relative and editor-only**: `../../../operating-system/…` from this directory, which sits two levels below the repository root. They are ctrl-clickable in an editor and survive renames.

> [!IMPORTANT]
> These links 404 on GitHub, by design. The two repositories live on different hosts, so no relative path can render on both. Do not "fix" them into absolute URLs. They assume both repositories sit side by side under `~/personal/`.

Run [check-notes.sh](check-notes.sh) after editing to confirm every link still resolves.
````

- [ ] **Step 5: Update the "Naming" section**

Replace the `## Naming` code block's contents with the real filenames, so it matches what Task 4 generates:

```
ch01-operating-system-interfaces.md
ch05-page-faults.md
ch13-summary.md
```

- [ ] **Step 6: Run the full checker**

```bash
docs/book/check-notes.sh
```

Expected: exit 1, with only the missing-concordance failure remaining. No link failures — the README now links to `00-ostep-concordance.md` and `check-notes.sh`, and the latter exists. If `00-ostep-concordance.md` is reported as an unresolved link, that is expected and clears in Task 3.

- [ ] **Step 7: Commit**

```bash
git add docs/book/README.md
git commit -F - <<'MSG'
docs(book): correct the chapter table and document the note rules

- Corrected the chapter table from ten entries to the book's actual
  thirteen, adding Page faults, Sleep and Wakeup, and Logging, which
  were missing, and renumbering every chapter from five onward.

- Added a Case column recording whether each chapter links to a
  written OSTEP note, borrows a concept from a placeholder, or owns
  its material outright.

- Documented the ownership rule and its Linux test, so a chapter note
  that starts defining a concept is recognisably wrong rather than
  merely long.

- Documented the three-case model and the greppable Concept on loan
  heading, which is what makes borrowed material findable later.

- Documented the cross-repo link convention and warned that the paths
  404 on GitHub deliberately, so they are not converted to URLs.

Cursor(%CUR): Yes
Effort: 2 SP
Co-authored-by: Claude Code (claude-opus-5)
MSG
```

---

### Task 3: The concordance

**Files:**
- Create: `docs/book/00-ostep-concordance.md`

**Interfaces:**
- Consumes: the case vocabulary documented in Task 2.
- Produces: nineteen concept rows and the tree-divergence appendix, both referenced by Tasks 4 and 5. Every code path in it is repo-relative from the xv6-riscv root, because `check_symbols` resolves them that way.

- [ ] **Step 1: Write the concordance**

Create `docs/book/00-ostep-concordance.md`:

````markdown
# OSTEP Concordance

---

[toc]

---

One concept, mapped across every source that covers it. **Read this before writing a chapter note**: it tells you what is already explained elsewhere, and who owns it.

Statuses are defined in [README.md](README.md#how-a-chapter-note-is-written): **linked** means the OSTEP note is written and this repository links to it; **loan** means the OSTEP note is still a placeholder, so an xv6 note holds the concept temporarily; **xv6-only** means no OSTEP counterpart exists and the xv6 note owns it permanently.

## Concepts

| Concept | OSTEP note | xv6 ch | xv6 source | Exercise | Status |
| ------- | ---------- | ------ | ---------- | -------- | ------ |
| **fork/exec/wait** | `The_Process_Abstraction.md` §7 | 1 | `kernel/proc.c` `kfork()`, `kernel/exec.c` `kexec()` | `codes/` `cpu-process-api/fork_*.c` | linked |
| **process states, PCB** | `The_Process_Abstraction.md` §4–5 | 1 | `kernel/proc.c` `allocproc()` | `codes/` `cpu-process-intro/` | linked |
| **fds, pipes, shell** | `The_Process_Abstraction.md` §7 | 1 | `kernel/pipe.c` `pipealloc()`, `user/sh.c` `runcmd()` | `codes/` `parent_children_pipe.c` | linked |
| **kernel/user split** | `Introduction_to_Operating_Systems.md` | 2 | `kernel/main.c`, `kernel/proc.c` `allocproc()` | — | linked |
| **machine mode, boot** | — | 2 | `kernel/entry.S`, `kernel/start.c` | — | xv6-only |
| **paging, Sv39** | `Paging.md` | 3 | `kernel/vm.c` `walk()`, `uvmalloc()` | `ostep-homework/vm-paging/` | linked |
| **address spaces** | `Address_Spaces_And_Translation.md` | 3 | `kernel/vm.c` `uvmcopy()` | `ostep-homework/vm-mechanism/` | linked |
| **memory layout, segments** | `Memory_Management.md` | 3 | `kernel/memlayout.h`, `user/user.ld` | `ostep-homework/vm-freespace/` | linked |
| **limited direct execution** | `The_Process_Abstraction.md` §8 | 4 | `kernel/trap.c` `usertrap()`, `kerneltrap()` | `codes/` `lde_cost.c` | linked |
| **trampoline, trapframe** | — | 4 | `kernel/trampoline.S` | lab: traps | xv6-only |
| **page faults, COW** | `Swapping_And_VM_Systems.md` | 5 | `kernel/trap.c` `usertrap()`, `kernel/vm.c` `uvmcopy()` | lab: cow | loan |
| **device drivers, PLIC** | `IO_Devices_And_Disks.md` | 6 | `kernel/uart.c` `uartintr()`, `kernel/plic.c` `plic_claim()` | `ostep-homework/file-devices/` | loan |
| **spinlocks, races** | `Concurrency_Threads_And_Locks.md` | 7 | `kernel/spinlock.c` `acquire()`, `release()` | `ostep-homework/threads-locks/` | loan |
| **context switch** | `CPU_Scheduling.md` | 8 | `kernel/swtch.S`, `kernel/proc.c` `sched()`, `scheduler()` | `ostep-homework/cpu-sched/` | linked |
| **sleep/wakeup, cond vars** | `Condition_Variables_And_Semaphores.md` | 9 | `kernel/proc.c` `sleep()`, `wakeup()` | `ostep-homework/threads-cv/` | loan |
| **inodes, buffer cache** | `File_Systems.md` | 10 | `kernel/fs.c` `ialloc()`, `kernel/bio.c` `bread()` | `ostep-homework/file-implementation/` | loan |
| **journaling, crash consistency** | `Crash_Consistency_And_Journaling.md` | 11 | `kernel/log.c` `begin_op()`, `write_log()` | `ostep-homework/file-journaling/` | loan |
| **memory ordering, fences** | `Concurrency_Bugs_And_Events.md` | 12 | `kernel/spinlock.c`, `kernel/virtio_disk.c` `io_fence()` | — | loan |
| **summary** | — | 13 | — | — | xv6-only |

OSTEP notes live in `../../../operating-system/`. The `Exercise` column spans three repositories: `codes/` is `../../../operating-system/codes/src/main/virtualization/`, `ostep-homework/` is `../../../ostep-homework/`, and `lab:` names a lab in this tree.

> [!WARNING]
> `../../../ostep-projects/` also contains six xv6-based projects — `initial-xv6`, `initial-xv6-tracer`, `scheduling-xv6-lottery`, `vm-xv6-intro`, `concurrency-xv6-threads`, and `filesystems-checker`. All six target **`xv6-public`**, the older x86 tree, and none of them build against this repository. They are deliberately absent from the table above.

## Appendix: divergence between the two xv6 trees

OSTEP's xv6 material describes [`mit-pdos/xv6-public`](https://github.com/mit-pdos/xv6-public), the 32-bit x86 tree. This repository is [`mit-pdos/xv6-riscv`](https://github.com/mit-pdos/xv6-riscv). They are one lineage, not rivals: this repository's history begins at the same `import` commit of 2006-06-12, and the RISC-V port lands in 2019. OSTEP's figures simply predate it.

A survey of the OSTEP notes bounds the problem to a single file. `Paging.md`, `Address_Spaces_And_Translation.md`, and `CPU_Scheduling.md` never mention xv6 at all — their x86 content is OSTEP's own teaching architecture, unrelated to either tree. `Memory_Management.md` mentions it once, already naming `sp` for RISC-V. Only `The_Process_Abstraction.md` is affected, and only in its Figure 4.5 and Figure 6.4 listings.

> [!IMPORTANT]
> That note is **not wrong and is not to be corrected.** It faithfully reproduces the figures of the book it takes notes on. Rewriting them to RISC-V would desync the note from its source and invent material OSTEP does not contain. Under the ownership rule the translation is this repository's job, and this appendix is where it lives.

### Context switch registers

OSTEP's Figure 4.5 shows the x86 `struct context`; this tree's is in `kernel/proc.h`.

| Role | `xv6-public` (x86, 32-bit) | `xv6-riscv` (this tree) |
| ---- | -------------------------- | ----------------------- |
| Return address / PC | `eip` | `ra` |
| Stack pointer | `esp` | `sp` |
| Callee-saved | `ebx`, `ecx`, `edx`, `esi`, `edi`, `ebp` | `s0`–`s11` |
| Width | 32-bit | 64-bit (`uint64`) |

The difference is structural, not cosmetic: RISC-V has no dedicated frame pointer register in the ABI sense, and its callee-saved set is twelve registers wide, so the saved context is larger and flatter. The switch itself is `kernel/swtch.S`, called from `kernel/proc.c` `sched()`.

### System call naming

This revision prefixes the kernel-side implementations, where both the book prose and OSTEP use the bare POSIX names.

| Book prose and OSTEP | This tree | Defined in |
| -------------------- | --------- | ---------- |
| `fork` | `kfork()` | `kernel/proc.c` |
| `exec` | `kexec()` | `kernel/exec.c` |
| `wait` | `kwait()` | `kernel/proc.c` |
| `exit` | `kexit()` | `kernel/proc.c` |

The user-facing names in `user/user.h` are unprefixed, so a program calls `fork()` and the kernel implements `kfork()`.
````

- [ ] **Step 2: Run the checker to verify it passes**

```bash
docs/book/check-notes.sh
```

Expected: `All checks passed.` and exit 0. Every file and symbol in the table is verified to exist, no `linked` row points at a placeholder, and every relative link resolves.

If `check_symbols` reports a missing symbol, fix the concordance rather than the check — the symbols were confirmed present at planning time and a failure means a typo in the table.

- [ ] **Step 3: Commit**

```bash
git add docs/book/00-ostep-concordance.md
git commit -F - <<'MSG'
docs(book): add the OSTEP concordance and tree-divergence appendix

- Added a concept map spanning the OSTEP notes, their runnable demos,
  the official homework, the xv6 book, this kernel, and the labs, so
  that a chapter note can be checked against it before being written.

- Assigned every one of the nineteen concepts an ownership status, of
  which seven are loans against OSTEP notes that are still
  placeholders and would otherwise have nowhere to link.

- Documented that all six xv6-based OSTEP projects target xv6-public
  and build against none of this tree, which is why they are absent
  from the table rather than merely unlisted.

- Added an appendix translating OSTEP's x86 context-switch figures to
  this tree's RISC-V equivalents, because the OSTEP note reproduces
  the book faithfully and must not be rewritten to match.

- Tabulated the kfork, kexec, kwait, and kexit naming so the prefixed
  kernel implementations are not mistaken for a different API.

Cursor(%CUR): Yes
Effort: 3 SP
Co-authored-by: Claude Code (claude-opus-5)
MSG
```

---

### Task 4: Generate the thirteen chapter stubs

**Files:**
- Create: `docs/book/ch01-operating-system-interfaces.md` through `docs/book/ch13-summary.md`

**Interfaces:**
- Consumes: the shape and case vocabulary from Task 2, the concordance from Task 3.
- Produces: thirteen files matching `docs/book/ch[0-9][0-9]-*.md`, each with a resolving theory link. Task 5 overwrites `ch01-operating-system-interfaces.md`; Task 6's backlinks point at these paths, so they must exist first.

- [ ] **Step 1: Generate the stubs**

Run this from the repository root. The data table drives every file, so the thirteen stay consistent by construction:

```bash
cd docs/book
while IFS='|' read -r num slug title src theory ostep; do
  [ -z "$num" ] && continue
  file="ch${num}-${slug}.md"
  {
    printf '# Chapter %s: %s\n\n---\n\n[toc]\n\n---\n\n' "${num#0}" "$title"
    if [ "$ostep" = "-" ]; then
      printf '> **No OSTEP counterpart.** This chapter is xv6-only, so this note stands alone by design and owns its material permanently.\n\n'
    else
      printf '> **Theory:** [OSTEP — %s][ostep]. Read that first; this note does not re-explain it.\n\n' "$theory"
    fi
    printf '> **Placeholder — not yet written.** Chapter source: `../../../xv6-riscv-book/%s`. Write it with the shape in [README.md](README.md#how-a-chapter-note-is-written), after checking [00-ostep-concordance.md](00-ostep-concordance.md) for what is already covered.\n\n' "$src"
    printf '## What xv6 actually does\n\n## Divergences\n\n## Code walked through\n\n## Questions I had\n\n## Lab connection\n'
    [ "$ostep" != "-" ] && printf '\n[ostep]: ../../../operating-system/%s\n' "$ostep"
  } >"$file"
  echo "wrote $file"
done <<'EOF'
01|operating-system-interfaces|Operating system interfaces|unix.tex|The Process Abstraction|The_Process_Abstraction.md
02|operating-system-organization|Operating system organization|first.tex|Introduction to Operating Systems|Introduction_to_Operating_Systems.md
03|page-tables|Page tables|mem.tex|Paging|Paging.md
04|traps-and-system-calls|Traps and system calls|trap.tex|The Process Abstraction §8|The_Process_Abstraction.md
05|page-faults|Page faults|pgfault.tex|Swapping and VM Systems|Swapping_And_VM_Systems.md
06|interrupts-and-device-drivers|Interrupts and device drivers|interrupt.tex|IO Devices and Disks|IO_Devices_And_Disks.md
07|locking|Locking|lock.tex|Concurrency: Threads and Locks|Concurrency_Threads_And_Locks.md
08|scheduling|Scheduling|sched.tex|CPU Scheduling|CPU_Scheduling.md
09|sleep-and-wakeup|Sleep and Wakeup|sleep.tex|Condition Variables and Semaphores|Condition_Variables_And_Semaphores.md
10|file-system|File system|fs.tex|File Systems|File_Systems.md
11|logging|Logging|log.tex|Crash Consistency and Journaling|Crash_Consistency_And_Journaling.md
12|concurrency-revisited|Concurrency revisited|lock2.tex|Concurrency: Bugs and Events|Concurrency_Bugs_And_Events.md
13|summary|Summary|sum.tex|-|-
EOF
cd ../..
```

The six fields are chapter number, filename slug, title, book source file, OSTEP note title, and OSTEP note filename. Chapter 13 carries `-` in the last two fields, which selects the `xv6-only` blockquote and emits no `[ostep]:` definition.

- [ ] **Step 2: Verify all thirteen exist with resolving links**

```bash
ls docs/book/ch*.md | wc -l
docs/book/check-notes.sh
```

Expected: `13`, then `All checks passed.` If a theory link fails to resolve, the `[ostep]:` target is wrong — check the depth is `../../../`, not `../../`.

- [ ] **Step 3: Fill the README Notes column**

In `docs/book/README.md`, replace each row's trailing `—` in the `Notes` column with a link to its stub, for example `[ch01](ch01-operating-system-interfaces.md)`.

- [ ] **Step 4: Re-run the checker**

```bash
docs/book/check-notes.sh
```

Expected: `All checks passed.` — this confirms all thirteen README links resolve.

- [ ] **Step 5: Commit**

```bash
git add docs/book/ch*.md docs/book/README.md
git commit -F - <<'MSG'
docs(book): stub all thirteen chapter notes

- Added one stub per chapter, generated from a single data table so
  that the thirteen share a shape by construction rather than by
  thirteen separate acts of discipline.

- Gave each stub its theory link into the matching OSTEP note, or a
  statement that no counterpart exists where the chapter is xv6-only,
  so no note is silently incomplete.

- Stubbed every chapter now rather than one at a time, which keeps
  the concordance and both link directions complete and means no
  backlink added later points at a file that does not exist.

- Linked each stub from the README chapter table, closing the loop
  between the progress checklist and the notes themselves.

Cursor(%CUR): Yes
Effort: 2 SP
Co-authored-by: Claude Code (claude-opus-5)
MSG
```

---

### Task 5: Write Chapter 1 in full

**Files:**
- Modify: `docs/book/ch01-operating-system-interfaces.md` — replace the stub body
- Read: `../xv6-riscv-book/unix.tex` (1116 lines)

**Interfaces:**
- Consumes: the concordance appendix from Task 3 for the divergence content.
- Produces: the worked example of a `linked` chapter note, which Tasks for chapters 2–13 will imitate.

- [ ] **Step 1: Read the source**

Read `../xv6-riscv-book/unix.tex` in full, and re-read `../operating-system/The_Process_Abstraction.md` §7 (`## 7. The Process API: fork / exec / wait (Ch 5)`, line 576 onward) so you know exactly what is already explained and must not be repeated.

- [ ] **Step 2: Write the note**

Keep the stub's header, `[toc]`, and theory blockquote. Remove the placeholder blockquote. Fill the sections:

**`## What xv6 actually does`** — the mandatory section, and the bulk of the note. Cover, in xv6's terms and with the code anchors named:

- Process creation and the fork/exec split as *this kernel* implements it: `kernel/proc.c` `kfork()` copying the address space via `kernel/vm.c` `uvmcopy()`, and `kernel/exec.c` `kexec()` replacing it.
- File descriptors as an indirection table: `kernel/file.c` `filealloc()`, the `ofile` array in `struct proc` (`kernel/proc.h`), and why `kfork()` duplicating that array is what makes shell redirection work.
- Pipes: `kernel/pipe.c` `pipealloc()` and the buffer it wraps.
- The shell as an ordinary user program: `user/sh.c` `runcmd()`, and `main()` at `user/sh.c:146`.

Do **not** explain why `fork` and `exec` are separate calls, what a file descriptor is conceptually, or what a process is — the theory link covers all three. If a paragraph would be equally true of Linux, it belongs in the OSTEP note, not here.

**`## Divergences`** — three short items, linking rather than restating:

- Naming: the kernel implements `kfork()`, `kexec()`, `kwait()`, `kexit()`; user programs call the unprefixed names from `user/user.h`. Link to [the naming table](00-ostep-concordance.md#system-call-naming).
- Architecture and vintage: OSTEP's Figure 4.5 `struct proc` and Figure 6.4 `swtch` are the x86 `xv6-public` tree's. Link to [the register table](00-ostep-concordance.md#context-switch-registers). One sentence, no restatement.

**`## Code walked through`** — a table of `file` / `symbol` / what it does, using the anchors above.

**`## Questions I had`** — genuine ones from reading. If none, delete the section rather than padding it.

**`## Lab connection`** — the `util` lab, which is the active lab per `conf/lab.mk`.

- [ ] **Step 3: Verify the ownership rule held**

```bash
docs/book/check-notes.sh
grep -nE '^\s*(A |The )?(process|file descriptor) is\b' docs/book/ch01-operating-system-interfaces.md
```

Expected: `All checks passed.`, and **no output** from the grep. A hit means the note started defining a concept the OSTEP repository owns — move it out or replace it with a link.

- [ ] **Step 4: Verify against the distil rule**

Confirm no sentence was lifted from `unix.tex`. Spot-check by picking three distinctive phrases from your note and grepping the source:

```bash
grep -c 'YOUR PHRASE HERE' ../xv6-riscv-book/unix.tex
```

Expected: `0` for each.

- [ ] **Step 5: Commit**

```bash
git add docs/book/ch01-operating-system-interfaces.md
git commit -F - <<'MSG'
docs(book): write the chapter 1 note on operating system interfaces

- Wrote the first chapter note in full as the worked example of a
  linked chapter, where the concept is already covered by an OSTEP
  note and this note therefore explains only what xv6 does.

- Traced the fork and exec split through this kernel specifically,
  naming kfork, uvmcopy, and kexec, so the note is anchored to code
  rather than restating an abstraction the theory link already owns.

- Explained file descriptors as the indirection that makes shell
  redirection work, by way of the ofile array that kfork duplicates.

- Recorded the naming, architecture, and vintage divergences by link
  rather than restatement, keeping the correspondence tables in the
  concordance where the later chapters can share them.

Cursor(%CUR): Yes
Effort: 3 SP
Co-authored-by: Claude Code (claude-opus-5)
MSG
```

---

### Task 6: Backlinks in the OSTEP repository

**Files:**
- Modify (in `../operating-system/`): `The_Process_Abstraction.md`, `Paging.md`, `Address_Spaces_And_Translation.md`, `CPU_Scheduling.md`, `Introduction_to_Operating_Systems.md`, `Memory_Management.md`, `Swapping_And_VM_Systems.md`, `IO_Devices_And_Disks.md`, `Concurrency_Threads_And_Locks.md`, `Condition_Variables_And_Semaphores.md`, `File_Systems.md`, `Crash_Consistency_And_Journaling.md`, `Concurrency_Bugs_And_Events.md`

**Interfaces:**
- Consumes: the thirteen stub paths created in Task 4. Every backlink target must already exist.
- Produces: nothing consumed by later tasks. This is the final task.

> [!CAUTION]
> This task runs in a **different repository** that has unrelated uncommitted work — `Paging.md` is modified and three files under `codes/` are staged. Stage only the files you edit, by explicit path. Do not run `git add -A`, `git add .`, or `git commit -a`. `Paging.md` is both on your edit list and already dirty, so inspect `git diff Paging.md` before staging it and confirm the only change you are adding is your backlink.

- [ ] **Step 1: Record the pre-existing state**

```bash
cd ../operating-system
git status --short > /tmp/ostep-before.txt
cat /tmp/ostep-before.txt
```

Keep this. Step 5 diffs against it to prove nothing unrelated was swept in.

- [ ] **Step 2: Append the line to the four notes that already have the section**

Each of these has a `## Related Notes` section. Append one bullet to the end of that section's list — do not touch anything else in the file.

`The_Process_Abstraction.md`:

```markdown
- [xv6 Ch 1 — Operating system interfaces](../xv6-riscv/docs/book/ch01-operating-system-interfaces.md) — the same `fork`/`exec`/`wait` in `xv6-riscv` (kernel-side: `kfork`/`kexec`/`kwait`). Note that §4.5's and §6's figures above are the older `xv6-public` x86 tree; the RISC-V equivalents are tabulated in [the concordance appendix](../xv6-riscv/docs/book/00-ostep-concordance.md#appendix-divergence-between-the-two-xv6-trees).
- [xv6 Ch 4 — Traps and system calls](../xv6-riscv/docs/book/ch04-traps-and-system-calls.md) — limited direct execution as this kernel implements it: `usertrap()`, the trapframe, and the trampoline page.
```

`Paging.md`:

```markdown
- [xv6 Ch 3 — Page tables](../xv6-riscv/docs/book/ch03-page-tables.md) — Sv39 three-level page tables in `xv6-riscv`, walked by `kernel/vm.c` `walk()`.
```

`Address_Spaces_And_Translation.md`:

```markdown
- [xv6 Ch 3 — Page tables](../xv6-riscv/docs/book/ch03-page-tables.md) — how `xv6-riscv` builds and copies an address space, in `kernel/vm.c` `uvmcopy()`.
```

`CPU_Scheduling.md`:

```markdown
- [xv6 Ch 8 — Scheduling](../xv6-riscv/docs/book/ch08-scheduling.md) — the mechanism side in `xv6-riscv`: `kernel/swtch.S` and `kernel/proc.c` `sched()`.
```

- [ ] **Step 3: Create the section in the two written notes that lack it**

In `Introduction_to_Operating_Systems.md` and `Memory_Management.md`, insert a `## Related Notes` section **between `## Useful Links` and `## Terminology`** — the position `templates/NOTE_TEMPLATE.md` specifies, and where the four notes above keep it.

`Introduction_to_Operating_Systems.md` (before line 46, `## Terminology`):

```markdown
## Related Notes

- [xv6 Ch 2 — Operating system organization](../xv6-riscv/docs/book/ch02-operating-system-organization.md) — the kernel/user split made concrete in `xv6-riscv`, plus the machine-mode boot path that has no OSTEP counterpart.

---
```

`Memory_Management.md` (before line 45, `## Terminology`):

```markdown
## Related Notes

- [xv6 Ch 3 — Page tables](../xv6-riscv/docs/book/ch03-page-tables.md) — where `xv6-riscv` places the segments described here: `kernel/memlayout.h` and `user/user.ld`.

---
```

- [ ] **Step 4: Append the section to the seven placeholder notes**

Each placeholder ends with its `> **Placeholder — not yet written.**` blockquote. Append the section **after** that blockquote, leaving it untouched. Use this table for the one bullet each:

| File | Bullet |
| ---- | ------ |
| `Swapping_And_VM_Systems.md` | `- [xv6 Ch 5 — Page faults](../xv6-riscv/docs/book/ch05-page-faults.md) — COW and lazy allocation in `xv6-riscv`. **That note may hold a concept on loan destined for this file** — check before writing.` |
| `IO_Devices_And_Disks.md` | `- [xv6 Ch 6 — Interrupts and device drivers](../xv6-riscv/docs/book/ch06-interrupts-and-device-drivers.md) — UART, PLIC, and the virtio disk. **May hold a concept on loan destined here.**` |
| `Concurrency_Threads_And_Locks.md` | `- [xv6 Ch 7 — Locking](../xv6-riscv/docs/book/ch07-locking.md) — spinlocks in `kernel/spinlock.c`. **May hold a concept on loan destined here.**` |
| `Condition_Variables_And_Semaphores.md` | `- [xv6 Ch 9 — Sleep and Wakeup](../xv6-riscv/docs/book/ch09-sleep-and-wakeup.md) — `kernel/proc.c` `sleep()` and `wakeup()`. **May hold a concept on loan destined here.**` |
| `File_Systems.md` | `- [xv6 Ch 10 — File system](../xv6-riscv/docs/book/ch10-file-system.md) — inodes and the buffer cache. **May hold a concept on loan destined here.**` |
| `Crash_Consistency_And_Journaling.md` | `- [xv6 Ch 11 — Logging](../xv6-riscv/docs/book/ch11-logging.md) — `kernel/log.c` and its write-ahead log. **May hold a concept on loan destined here.**` |
| `Concurrency_Bugs_And_Events.md` | `- [xv6 Ch 12 — Concurrency revisited](../xv6-riscv/docs/book/ch12-concurrency-revisited.md) — memory ordering and RISC-V fences. **May hold a concept on loan destined here.**` |

Each gets the heading, a blank line, its bullet, and a closing `---`:

```markdown
## Related Notes

<bullet from the table above>

---
```

The "concept on loan" warning is the point of backlinking a placeholder: at the moment you finally write that note, it tells you an xv6 note may be holding material that belongs here.

- [ ] **Step 5: Verify every backlink resolves and nothing unrelated was touched**

```bash
cd ../operating-system
for f in $(git diff --name-only; git diff --cached --name-only) ; do :; done
grep -ho '](\.\./xv6-riscv[^)]*)' *.md | tr -d '](' | sed 's/)$//' | sort -u |
  while read -r p; do [ -e "$p" ] || echo "BROKEN: $p"; done
git status --short | diff /tmp/ostep-before.txt - || true
```

Expected: no `BROKEN:` lines. The `diff` should show only the thirteen note files newly appearing as modified — if any file under `codes/` changed state, stop and investigate.

- [ ] **Step 6: Commit, staging by explicit path only**

```bash
cd ../operating-system
git add The_Process_Abstraction.md Paging.md Address_Spaces_And_Translation.md \
        CPU_Scheduling.md Introduction_to_Operating_Systems.md Memory_Management.md \
        Swapping_And_VM_Systems.md IO_Devices_And_Disks.md Concurrency_Threads_And_Locks.md \
        Condition_Variables_And_Semaphores.md File_Systems.md \
        Crash_Consistency_And_Journaling.md Concurrency_Bugs_And_Events.md
git status --short
```

Confirm the staged set is exactly those thirteen plus whatever was already staged under `codes/` before you started. Then:

```bash
git commit -F - <<'MSG'
docs(notes): link each note to its xv6-riscv chapter counterpart

- Added a backlink from every note with an xv6 counterpart to the
  matching chapter note, so the concept side and the implementation
  side reach each other rather than being related only in the reader.

- Named xv6-riscv explicitly in each link, because these notes already
  reference xv6-public and the two trees diverged at the 2019 port.

- Warned in The_Process_Abstraction.md that its §4.5 and §6 figures
  are the older x86 tree's, at the point where a reader would compare
  them against the RISC-V source and doubt one of them.

- Created the Related Notes section in the nine notes lacking it, at
  the position templates/NOTE_TEMPLATE.md specifies, leaving every
  placeholder blockquote untouched.

- Flagged in each placeholder that its xv6 counterpart may be holding
  a concept on loan, so the material is reclaimed when that note is
  finally written rather than silently duplicated.

Cursor(%CUR): Yes
Effort: 3 SP
Co-authored-by: Claude Code (claude-opus-5)
MSG
```

- [ ] **Step 7: Final end-to-end check**

```bash
cd ../xv6-riscv
docs/book/check-notes.sh
```

Expected: `All checks passed.`

---

## Self-Review

**Spec coverage.** Ownership rule → Task 2 Step 4. Three cases → Tasks 2 and 3. Concordance → Task 3. Chapter-note shape → Task 2 Step 4, applied in Tasks 4 and 5. Backlinks → Task 6. Terminology rule → **gap, see below.** Link mechanics → Task 2 Step 4, enforced by Task 1. Chapter map correction → Task 2. Per-chapter procedure → Task 2 Step 4. Divergence handling → Task 3 appendix, applied in Task 5.

**Known gap:** the spec's Terminology section (one definition per term; `04-terminology.md` may gloss and link but never redefine) has no task. It is deliberately deferred — there is nothing to deduplicate until a chapter note introduces a term that collides, and Chapter 1 introduces none. Add it to the chapter-note procedure when the first collision appears, rather than auditing `04-terminology.md` speculatively now.

**Placeholder scan:** clean. Every step carries runnable content or exact prose. Task 5 is the one step whose output is authored rather than pasted, so it specifies required sections, required code anchors, the two forbidden moves, and two mechanical checks in place of a literal body.

**Consistency:** case vocabulary is `linked` / `loan` / `xv6-only` throughout, in the README table, the concordance status column, and `check_status`. Stub filenames in Task 4's data table match the backlink targets in Task 6 and the README links in Task 4 Step 3.
