# xv6 debugger symbols and execution terminology design

## Goal

Explain why this repository's VS Code session resolves a breakpoint in `kernel/sysproc.c` but leaves a breakpoint in `user/sleep.c` unresolved, contrast that behavior with the native single-program debugger setup in `/home/wahsieh/personal/c-programming/notes/.vscode`, explain when and why to use `CPUS=1`, provide a verified manual user-program debugging recipe without changing `.vscode`, and expand the repository's canonical terminology lookup.

## Scope

This work is documentation-only. It will rename and embed the two existing screenshots under `docs/images/`, create one focused debugger guide, expand the existing central glossary, and add concise navigation links from relevant documents. It will not add or modify a VS Code launch configuration, change QEMU or GDB behavior, alter xv6 source, or disclose a lab solution in `docs/labs/util-sleep.md`.

The existing uncommitted table-formatting changes in `docs/labs/util-sleep.md` belong to the user and must be preserved. Any edit to that file will be limited to a small cross-link outside those changed table lines.

## Information architecture

`docs/09-debugging-xv6.md` will own the reusable explanation. It will lead from the visible breakpoint symptom to the native-versus-remote launch model, interpret the screenshots as evidence, explain ELF and symbol ownership, provide the manual recipe, explain the `CPUS` trade-off, and end with troubleshooting guidance.

`docs/04-terminology.md` will remain the single authority for definitions. The new guide will use short contextual explanations and link to glossary entries instead of establishing a competing debugger glossary.

`docs/02-build-boot-and-usage.md` will retain the basic GDB and VS Code procedures. Its single-hart wording will be corrected so it promises fewer concurrency interleavings rather than full determinism, and it will link to the focused guide for symbol-loading and execution-model details.

`docs/labs/util-sleep.md` will keep grader and exercise-specific reasoning. Its existing debugging section will link to the focused guide's manual `sleep` recipe rather than duplicating the recipe.

`docs/README.md` will register the new guide in the reading-order table.

## Image treatment

The opaque screenshot names will be replaced with descriptive names:

| Current file | New file | Evidence it carries |
| --- | --- | --- |
| `docs/images/Code_s8L29GY56y.png` | `docs/images/vscode-kernel-breakpoint-hit.png` | VS Code is paused on the verified breakpoint in `kernel/sysproc.c`, while the `user/sleep.c` breakpoint is gray and unresolved. |
| `docs/images/Code_nHXikydY3p.png` | `docs/images/vscode-sys-pause-argument-loaded.png` | After stepping in `sys_pause`, the Variables and Call Stack views show `n = 10` and the kernel syscall path. |

Both images will appear in the new guide as an evidence sequence with descriptive alt text and captions. They will support the explanation rather than substitute for it.

## Technical explanation

The native C notes build the selected source into one host ELF containing executable code, symbols, and source-line mappings. GDB launches that ELF as the debugged host process, so a source breakpoint resolves when its line belongs to the selected binary.

The xv6 configuration starts QEMU separately and attaches GDB through QEMU's remote stub. `.gdbinit` runs `symbol-file kernel/kernel`, so GDB knows the addresses and source lines compiled into the kernel ELF. `kernel/sysproc.c` therefore resolves. `user/sleep.c` is compiled into the separate `user/_sleep` ELF, installed into `fs.img`, and loaded later by xv6 `exec`; the current GDB session never loads that ELF's symbols, so the source breakpoint remains unresolved even though the file was compiled with debug information.

The guide will use one compact Mermaid flowchart to compare these attachment paths. It will use a table for the exact roles of VS Code's `program` field, QEMU, the remote stub, `.gdbinit`, `kernel/kernel`, `user/_sleep`, and `fs.img`.

## Manual user-program recipe

The recipe will start QEMU with `make CPUS=1 qemu-gdb`, attach `gdb-multiarch` through `.gdbinit`, continue until the xv6 shell is ready, interrupt the target from GDB, load `user/_sleep` with `add-symbol-file user/_sleep 0`, set a hardware source breakpoint on `sleep.c:main`, continue, and run `sleep 10` in the QEMU console. It will show commands for confirming the selected source frame and arguments, then deleting the breakpoint and removing the user symbol file when finished.

The recipe will state its limits beside the commands. xv6 links user ELFs at virtual address zero, separate xv6 processes reuse that address range, and QEMU's GDB stub exposes emulated CPUs rather than teaching GDB xv6's process model. Loading `user/_sleep` symbols labels addresses; it does not make GDB process-aware. Installing the breakpoint after the shell is ready avoids accidentally stopping at an earlier user program's address-zero entry point, and a hardware breakpoint avoids modifying the guest text page.

The exact command sequence must be verified against the current build before it is documented. If the source-qualified function form differs from the installed GDB's accepted syntax, the verified form will replace the provisional wording above without changing the design intent.

## `CPUS=1` explanation

The guide will correct the word “auto”: this Makefile sets `CPUS := 3` unless the caller overrides it, while the `fs` lab forces one CPU. The value controls QEMU's `-smp` count and therefore the number of emulated RISC-V harts, not the number of host CPUs discovered automatically.

One hart removes cross-hart execution, lock contention, and many scheduler interleavings, which makes stepping, call stacks, console output, and QEMU monitor observations easier to follow. It does not eliminate timer interrupts or xv6 process scheduling, so it does not make execution fully deterministic. The guide will recommend one hart for tracing a causal path and the default three harts for final verification that must expose concurrency failures.

QEMU presents harts to GDB as debugger threads. The glossary and guide will explicitly distinguish those GDB threads from xv6 processes and from software threads inside an operating system.

## Terminology

The glossary update will refine `hart` from the over-broad synonym “CPU core” to RISC-V's architectural hardware-thread or logical-CPU context. It will explain that one physical core may implement one or more harts, while this QEMU invocation creates virtual CPUs that xv6 identifies as harts.

The lookup additions will cover the terms needed to reason about this session: host, guest, target or debuggee, native program, user program, kernel, user space and kernel space, process, hart, GDB thread, ELF symbol table, debug information, source breakpoint, hardware breakpoint, remote GDB stub, and address space. Existing terms such as GDB, ELF, stub, and privilege modes will be extended or cross-linked instead of duplicated.

The guide will emphasize two category boundaries. “Guest” includes both the xv6 kernel and xv6 user programs because both run inside the emulated machine. A user program is therefore guest software, but it is not synonymous with all guest software. The kernel is shared privileged code rather than an ordinary “kernel process.”

## Troubleshooting

A compact table will map observable symptoms to causes and checks. It will cover a gray or hollow source breakpoint, a hardware breakpoint that cannot be inserted, a breakpoint that hits an earlier user program at the same virtual address, locals reported as optimized out, a stale or occupied QEMU GDB port, an old `fs.img`, and behavior that appears only with multiple harts.

The table will keep failure guidance evidence-based. Each row will name what GDB or VS Code knows at that moment, the likely mismatch, and the next read-only or reversible check.

## Verification

The manual recipe will be exercised end to end against the current `user/_sleep` and `kernel/kernel` binaries. The evidence must show that the user symbol file loads, the source-qualified hardware breakpoint resolves, `sleep 10` reaches `sleep.c:main`, and cleanup succeeds.

The documentation checks will confirm that both renamed image paths exist, all new relative Markdown links resolve, internal anchors used by the new cross-links correspond to headings, code fences are balanced, and no new hard-wrapped prose was introduced. `docs/book/check-notes.sh` and `git diff --check` must pass.

The final diff review will confirm that `.vscode` is unchanged, the user's existing `docs/labs/util-sleep.md` table edits remain present, the lab note contains no new C solution block, and only the intended documentation and image paths changed.
