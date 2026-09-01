# Sleep Grader and Debugger Documentation Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Explain the util sleep grader precisely and provide working terminal and VS Code workflows for stopping at `sys_pause`.

**Architecture:** Keep grader-specific reasoning in `docs/labs/util-sleep.md` and reusable debugger setup in `docs/02-build-boot-and-usage.md`. Add repository-local VS Code tasks that run and stop the existing QEMU GDB target, while the launch configuration delegates the dynamic port and RISC-V setup to the Makefile-generated `.gdbinit`.

**Tech Stack:** Python grader decorators and monitors, QEMU's remote GDB stub, GDB/MI through `gdb-multiarch`, VS Code tasks 2.0, Microsoft C/C++ `cppdbg`, JSON, and Markdown.

---

## File map

- Create `.vscode/tasks.json`: start single-hart QEMU as a background task and stop the QEMU instance bound to this user's generated GDB port.
- Create `.vscode/launch.json`: start `cppdbg`, explicitly source the generated `.gdbinit`, install a `sys_pause` breakpoint, and connect the launch lifecycle to the QEMU tasks.
- Modify `docs/labs/util-sleep.md`: explain the three assertions, duplicate decorated names, breakpoint-monitor internals, and the focused sleep debugging recipe.
- Modify `docs/02-build-boot-and-usage.md`: explain the reusable two-terminal and VS Code workflows, including generated-port and safe-path behavior.

### Task 1: Add the VS Code QEMU and GDB configuration

**Files:**

- Create: `.vscode/tasks.json`
- Create: `.vscode/launch.json`

- [x] **Step 1: Confirm the configuration does not already exist**

Run:

```bash
test ! -e .vscode/tasks.json && test ! -e .vscode/launch.json
```

Expected: exit status 0 and no output. If either file appears after this plan was written, inspect and merge it instead of overwriting it.

- [x] **Step 2: Create the QEMU tasks**

Create `.vscode/tasks.json` with this content:

```json
{
    "version": "2.0.0",
    "tasks": [
        {
            "label": "xv6: start QEMU for GDB",
            "type": "shell",
            "command": "make",
            "args": ["CPUS=1", "qemu-gdb"],
            "options": { "cwd": "${workspaceFolder}" },
            "isBackground": true,
            "problemMatcher": {
                "owner": "xv6",
                "pattern": {
                    "regexp": "^(.*?):([0-9]+):([0-9]+): (warning|error): (.*)$",
                    "file": 1,
                    "line": 2,
                    "column": 3,
                    "severity": 4,
                    "message": 5
                },
                "background": {
                    "activeOnStart": true,
                    "beginsPattern": ".",
                    "endsPattern": "^\\*\\*\\* Now run 'gdb' in another window\\.$"
                }
            },
            "presentation": {
                "reveal": "always",
                "panel": "dedicated",
                "focus": true
            }
        },
        {
            "label": "xv6: stop QEMU GDB",
            "type": "shell",
            "command": "pkill -TERM -f \"qemu-system-riscv64.*-gdb tcp::$(make -s print-gdbport)\" || true",
            "options": { "cwd": "${workspaceFolder}" },
            "problemMatcher": []
        }
    ]
}
```

- [x] **Step 3: Create the GDB launch configuration**

Create `.vscode/launch.json` with this content:

```json
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "xv6: debug sys_pause",
            "type": "cppdbg",
            "request": "launch",
            "program": "${workspaceFolder}/kernel/kernel",
            "cwd": "${workspaceFolder}",
            "MIMode": "gdb",
            "miDebuggerPath": "gdb-multiarch",
            "miDebuggerArgs": "-x ${workspaceFolder}/.gdbinit",
            "customLaunchSetupCommands": [],
            "setupCommands": [
                {
                    "description": "Break when the sleep command reaches its kernel handler",
                    "text": "break sys_pause",
                    "ignoreFailures": false
                }
            ],
            "launchCompleteCommand": "None",
            "stopAtConnect": true,
            "hardwareBreakpoints": { "require": true },
            "preLaunchTask": "xv6: start QEMU for GDB",
            "postDebugTask": "xv6: stop QEMU GDB"
        }
    ]
}
```

The explicit `-x` matters: it sources the repository-generated `.gdbinit` even when GDB refuses to auto-load an untrusted local init file. `customLaunchSetupCommands: []` and `launchCompleteCommand: "None"` prevent `cppdbg` from trying to launch `kernel/kernel` as a host process or issuing `run` against a remote target.

- [x] **Step 4: Parse both JSON files**

Run:

```bash
python3 -m json.tool .vscode/tasks.json >/dev/null
python3 -m json.tool .vscode/launch.json >/dev/null
```

Expected: both commands exit 0 with no output.

- [x] **Step 5: Confirm every cross-file task reference resolves**

Run:

```bash
python3 - <<'PY'
import json

with open('.vscode/tasks.json') as f:
    tasks = json.load(f)['tasks']
with open('.vscode/launch.json') as f:
    configs = json.load(f)['configurations']

labels = {task['label'] for task in tasks}
for config in configs:
    assert config['preLaunchTask'] in labels
    assert config['postDebugTask'] in labels
assert configs[0]['program'].endswith('/kernel/kernel')
assert configs[0]['miDebuggerArgs'].endswith('/.gdbinit')
print('VS Code task references: OK')
PY
```

Expected: `VS Code task references: OK`.

- [x] **Step 6: Commit only the VS Code configuration**

Run:

```bash
git add .vscode/tasks.json .vscode/launch.json
git commit --only .vscode/tasks.json .vscode/launch.json -m "chore(debugging): configure xv6 GDB session" -m "- Added a single-hart QEMU background task for predictable debugging.

- Connected cppdbg through the generated per-user GDB configuration.

- Added scoped QEMU cleanup after each VS Code debug session.

Cursor(%CUR): Yes
Effort: 1 SP"
```

Expected: one commit containing only the two `.vscode` files; the user's pre-existing staged changes remain staged.

### Task 2: Explain what the sleep grader actually asserts

**Files:**

- Modify: `docs/labs/util-sleep.md:24-34`
- Modify: `docs/labs/util-sleep.md:162-177`

- [x] **Step 1: Expand the test table with exact predicate behavior**

Update the table under “What the exercise asks for” so it states that `r.match(no=[...])` supplies only forbidden regular expressions, that `"Usage: sleep ticks"` matches neither forbidden expression, and that the second test's positive `^OK$` assertion is what proves control returned to the shell.

- [x] **Step 2: Add the decorator-registration explanation**

Immediately after the table, add a “Why two functions can have the same name” subsection with this execution model:

```text
@test(...) constructs register_test.
register_test(original_function) constructs run_test.
TESTS.append(run_test) preserves that wrapper in registration order.
return run_test binds the module name test_sleep_no_args.
The second def replaces only that module name; it does not remove the first wrapper from TESTS.
run_tests() iterates TESTS, so both wrappers execute and retain distinct decorator titles.
```

State that the duplicate name is legal but misleading: failure-log naming uses `get_current_test().__name__[5:]`, so both failures target `xv6.out.sleep_no_args`, and the second can overwrite the first one's saved transcript.

- [x] **Step 3: Add the `stop_breakpoint` control flow**

Add a subsection near “When the grader and shell disagree” that traces these exact operations:

1. `Runner.run_qemu()` converts target base `qemu` to `qemu-gdb`, starts QEMU halted, and connects its small `GDBClient` to the port returned by `make print-gdbport`.
2. `shell_script()` waits for each `$ ` prompt and writes `sleep 10`, then `echo FAIL`.
3. `stop_breakpoint('sys_pause')` scans `kernel/kernel.sym`, selects the address whose symbol text equals `sys_pause`, and calls `GDBClient.breakpoint()`.
4. `breakpoint()` sends remote packet `Z1,<address>,1`, which requests a hardware breakpoint; `cont()` sends `c`.
5. QEMU replies with a stop packet beginning `T05` when execution reaches the breakpoint. `GDBClient.handle_read()` raises `TerminateTest`, which ends the reactor loop without marking the test failed.
6. The `finally` block terminates QEMU, and `r.match('\\$ sleep 10', no=['FAIL'])` checks the captured console output.

Include an `IMPORTANT` callout stating that the breakpoint is the success event, not an exception treated as failure. Explain that absence of `FAIL` follows because QEMU is stopped and destroyed before `sys_pause` returns, not because the grader waited ten ticks.

- [x] **Step 4: Verify all grader claims against their sources**

Run:

```bash
rg -n "def test_sleep_no_args|r\.match|def test\(|TESTS\.append|def stop_breakpoint|Z1,|T05|def shell_script" grade-lab-util gradelib.py docs/labs/util-sleep.md
```

Expected: every named behavior appears in `grade-lab-util` or `gradelib.py`, and every source concept appears in the updated note.

- [x] **Step 5: Commit only the grader explanation**

Run:

```bash
git add docs/labs/util-sleep.md
git commit --only docs/labs/util-sleep.md -m "docs(labs): explain sleep grader control flow" -m "- Clarified why any visible no-argument diagnostic satisfies the test.

- Explained decorator registration despite duplicate Python function names.

- Traced the remote breakpoint packet that ends the syscall test.

Cursor(%CUR): Yes
Effort: 1 SP"
```

Expected: one documentation commit while unrelated staged files remain staged.

### Task 3: Document console and VS Code debugging

**Files:**

- Modify: `docs/02-build-boot-and-usage.md:281-318`
- Modify: `docs/labs/util-sleep.md:179-201`

- [x] **Step 1: Make the general terminal workflow complete**

In `docs/02-build-boot-and-usage.md`, retain the two-terminal structure and update it to use `make CPUS=1 qemu-gdb`. Explain that QEMU starts halted by `-S`, exposes its remote stub through the Makefile's `-gdb tcp::<port>`, and the generated `.gdbinit` supplies `target remote`, architecture, symbol file, and compressed-breakpoint settings.

Add these reusable commands:

```gdb
(gdb) break sys_pause
(gdb) continue
(gdb) info breakpoints
(gdb) next
(gdb) print n
(gdb) info registers a0 a7
(gdb) backtrace
(gdb) detach
(gdb) quit
```

Clarify that a local variable such as `n` is not initialized at function entry; stop after `argint(0, &n)` before printing it. State that `Ctrl-C` interrupts the emulated target while focus is in GDB, whereas `Ctrl-a x` exits QEMU while focus is in its terminal.

- [x] **Step 2: Correct and generalize the safe-path guidance**

Replace the home-directory-specific example with a placeholder absolute workspace path and state that current GDB may name either `~/.config/gdb/gdbinit` or `~/.gdbinit` in its diagnostic. Preserve the rule that only trusted repositories should be added to `auto-load safe-path`.

- [x] **Step 3: Add the reusable VS Code workflow**

Add a “Debugging with VS Code” subsection that requires `ms-vscode.cpptools` and `gdb-multiarch`, then gives these actions:

1. Open the repository root in VS Code.
2. Choose “xv6: debug sys_pause” in Run and Debug and press F5.
3. Wait for the dedicated QEMU terminal, then Continue once from the reset-vector stop so xv6 reaches its shell.
4. Enter `sleep 10` in the QEMU terminal.
5. Inspect `n`, registers, stack frames, and source when VS Code stops in `sys_pause`.
6. Stop debugging so `postDebugTask` terminates the QEMU process.

Explain that `launch.json` explicitly sources `.gdbinit` to consume the generated per-user port, while the terminal workflow can rely on trusted auto-loading.

- [x] **Step 4: Replace the sleep note's one-sentence GDB recipe**

In `docs/labs/util-sleep.md`, add an exact console session that breaks at `sys_pause`, continues to the shell, receives `sleep 10`, uses `next` to execute `argint(0, &n)`, and prints `n`. Follow it with a link to the general VS Code subsection rather than duplicating the VS Code steps.

- [x] **Step 5: Verify documentation structure and references**

Run:

```bash
rg -n "Debugging with gdb|Debugging with VS Code|sys_pause|stop_breakpoint|test_sleep_no_args|xv6: debug sys_pause" docs/02-build-boot-and-usage.md docs/labs/util-sleep.md .vscode/launch.json
```

Expected: the reusable workflows occur in `docs/02-build-boot-and-usage.md`; grader behavior and the focused session occur in `docs/labs/util-sleep.md`; the launch name matches exactly.

- [x] **Step 6: Check the lab-note policy and Markdown invariants**

Run:

````bash
docs/book/check-notes.sh
python3 - <<'PY'
from pathlib import Path

for path in [Path('docs/02-build-boot-and-usage.md'), Path('docs/labs/util-sleep.md')]:
    text = path.read_text()
    assert text.count('```') % 2 == 0, path
print('Markdown structural checks: OK')
PY
````

Expected: the note checker passes and the Python check prints `Markdown structural checks: OK`.

- [x] **Step 7: Commit only the debugger documentation**

Run:

```bash
git add docs/02-build-boot-and-usage.md docs/labs/util-sleep.md
git commit --only docs/02-build-boot-and-usage.md docs/labs/util-sleep.md -m "docs(debugging): add xv6 GDB workflows" -m "- Documented terminal commands for inspecting syscall handler state.

- Added a VS Code walkthrough backed by repository debug tasks.

- Clarified generated ports, init loading, and debugger termination.

Cursor(%CUR): Yes
Effort: 1 SP"
```

Expected: one documentation commit containing only the two intended files.

### Task 4: Run end-to-end verification

**Files:**

- Verify: `.vscode/tasks.json`
- Verify: `.vscode/launch.json`
- Verify: `docs/02-build-boot-and-usage.md`
- Verify: `docs/labs/util-sleep.md`

- [x] **Step 1: Confirm the generated GDB configuration connects**

In terminal 1, run:

```bash
make CPUS=1 qemu-gdb
```

In terminal 2, run:

```bash
gdb-multiarch --batch -x .gdbinit kernel/kernel -ex "info target" -ex "detach"
```

Expected: GDB reports a remote RISC-V target using `kernel/kernel`; QEMU remains available until exited with `Ctrl-a x`.

- [x] **Step 2: Run the focused grader**

After stopping every interactive QEMU instance, run:

```bash
./grade-lab-util sleep
```

Expected:

```text
== Test sleep, no arguments == sleep, no arguments: OK
== Test sleep, returns == sleep, returns: OK
== Test sleep, makes syscall == sleep, makes syscall: OK
```

Elapsed-time suffixes may differ.

- [x] **Step 3: Review only the intended final diff and index state**

Run:

```bash
git status --short
git diff --check
git diff --cached --check
git log -4 --oneline
```

Expected: no whitespace errors; the new commits contain only this task's files; any staged changes that predated the task are still staged unless the same documentation files were deliberately incorporated by `--only` commits.
