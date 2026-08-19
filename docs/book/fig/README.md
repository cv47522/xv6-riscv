# Book Figures

---

[toc]

---

The figures from [xv6: a simple, Unix-like teaching operating system](https://mit-pdos.github.io/xv6-riscv-book/), copied here so the chapter notes can show them. Upstream lives in `../../../../xv6-riscv-book/fig/`, which is a sibling checkout and therefore unreachable from a rendered page — hence the copy.

## Provenance and licence

The book sources are Copyright (c) 2006-2024 Russ Cox, Frans Kaashoek, and Robert Morris, Massachusetts Institute of Technology, released under an MIT-style licence that permits copying provided the notice travels with the copy. That notice is `../../../../xv6-riscv-book/LICENSE`; this file is the notice for the copies in this directory.

> [!IMPORTANT]
> These are **not** original work. Do not edit them by hand. Every one is a mechanical transform of an upstream file, so a fix belongs upstream and then comes back through [prep-figures.py](prep-figures.py).

## What each chapter note uses

| Figure                                             | Chapter                                            | Book caption                                                | Source                     |
| -------------------------------------------------- | -------------------------------------------------- | ------------------------------------------------------------ | -------------------------- |
| [os.svg](os.svg)                                   | [ch01](../ch01-operating-system-interfaces.md)     | A kernel and two user processes.                            | `unix.tex`, `fig:os`       |
| [mkernel.svg](mkernel.svg)                         | [ch02](../ch02-operating-system-organization.md)   | A microkernel with a file-system server                     | `first.tex`, `fig:mkernel` |
| [as.svg](as.svg)                                   | [ch02](../ch02-operating-system-organization.md)   | Layout of a process's virtual address space                 | `first.tex`, `fig:as`      |
| [riscv_address.svg](riscv_address.svg)             | [ch03](../ch03-page-tables.md)                     | An abstract view of a flat page table.                      | `mem.tex`                  |
| [riscv_pagetable.svg](riscv_pagetable.svg)         | [ch03](../ch03-page-tables.md)                     | RISC-V address translation details.                         | `mem.tex`                  |
| [xv6_layout.svg](xv6_layout.svg)                   | [ch03](../ch03-page-tables.md)                     | Kernel virtual and physical address spaces.                 | `mem.tex`                  |
| [processlayout.svg](processlayout.svg)             | [ch03](../ch03-page-tables.md)                     | A process's user address space, with its initial stack.     | `mem.tex`                  |
| [smp.svg](smp.svg)                                 | [ch07](../ch07-locking.md)                         | Simplified SMP architecture                                 | `lock.tex`, `fig:smp`      |
| [race.svg](race.svg)                               | [ch07](../ch07-locking.md)                         | Example race                                                | `lock.tex`, `fig:race`     |
| [inode.svg](inode.svg)                             | [ch10](../ch10-file-system.md)                     | The representation of a file on disk.                       | `fs.tex`, `fig:inode`      |

Chapters 5, 6, 11, 12, and 13 carry no figure because the book gives them none.

Two upstream figures are deliberately absent: `bufrace.svg` and `deadlock.svg`. Neither is `\includegraphics`'d by any `.tex` file, so they are leftovers rather than figures of the current edition, and copying them here would imply a place in the book they do not have.

## The figures that are redrawn instead

Six of the book's figures are TikZ (`\input{fig/*.tex}`) rather than images, and one — `fig/switch.pdf` — ships only as a PDF with no SVG beside it. Rendering any of them needs a LaTeX toolchain (`pdflatex` plus `pdf2svg`, driven by upstream's `fig/tikz-to-svg.sh`), which this environment does not have.

Rather than commit a figure nobody can regenerate, the chapter notes redraw these as Mermaid diagrams, tables, or annotated code blocks — native markdown that renders on GitHub, reads in both themes, and is editable in place:

| Upstream source     | Book caption                                                      | Redrawn in                                       | As                       |
| ------------------- | ------------------------------------------------------------------- | ------------------------------------------------ | ------------------------ |
| `fig/trap.tex`      | Outline of how a trap from user code is handled.                  | [ch04](../ch04-traps-and-system-calls.md)        | Mermaid flowchart        |
| `fig/order.tex`     | (deadlock from inverted lock order)                               | [ch07](../ch07-locking.md)                       | Side-by-side code        |
| `fig/switch.pdf`    | Switching from one user process to another.                       | [ch08](../ch08-scheduling.md)                    | Mermaid sequence diagram |
| `fig/switch.tex`    | `swtch()` always has the scheduler thread as source or destination. | [ch08](../ch08-scheduling.md)                    | Mermaid sequence diagram |
| `fig/sleep.tex`     | Overlapping locks to avoid lost wake-up                           | [ch09](../ch09-sleep-and-wakeup.md)              | Annotated code block     |
| `fig/fslayer.tex`   | Layers of the xv6 file system.                                    | [ch10](../ch10-file-system.md)                   | Mermaid flowchart        |
| `fig/fslayout.tex`  | Structure of the xv6 file system.                                 | [ch10](../ch10-file-system.md)                   | Block table              |

> [!NOTE]
> A redraw is a restatement, which is the same rule the prose notes follow. If a LaTeX toolchain ever lands here, upstream's `tikz-to-svg.sh` produces the originals and either form is defensible — but the redraws carry the colour legend the rest of these notes use, and the originals do not.

## How the copies were made

Two things had to change on the way in, and both are mechanical:

- **Crop.** The upstream SVGs are Inkscape exports on full letter or A4 canvases, so most are a small drawing in the corner of a mostly empty page. `mkernel.svg` was 744x1052 user units around a 660x210 drawing; rendered inline that is a mostly blank box taller than the screen.
- **Backdrop.** They are black line art on a transparent canvas, which disappears against GitHub's dark theme. Each copy gains an opaque white `<rect>` covering its viewBox, so the figure reads as a white plate in either theme.

[prep-figures.py](prep-figures.py) does both: it renders the file, finds the bounding box of the non-white pixels, rewrites the root `viewBox` around it with 2% padding, and inserts the backdrop. Re-run it from this directory to reproduce every file here:

```bash
python3 prep-figures.py ../../../../xv6-riscv-book/fig . \
  os.svg mkernel.svg as.svg riscv_address.svg riscv_pagetable.svg \
  xv6_layout.svg processlayout.svg smp.svg race.svg inode.svg
```

It needs `cairosvg` and `Pillow`, which nothing else in this tree uses:

```bash
pip install cairosvg Pillow
```

> [!TIP]
> `inode.svg` and `processlayout.svg` crash cairosvg outright — a zero-width marker scale trips `CAIRO_STATUS_INVALID_MATRIX`. The script strips `marker-*` properties before measuring for exactly this reason. That is a quirk of one rasteriser, not of the files; browsers render both correctly, arrowheads included.

## Conventions for citing one

The chapter notes have no figure numbers, because LaTeX assigns those at build time and this directory has no build. Cite a figure by its source file and label instead — `` `lock.tex`, `fig:race` `` — which is stable, greppable upstream, and matches how the notes cite code.

The split between alt text and caption is a rule, not a preference: alt text stays a one-line description of the image's shape, and every fact a reader needs — field names, bit widths, the addresses annotating a memory map — goes in the visible caption. A detail parked in alt text is a detail hidden from everyone whose browser loads the image. See [../README.md](../README.md#figures) for the three moves a caption makes.

Every image reference is checked by [../check-notes.sh](../check-notes.sh): `check_links` matches the inline link form wherever it appears, and an image reference is that form with a bang in front — so a renamed or deleted figure fails the same way a broken prose link does.

> [!WARNING]
> That also means the checker cannot tell a link from prose *about* a link. Writing the inline-link syntax as an inline-code example anywhere under `docs/` fails the run, because only fenced blocks are skipped. Put such examples in a fenced block.
