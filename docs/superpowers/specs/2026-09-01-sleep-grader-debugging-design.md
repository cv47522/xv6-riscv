# Sleep grader and debugger documentation design

## Goal

Explain precisely why the focused sleep grader accepts the current missing-argument message, why its duplicate Python function names do not discard a test, how its symbolic breakpoint monitor stops QEMU, and how to reproduce the breakpoint workflow both in a terminal and in VS Code.

## Documentation ownership

`docs/labs/util-sleep.md` will own behavior specific to `grade-lab-util` and the `sleep` exercise. It will trace the test predicates, decorator registration, `stop_breakpoint('sys_pause')`, the evidence supplied by a breakpoint hit, and a focused `sys_pause` debugging session.

`docs/02-build-boot-and-usage.md` will own reusable debugger setup. It will retain the two-terminal GDB procedure and add the corresponding VS Code entry point without duplicating the sleep grader analysis.

## VS Code configuration

`.vscode/tasks.json` will provide a background task that launches `make CPUS=1 qemu-gdb` and a termination task for the matching QEMU process. The background task will identify readiness from the Makefile's GDB startup message so a debug launch can wait for QEMU without assuming that the long-running task exits.

`.vscode/launch.json` will provide a C/C++ `cppdbg` launch using `gdb-multiarch` and `kernel/kernel`. It will connect to QEMU's per-user port, use the background QEMU task as its `preLaunchTask`, and stop at `sys_pause` so the user can run `sleep 10` in the xv6 console and inspect the handler. The configuration will document its dependency on Microsoft's C/C++ extension and preserve the repository's generated `.gdbinit` as the authority for command-line GDB.

## Runtime flow

The documentation will distinguish four events: Python imports `grade-lab-util` and decorators register wrappers in `gradelib.TESTS`; the runner starts a fresh `qemu-gdb` process for each selected test; `stop_breakpoint` resolves `sys_pause` through `kernel/kernel.sym` and sends QEMU's GDB stub a breakpoint packet; and the runner treats the resulting breakpoint stop as the intended end of the test before applying output assertions.

The missing-argument explanation will state that `r.match(no=[...])` is exclusively a negative assertion. Any visible diagnostic other than the forbidden `exec ... failed` text satisfies the first test, and the second test separately establishes that the shell regains control by requiring `echo OK`.

## Verification

Validate both JSON files syntactically, verify every referenced task and launch field against the installed debugger interface and repository Makefile, run the focused sleep grader, and run `docs/book/check-notes.sh`. Inspect the Markdown changes for heading hierarchy, links, fenced blocks, and newly introduced hard-wrapped prose. Preserve all pre-existing staged changes and stage only files created or modified for this documentation task.
