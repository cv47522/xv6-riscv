# Prompt: draft or update a lecture companion note

---

[toc]

---

Portable, agent-neutral. Copy this whole file into any coding agent, replace the target on the first line, and run it. It assumes only a shell and file access.

---

**Target lecture:** `<REPLACE ME — e.g. "lecture 4", "l-vm.txt", or "next">`

If the target is empty, ask. `next` means the first unchecked lecture under `## Lectures` in [`STUDY-PROGRESS.md`](../../STUDY-PROGRESS.md). **Do not** infer the target from `conf/lab.mk` or recently edited files.

---

You are writing, or bringing up to date, the companion note `docs/<NN>-<topic>.md` for one 6.1810 lecture, built from three sources: the lecture text under `lectures/`, the chapter notes under `docs/book/`, and your own knowledge checked against this tree. The rule that keeps it short is ownership: **this repository keeps what is true of xv6; the sibling repositories keep what is true everywhere**, and each side links to the other instead of restating it. [`../10-c-in-xv6.md`](../10-c-in-xv6.md) is the reference result; [`../11-os-design-in-xv6.md`](../11-os-design-in-xv6.md) is the draft shape for a lecture not yet studied.

> **First, read `AGENTS.md` at the repository root**, then the `AGENTS.md` of every sibling repository you will edit (`../operating-system/`, `../c-programming/`, and `../cpp-programming/` for C++ material). Each owns its own prose, figure, and commit rules; this prompt does not repeat them.

This prompt never marks a lecture studied. [`close-lecture.md`](close-lecture.md) does that, after the learner confirms, and calls this prompt for its placement step.

## Step 1 — Gather the sources

For machine-level prerequisites, consult the [assembly companion map](../README.md#assembly-prerequisites-and-companion-notes). Link reusable instruction, calling-convention, ELF, and CSR explanations to the sibling assembly lessons; retain this tree’s exact source symbols, values, invariants, and failure paths here. A simplified RV32 preview does not establish behavior of the RV64 kernel.

| Source                                       | What to take from it                                                                                                                                                     |
| -------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| **`STUDY-PROGRESS.md`**                      | The lecture's row (date, links, checked or draft), its homework's `**Book:**` or `**Read:**` line, and its practice section if one exists                                |
| **`lectures/l-*.txt` and `lectures/<dir>/`** | Every topic, in order, and every command, file, or demo the lecture names. These are upstream bytes hashed in `lectures/manifest.json`; read them, never edit them       |
| **The assigned `docs/book/ch*.md`**          | What the chapter note already owns. If the chapter is still a scaffold, run [`sync-book-chapter.md`](sync-book-chapter.md) for it first, or record the gap in the report |
| **`book/00-ostep-concordance.md`**           | Which portable concepts are linked, on loan, or xv6-only                                                                                                                 |
| **This tree's source and build**             | Every symbol, address, size, and constant the note will state                                                                                                            |
| **An existing `docs/<NN>-*.md`**             | Update it in place rather than starting a second note. Check `docs/README.md` for the next free `<NN>` when none exists                                                  |

## Step 2 — Map each topic to its owner

Make a table of the lecture's topics, then classify each one **before** writing anything.

| The topic is...                                          | It belongs in                                          | Check first                                                       |
| -------------------------------------------------------- | ------------------------------------------------------ | ----------------------------------------------------------------- |
| **General C language**                                   | `../c-programming/C_*.md`                              | `ls ../c-programming/*.md`, then `grep -n '^#'` the likely files  |
| **C++ language**                                         | `../cpp-programming/CPP_*.md`                          | `ls ../cpp-programming/*.md`                                      |
| **A portable OS concept**                                | `../operating-system/*.md`                             | [`book/00-ostep-concordance.md`](../book/00-ostep-concordance.md) |
| **xv6's implementation of a concept**                    | the matching `docs/book/ch*.md`                        | [`book/README.md`](../book/README.md) ownership rules             |
| **Lecture-specific xv6 material with no single chapter** | the lecture companion note, `docs/<NN>-<topic>.md`     | `docs/README.md`; follow `10-c-in-xv6.md`'s shape                 |
| **A practice program**                                   | its section in [`07-exercises.md`](../07-exercises.md) | the existing section for that lecture                             |

Apply one test when placement is unclear: **would the sentence still be true on Linux?** If yes, it is generic.

## Step 3 — Write without duplicating

1. **Find the gap before writing.** The sibling repositories are large and usually already cover the generic concept. `grep -rn` them for the term and its synonyms first. Add only what is missing, typically one subsection, and say in the report why it was missing.
2. **Open the companion note with a topic map**, as both reference notes do: lecture topic, the generic note to read (with anchor), the xv6-specific section, the practice program. A topic owned by a book chapter links to that chapter's section; the companion note adds only what the lecture says beyond the chapter.
3. **Link instead of restating.** When the book note, a sibling note, and the lecture say the same thing, the owner keeps one explanation and the others link to it. Never copy a paragraph from `docs/book/` into the lecture note or back.
4. **Hold portable material on loan only when its owner is a placeholder**, with a short block that names the destination, the way [`book/README.md`](../book/README.md#how-a-chapter-note-is-written) defines loans. Move it when the sibling note is written.
5. **Measure, do not recall.** Every address, symbol, size, and constant comes from this tree: `riscv64-linux-gnu-nm -n`, `objdump -h`/`-fp`/`-r`, `grep` in `kernel/`. Where the lecture's number differs from this build (Lecture 2's `0xf6: start` versus this tree's `0xea`), give both and say why.
6. **Record divergences.** When the lecture or book describes code this tree has changed (Lecture 2 says `start()` calls `exit(0)`; `user/ulib.c` calls `exit(main(...))`), state the difference and cite the file.
7. **Use your own knowledge only where it is checkable.** Background that no source here covers is welcome in the owning note, but every claim about xv6 is verified against the source, and a claim you cannot verify is labelled as such or left out.
8. **Mark the status honestly.** A lecture not yet confirmed studied gets the `[!IMPORTANT]` draft callout that `11-os-design-in-xv6.md` carries, with the build and date the measurements came from.

## Step 4 — Place figures

A figure follows the same ownership as prose, with one difference: each repository has to render on its own, so an image may exist in two repositories when both genuinely show it. Prose never does.

| Figure                                                           | Lives in                                                                                                        |
| ---------------------------------------------------------------- | --------------------------------------------------------------------------------------------------------------- |
| **A book figure**                                                | `docs/book/fig/`, the complete mirror kept by [`sync-book-chapter.md`](sync-book-chapter.md); link it from here |
| **An xv6-specific diagram or screenshot authored for this repo** | Inline Mermaid in the note, or `docs/images/` for a raster or Excalidraw figure                                 |
| **A portable concept drawn for a sibling note**                  | `../operating-system/images/<topic>/` or `../c-programming/img/<topic>/`, following that repo's `AGENTS.md`     |
| **A book figure of a portable concept that a sibling note uses** | Copy the prepared file into the sibling's image directory; see [below](#show-book-figures-in-the-sibling-notes) |

### Show the book figures that teach the lecture

A lecture note **should** embed every book figure that makes one of its topics easier to learn, because the reader studies the lecture without the chapter open. Check the inventory in [`book/fig/README.md`](../book/fig/README.md#what-each-chapter-note-uses) against the topic map: a figure belongs when it draws a region, boundary, or path that a section of this note explains or measures.

1. **Link the shared copy**, `book/fig/<name>.svg` from `docs/`; never copy a book figure within this repository. For a TikZ figure that exists only as a redraw, link the chapter note's redraw section instead of duplicating its Mermaid.
2. **Place it in the section it illustrates**, directly before or after that section's table or diagram, not in a figures section at the top.
3. **Caption it in the three moves** from [`book/README.md`](../book/README.md#figures): the book's caption in quotes with `book figure` linked to the inventory and the source and label (`` `mem.tex` `fig:processlayout` ``), what is drawn, then what it shows **for this lecture**: which measured address, `isolate` result, or lecture claim it matches, and where this tree differs from the drawing. Write that last move fresh; do not paste the chapter note's caption.
4. **Keep alt text to one line** describing the image's shape; every fact goes in the caption.
5. **Do not restate a figure as a table, or a table as a figure.** When the note already has a measured table of the same region, the figure is the overview and the table stays the evidence; the caption names the differences between them.
6. **Spend the visual budget on distinct relationships.** One or two figures per note by default; add another only when it draws a relationship neither of the others does. A figure the note never refers to in prose is decoration and stays out.

`grep -rn 'book/fig/' docs/*.md` lists every lecture note that embeds a book figure, which is what [`sync-book-chapter.md`](sync-book-chapter.md) checks when a figure changes.

### Show book figures in the sibling notes

The sibling notes teach the portable concept, and a book figure is often the clearest picture of one real kernel doing it. For each book figure, ask which **already-written** sibling section explains the concept it draws, and embed it there, whether or not this lecture's topics led you to that section. Check every figure in [`book/fig/README.md`](../book/fig/README.md#what-each-chapter-note-uses), not only the ones this lecture uses.

| Repository                 | Image directory    | Provenance file             |
| -------------------------- | ------------------ | --------------------------- |
| **`../operating-system/`** | `images/xv6-book/` | `images/xv6-book/README.md` |
| **`../c-programming/`**    | `img/xv6_book/`    | `img/xv6_book/README.md`    |
| **`../cpp-programming/`**  | `img/xv6_book/`    | `img/xv6_book/README.md`    |

1. **Copy the prepared file unchanged** from `docs/book/fig/`, keeping its upstream file name so it stays traceable; never re-crop or re-render it in the sibling. Create the directory's provenance README if it is missing, modelled on `../operating-system/images/xv6-book/README.md`: a table of file, book caption, upstream source and label, and the notes that use it, followed by the **full** text of `../xv6-riscv-book/LICENSE`, which its terms require to travel with every copy. Add a row whenever a note starts using a new file.
2. **Embed it only in a written section** whose prose the figure illustrates. A placeholder note gets nothing; list the figure for it in the report instead.
3. **Caption it for the sibling's reader** in the three moves: the book's caption in quotes with `xv6 book`, the source and label, and a `provenance` link; what is drawn; then how it shows **this section's** concept, including where xv6 differs from the convention the section describes. Use RISC-V names only with a short gloss (`satp`, RISC-V's PDBR). Finish with one link back to the xv6 note that realizes it, in the editor-relative form `../xv6-riscv/docs/...`.
4. **Do not move xv6-specific explanation into the sibling.** The caption may name a symbol to read next, but walkthroughs of `kernel/` code stay in this repository.
5. **Edit sibling notes additively.** They often carry uncommitted work; insert the figure and caption without reflowing or reordering anything around it, and report which files already had changes.
6. Diagrams the agent draws itself follow the repository palette and the figure rules in `AGENTS.md`; the sibling repository's own conventions win inside that repository.

## Step 5 — Cross-reference both ways

- Each sibling section that gained content, or that the topic map links to, gains a one-line backlink to the xv6 note or chapter that realizes it. Use the editor-relative form `../xv6-riscv/docs/...` from a sibling root, which 404s on GitHub by design; see [`book/README.md`](../book/README.md#cross-repo-links).
- Update the concordance row when a concept's case changes, and repay any loan the new sibling content makes obsolete.
- Add new vocabulary to [`04-terminology.md`](../04-terminology.md) once and link to it.
- Register a new note in [`docs/README.md`](../README.md) and link it from the lecture row in `STUDY-PROGRESS.md` as draft notes. Do not check the box.

## Step 6 — Verify before claiming done

```bash
bash docs/book/check-notes.sh                             # links, anchors, symbols, backlinks
sha256sum lectures/l-<name>.txt                           # matches lectures/manifest.json
git status --short
git -C ../operating-system status --short
git -C ../c-programming status --short
```

Check every relative link you added by hand, especially those crossing repositories (`../../operating-system/` from `docs/`, `../xv6-riscv/docs/` from a sibling root), and every anchor they name. Confirm no new hard line breaks inside paragraphs. **Do not** commit unless the user asks; when they do, each repository gets its own commit following its own `AGENTS.md`.

Report per repository: what was added and why it was missing, what was linked instead of written, which claims were measured and how, the divergences recorded, figures copied with their provenance, and what remains open.
