# xv6-riscv Study Notes

---

[toc]

---

Personal notes and guides for learning [xv6-riscv](https://github.com/mit-pdos/xv6-riscv) alongside the [xv6 book](https://mit-pdos.github.io/xv6-riscv-book/) and MIT's [6.1810](https://pdos.csail.mit.edu/6.1810/2026/) labs.

Use the [self-study progress tracker](../STUDY-PROGRESS.md) for the full course checklist, dated completion evidence, and links to homework, labs, and exams. The [lecture archive](../lectures/README.md) holds the published texts and original example code for later practice under `user/`.

## Setup reading order and guide lookup

**Start with guides 00–03 in order; each assumes the previous guide.** Use the later guides as topic references.

| Setup guide                                              | What to do or learn                                                                                                    |
| -------------------------------------------------------- | ---------------------------------------------------------------------------------------------------------------------- |
| [00-repo-workflow.md](00-repo-workflow.md)               | Understand remotes, branches, tags, and where work lives; find exact sync, push, and recovery commands.                |
| [01-environment-setup.md](01-environment-setup.md)       | Install the toolchain and understand each `apt` package.                                                               |
| [02-build-boot-and-usage.md](02-build-boot-and-usage.md) | Run `make qemu`, read boot output, use the shell, and debug with GDB, `kernel.asm`, `addr2line`, and the QEMU monitor. |
| [03-lab-workflow.md](03-lab-workflow.md)                 | Select `conf/lab.mk`, run `make grade`, merge lab branches, and approach an exercise.                                  |

### Find a terminology, build, debugging, or lecture guide

| Reference guide                                      | Main topics                                                                                                                                   |
| ---------------------------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------- |
| [04-terminology.md](04-terminology.md)               | Glossary expanding xv6, RISC-V, QEMU, hart, PLIC, ISO C, POSIX, GNU, and the remaining acronyms.                                              |
| [05-syscall-reference.md](05-syscall-reference.md)   | All 22 calls by group: signature, return value, failure mode, and implementing file. xv6's substitute for man pages; consult as needed.       |
| [06-build-artifacts.md](06-build-artifacts.md)       | File extensions, generated artifacts, ELF, assembly, libraries, and the guest/host filesystem boundary.                                       |
| [07-exercises.md](07-exercises.md)                   | Each lecture program's behavior, teaching point, and regression tests; grows as exercises are added.                                          |
| [08-makefile-tour.md](08-makefile-tour.md)           | The [Makefile](../Makefile): default goal, dependency graph, cold build, link steps, constructs, and comparison with a simpler project.       |
| [09-debugging-xv6.md](09-debugging-xv6.md)           | Native versus xv6 breakpoints, ELF symbol loading, manual user-program debugging, and tracing with one hart before verification with three.   |
| [10-c-in-xv6.md](10-c-in-xv6.md)                     | Lecture 2 and Homework 2: user memory, binary tools, arguments, kernel lists, flags, and struct-address calculations.                         |
| [11-os-design-in-xv6.md](11-os-design-in-xv6.md)     | Lecture 3 draft, not yet studied: isolation, syscall dispatch, boot, process exhaustion, and hostile ELF validation. Homework 3 remains open. |
| [12-os-overview-in-xv6.md](12-os-overview-in-xv6.md) | Lecture 1 topic map, four OS goals, syscall steps, alternative designs, and lecture differences. Homework 1 has no recorded answer.           |

For a specific mechanism, use these guide details:

- **Build artifacts (06):** `.o`, `.d`, `.asm`, `.sym`, `.S`, `.pl`, and `.ld` contents and generation; why a kernel needs these file types; every instruction and register used in `.S`; ELF and `0x80000000`; `ULIB` versus glibc; and how `fs.img`, `mkfs`, `kernel/`, and `user/` map onto Linux.
- **Makefile comparison (08):** the final comparison uses `operating-system/codes/Makefile`; [C_Project_Build_Tool.md](../../c-programming/C_Project_Build_Tool.md) explains its syntax.
- **C in xv6 (10):** real user-space addresses, `objdump` and `nm` output, `argv` on the stack, two kernel linked lists, `fcntl.h` bit flags, and the worked struct-address answer. Generic C stays in `../c-programming/`.
- **OS design (11):** user/kernel enforcement, a syscall as a number, argument extraction and safe user-memory copies, kernel traps and timer preemption, `_entry` through `/init` to the shell, `fork` with a full process table, and `exec` checks on a hostile ELF.
- **OS overview (12):** a map into ch01 and the exercises, each lecture goal's kernel location, syscall steps pinned to symbols, the cost of the lecture's "other designs", and qualifications where lecture text differs from this tree.

### Find book notes, lab derivations, and reusable prompts

| Collection                                                   | Purpose                                                                                                                                                                                                       |
| ------------------------------------------------------------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| [book/](book/)                                               | Chapter-by-chapter xv6 book notes, added as chapters are read.                                                                                                                                                |
| [labs/](labs/)                                               | MIT exercise contracts, source-reading paths, and derivation decisions. Includes `util` exercises such as `sleep` and optional shell extensions; completed solutions stay behind marked spoiler walkthroughs. |
| [book/00-ostep-concordance.md](book/00-ostep-concordance.md) | One concept per row across the xv6 book, OSTEP notes in `../operating-system/`, this tree's source, and exercises. Read before writing a chapter note.                                                        |
| [prompts/](prompts/)                                         | Reusable, agent-neutral prompts for lecture exercises and lab derivations. Copy into a coding agent or invoke a generated command mirror.                                                                     |

> [!TIP]
> The glossary is the hub. Rather than re-explaining a term in each guide, the other documents link into [04-terminology.md](04-terminology.md). When a chapter introduces a new term, add it there once and link to it.

## Assembly prerequisites and companion notes

**Keep the sibling repositories side by side:** cross-repo relative links work in a local checkout or editor and are not expected to resolve on separate repositories' web pages.

The [assembly-language course](../../assembly-language/README.md) owns reusable instruction, ABI, tool, and privilege explanations. These notes own this xv6 tree's implementation, addresses, build rules, and failure paths.

| Topic                          | Practice in the assembly repo                                                                                                                                                                | Apply it in this tree                                                                                                                                                                                                        |
| ------------------------------ | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **Registers and instructions** | [Values and register aliases](../../assembly-language/docs/riscv/01-values-registers-instructions.md), [branches and jumps](../../assembly-language/docs/riscv/03-control-flow.md)           | [The assembly files' instruction and register reference](06-build-artifacts.md#reading-the-assembly-files)                                                                                                                   |
| **Memory and pointers**        | [Loads, widths, and byte order](../../assembly-language/docs/riscv/02-memory-and-pointers.md)                                                                                                | [C and memory](10-c-in-xv6.md), [the memdump derivation](labs/03-util-memdump.md)                                                                                                                                            |
| **Calls and stacks**           | [Calling convention and nested calls](../../assembly-language/docs/riscv/04-functions-and-stacks.md)                                                                                         | [How exec supplies argc and argv](10-c-in-xv6.md#argv-and-the-calling-convention)                                                                                                                                            |
| **ELF and tools**              | [C, ELF, linker symbols, and disassembly](../../assembly-language/docs/riscv/05-c-elf-and-disassembly.md), [QEMU/GDB practice](../../assembly-language/docs/riscv/00-tools-and-first-run.md) | [Build artifacts](06-build-artifacts.md), [debugging xv6](09-debugging-xv6.md)                                                                                                                                               |
| **Privilege and saved state**  | [CSRs and privilege](../../assembly-language/docs/riscv/06-csrs-and-privilege.md), [allocation, mapping, and saved state](../../assembly-language/docs/riscv/07-memory-and-saved-state.md)   | [Boot overview](11-os-design-in-xv6.md#kernel-boot-from-entry-to-the-first-user-process), [boot-study proposal](boot/README.md), [trap-path concept loan](labs/01-util-sleep.md#the-foundation-a-c-call-that-becomes-a-trap) |

The assembly course includes simplified RV32 CPUlator previews as well as RV64 QEMU examples. Its [readiness lesson](../../assembly-language/docs/riscv/08-readiness-and-next-steps.md) identifies what remains for the kernel study; preview results do not establish this tree's runtime behavior.

## Repository layout: kernel, programs, graders, and notes

This is a single consolidated repository. It holds the upstream xv6 source, these notes, MIT's grading harness, and lab implementations — all on one branch, so nothing requires a branch switch to read or edit.

```
xv6-riscv/
  kernel/          the xv6 kernel
  user/            user programs (the contents of fs.img)
  mkfs/            host-side tool that builds the filesystem image
  conf/lab.mk      selects the active lab (LAB=util)
  grade-lab-*      MIT grading scripts
  gradelib.py      the grading harness they run on
  docs/            these notes
  lectures/        downloaded MIT lecture texts and original example code
  STUDY-PROGRESS.md course checklist and completion evidence
  docs/prompts/    reusable task prompts for coding agents
  Makefile         the build, heavily commented as a teaching aid
                   (walked through in docs/08-makefile-tour.md)
```

Branches and remotes are covered in [00-repo-workflow.md](00-repo-workflow.md). The short version: `riscv` is an untouched mirror of upstream, and `study` is where all work happens.

## Write a book chapter note: ownership and source evidence

**Fill an existing stub:** all thirteen chapter files already exist under `book/`, one per xv6 book chapter.

1. **Check existing coverage** in [book/00-ostep-concordance.md](book/00-ostep-concordance.md). Each concept maps to an OSTEP note, xv6 chapter, source symbols, and exercise, so check what is already explained.
2. **Follow the authoritative procedure** in [book/README.md](book/README.md): its three-case table, note shape, and per-case blockquote variant. Keep that procedure in one place.
3. **Link implementation evidence** using source references such as `kernel/vm.c:57`, clickable in most editors. Add new vocabulary to the glossary rather than defining it inline.

**`../operating-system/` owns concepts; `docs/book/` owns xv6's realization of them.** If an OSTEP note is not written, its concept stays on loan in the xv6 note. The concordance records that status until the owning concept note exists.
