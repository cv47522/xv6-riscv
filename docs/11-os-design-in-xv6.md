# OS Design in xv6

---

[toc]

---

**How does xv6 isolate programs it must assume are malicious?** This companion to Lecture 3, [_OS design_](../lectures/l-os.txt), follows this tree's code, measured addresses, and two practice programs that attack the boundary and record what refuses them.

## xv6 isolation, system calls, and boot at a glance

- **Isolation has two defences:** hardware blocks forbidden instructions and memory accesses; kernel code checks system-call numbers and user-memory copies.
- **Mapped does not mean user-accessible:** the trampoline and trapframe appear in every user page table, but their mappings lack `PTE_U`.
- **Extracting an address does not validate it:** `argaddr()` retrieves a number; the copy helpers translate and check user memory when it is accessed.
- **The first process starts through the return path:** `userinit()` creates its slot, `forkret()` loads `/init`, and the trap-return machinery enters U-mode.
- **Failures expose different limits:** a full process table makes `fork()` return `-1`; a rejected `exec()` leaves the old image intact. xv6 has no per-user process limit.

### Concept ownership: canonical explanations

Use this ownership map to find the explanation you need, following the [book notes rule](book/README.md):

| Explanation                                                   | Owning note                                                                                                                                                   |
| ------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **Portable user/kernel mode, traps, and system-call numbers** | [The Process Abstraction](../../operating-system/The_Process_Abstraction.md#user-mode-vs-kernel-mode).                                                        |
| **Stub-to-`sys_*` dispatch**                                  | [05-syscall-reference.md](05-syscall-reference.md#system-call-path-declaration-stub-dispatch-implementation).                                                 |
| **`ecall` and `a7`**                                          | [06-build-artifacts.md](06-build-artifacts.md#why-the-system-call-number-goes-in-a7).                                                                         |
| **Argument extraction and safe user-memory copies**           | [System-call arguments and user memory](#system-call-arguments-and-user-memory), on loan to ch04.                                                             |
| **Traps while already in the kernel**                         | [Traps from kernel space](#kernel-space-traps-dispatch-preemption-and-return), on loan to ch04.                                                               |
| **The user trap path**                                        | [Lecture 1’s step-by-step flow](12-os-overview-in-xv6.md#what-a-system-call-does-step-by-step) and [ch04](book/ch04-traps-and-system-calls.md), when written. |
| **Lecture 3's additions in this tree**                        | This note: where the boundary is enforced, how the first process crosses it, and how `fork` and `exec` handle hostile input.                                  |

> [!IMPORTANT]
> **Status: draft study notes.** Lecture 3 is not yet marked studied in [STUDY-PROGRESS.md](../STUDY-PROGRESS.md#lectures), and [Homework 3](#homework-3-a-full-process-table-versus-exec-fork-and-kill) was answered on 2026-11-03. Measurements below are from this tree's `LAB=util` build on 2026-10-28, except the Homework 3 run, measured on 2026-11-03.

## Lecture 3 topics and practice programs

| Lecture topic                            | Generic concept (read when needed)                                                                                                                                                                                | xv6-specific, in this note                                                                 | Practice                                |
| ---------------------------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------ | --------------------------------------- |
| **Isolation as the design goal**         | [Design Goals](../../operating-system/Introduction_to_Operating_Systems.md#5-design-goals-25)                                                                                                                     | [Where the boundary is enforced](#program-isolation-hardware-faults-and-kernel-checks)     | [`user/isolate.c`](../user/isolate.c)   |
| **Threat model, TCB**                    | — (held here on loan)                                                                                                                                                                                             | [Kernel organization and trust](#kernel-organization-threat-model-and-trusted-code)        | —                                       |
| **User/supervisor mode**                 | [User Mode vs. Kernel Mode](../../operating-system/The_Process_Abstraction.md#user-mode-vs-kernel-mode)                                                                                                           | [Where the boundary is enforced](#program-isolation-hardware-faults-and-kernel-checks)     | [`isolate csr`](../user/isolate.c)      |
| **Virtual memory as isolation**          | [Address Spaces and Translation](../../operating-system/Address_Spaces_And_Translation.md)                                                                                                                        | [Where the boundary is enforced](#program-isolation-hardware-faults-and-kernel-checks)     | [`isolate kload`](../user/isolate.c)    |
| **`ecall` and system-call numbers**      | [Trap / Return-from-trap](../../operating-system/The_Process_Abstraction.md#trap--return-from-trap), [05-syscall-reference.md](05-syscall-reference.md#system-call-path-declaration-stub-dispatch-implementation) | [A system call is a number](#system-call-dispatch-numbers-handlers-and-invalid-requests)   | [`isolate badsys`](../user/isolate.c)   |
| **Monolithic kernel versus microkernel** | — (held here on loan; Lecture 12 reads about microkernels)                                                                                                                                                        | [Kernel organization and trust](#kernel-organization-threat-model-and-trusted-code)        | —                                       |
| **The board, QEMU, and emulation**       | —                                                                                                                                                                                                                 | [The machine xv6 runs on](#qemu-machine-configuration-and-physical-memory-map)             | —                                       |
| **Kernel source and the build**          | [08-makefile-tour.md](08-makefile-tour.md), [06-build-artifacts.md](06-build-artifacts.md)                                                                                                                        | [The machine xv6 runs on](#qemu-machine-configuration-and-physical-memory-map)             | —                                       |
| **Boot to `/init`**                      | [`init` / PID 1](../../operating-system/The_Process_Abstraction.md#init--pid-1)                                                                                                                                   | [From boot to the first process](#kernel-boot-from-entry-to-the-first-user-process)        | —                                       |
| **`fork`: `allocproc`, `uvmcopy`**       | [`fork()`](../../operating-system/The_Process_Abstraction.md#71-fork--create-a-near-copy-51)                                                                                                                      | [`fork`, `kfork`, and a full table](#fork-and-kfork-process-creation-and-table-exhaustion) | [`user/forkfill.c`](../user/forkfill.c) |
| **`exec`: ELF, checks, stack**           | [`exec()`](../../operating-system/The_Process_Abstraction.md#73-exec--become-a-different-program-53), [What ELF is](06-build-artifacts.md#elf-headers-segments-and-load-addresses)                                | [`exec` treats the file as hostile](#exec-hostile-elf-checks-and-failure-cleanup)          | [`isolate kcopy`](../user/isolate.c)    |

## Program isolation: hardware faults and kernel checks

The lecture names two defences: hardware controls on user instructions, and a careful system-call interface. [`isolate`](../user/isolate.c) tries one attack per mode, and each is refused at a different layer:

| Attempt (`isolate <mode>`)                         | Layer that refuses it                                               | Observed result                                            |
| -------------------------------------------------- | ------------------------------------------------------------------- | ---------------------------------------------------------- |
| **`csr`**: run `r_sstatus()` from `kernel/riscv.h` | CPU privilege check                                                 | `scause 0x2` (illegal instruction); `usertrap()` kills it  |
| **`kload`**: load a byte from `TRAMPOLINE`         | MMU: the page is mapped but lacks `PTE_U`                           | `scause 0xd` (load page fault); `usertrap()` kills it      |
| **`badsys`**: `ecall` with `a7` = 99               | `syscall()` bounds-checks `a7` against `syscalls[]`                 | Kernel prints `unknown sys call 99`; the call returns `-1` |
| **`kcopy`**: `read()` from a pipe into `TRAPFRAME` | `copyout()` calls `walkaddr()`, which rejects a PTE without `PTE_U` | `read()` returns `-1`; the two pipe bytes are not consumed |

The results divide into the lecture's two defences:

- **Hardware faults (`csr`, `kload`):** hardware stops the instruction. `usertrap()` in `kernel/trap.c` has no applicable handler for these cases and calls `setkilled()`.
- **Kernel checks (`badsys`, `kcopy`):** these attempts **never** fault. Kernel code detects the invalid request and returns an ordinary error.

> [!NOTE]
> `TRAMPOLINE` (`0x3FFFFFF000`) and `TRAPFRAME` (`0x3FFFFFE000`) are **mapped in every user page table**. `proc_pagetable()` in `kernel/proc.c` maps them with `PTE_R | PTE_X` and `PTE_R | PTE_W` but never `PTE_U`, so isolation here is a permission bit, not a missing mapping.

![A tall column divided into labelled bands, spanning one process's virtual address space](book/fig/as.svg)

_“Layout of a process's virtual address space” — [book figure](book/fig/README.md), `first.tex` `fig:as`; also captioned in [ch02](book/ch02-operating-system-organization.md#figures)._

Read the bands **bottom to top**, starting at address 0:

1. User text and data.
2. User stack.
3. A large heap region.
4. Trapframe, then trampoline: two thin bands immediately below `MAXVA`.

The last two bands are the targets of `isolate kcopy` and `isolate kload`, respectively. They are inside the user's own virtual address space, but neither mapping carries `PTE_U`: **mapped does not mean user-accessible**. The attack table above records the resulting refusals.

Why `kcopy` matters:

- **Protected state:** the trapframe holds this process's saved registers, including `epc`, the user instruction address that the kernel will resume. Without `walkaddr()`'s `PTE_U` test in `kernel/vm.c`, a `read()` into that page could corrupt the saved return state.
- **Failure preserves the pipe bytes:** `piperead()` in `kernel/pipe.c` returns `-1` before advancing `nread`. A following `read()` into a real buffer therefore still gets `"hi"`.

## System-call dispatch: numbers, handlers, and invalid requests

Lecture 3's observation: a user call to `fork()` reaches the kernel through a **number**, rather than an ordinary function call into kernel code.

1. The stub in `user/usys.S` executes `li a7, SYS_fork`, then `ecall`.
2. `syscall()` in `kernel/syscall.c` uses saved `a7` to index `syscalls[]`.

The full mechanics belong to [05-syscall-reference.md](05-syscall-reference.md#system-call-path-declaration-stub-dispatch-implementation). The numbers below come from this tree's `kernel/syscall.h`:

| Call       | Number | Kernel entry | Does the work                                    |
| ---------- | ------ | ------------ | ------------------------------------------------ |
| **`fork`** | 1      | `sys_fork()` | `kfork()` in `kernel/proc.c`                     |
| **`exec`** | 7      | `sys_exec()` | `kexec()` in `kernel/exec.c`                     |
| **`sync`** | 22     | `sys_sync()` | the highest number in the table; 99 has no entry |

| Dispatch condition                                  | Result                                                                        |
| --------------------------------------------------- | ----------------------------------------------------------------------------- |
| `num > 0 && num < NELEM(syscalls) && syscalls[num]` | Call the registered handler.                                                  |
| **Any other value**                                 | Run `printk("%d %s: unknown sys call %d\n", ...)` and set saved `a0` to `-1`. |

The second row produces `5 isolate: unknown sys call 99` in `isolate badsys`. Each generated `usys.S` stub hard-codes its own `a7`, so `rawsyscall()` uses a hand-written `ecall` to try an unused number.

> [!WARNING]
> The lecture's naming convention, `k<syscallname>()`, holds only for process calls. This tree has `kfork()`, `kexec()`, `kexit()`, `kwait()`, and `kkill()`; file calls go from `sys_read()` to `fileread()`, not to a `kread()`. The full rename table is in [the concordance appendix](book/00-ostep-concordance.md#system-call-naming).

The lecture also suggests editing `syscall()` to print a trace of every system call `init` and `sh` make. That edit is the syscall lab's `trace` exercise, so it is deliberately not done here.

## System-call arguments and user memory

| Flow                                          | Canonical explanation                                                                                                                                                                       |
| --------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **Entry, dispatch, and return to user space** | [Lecture 1’s symbol-by-symbol flow](12-os-overview-in-xv6.md#what-a-system-call-does-step-by-step); [`a7` and stub rationale](06-build-artifacts.md#why-the-system-call-number-goes-in-a7). |
| **Extract arguments and validate an fd**      | The sequence below, verified against [`kernel/syscall.c`](../kernel/syscall.c) and [`kernel/sysfile.c`](../kernel/sysfile.c).                                                               |
| **Follow a user pointer safely**              | [The string-copy flow](#following-a-user-string-with-copyinstr), verified against [`kernel/vm.c`](../kernel/vm.c).                                                                          |

> [!NOTE]
> **On loan to [Chapter 4](book/ch04-traps-and-system-calls.md).** This section owns the current argument/copy walkthrough; move it into that chapter when written and leave section links here. Book §4.4 describes the mechanism; the signatures and lazy-page branches below come from this tree.

**Diagram legend for this page:** phase names, actions, and branch labels carry the same meaning without color.

| Color                  | Meaning                |
| ---------------------- | ---------------------- |
| **Pink** (`#fff0f0`)   | Request                |
| **Blue** (`#f0f0ff`)   | Decision or routing    |
| **Green** (`#f0fff0`)  | Data movement          |
| **Yellow** (`#fffff0`) | Processing             |
| **Cyan** (`#f0ffff`)   | Stored state or commit |
| **Red** (`#ffd9d9`)    | Stall or failure       |

M-mode, S-mode, and U-mode mean machine, supervisor, and user mode; see the [architecture glossary](04-terminology.md#risc-v-architecture).

```mermaid
sequenceDiagram
    participant U as User C / user/usys.S
    participant T as trampoline.S / trap.c
    participant D as syscall.c
    participant W as sysfile.c: sys_write
    rect rgb(255, 240, 240)
        Note over U,T: 1. The caller supplies arguments and the stub selects write.
        U->>U: The caller sets a0=fd, a1=buf, a2=n.
        U->>T: The stub sets a7=SYS_write and executes ecall.
    end
    rect rgb(240, 240, 255)
        Note over T,W: 2. Software saves registers and selects the handler.
        T->>T: uservec saves registers, switches sp/satp,<br/>and calls usertrap().
        T->>D: usertrap() saves epc and advances it by 4,<br/>then enables interrupts and calls syscall().
        D->>W: syscalls[trapframe.a7] selects sys_write().
    end
    rect rgb(255, 255, 240)
        Note over D,W: 3. Helpers extract arguments and argfd also validates the fd.
        W->>D: argaddr(1, &p) reads saved a1 through argraw(1).
        D-->>W: p is a numeric user address, not a checked kernel pointer.
        W->>D: argint(2, &n) reads saved a2 and converts it to int.
        W->>W: argfd(0) uses argint(0), then checks ofile[fd].
        alt The fd is out of range or unopened.
            W-->>D: sys_write() returns -1.
        else The fd names an open file.
            W->>W: filewrite(f, p, n) selects the object<br/>and copies user data.
            W-->>D: sys_write() returns the filewrite result.
        end
    end
    rect rgb(240, 255, 255)
        Note over U,D: 4. The result returns through the saved a0.
        D-->>T: syscall() stores the result in trapframe.a0.
        T-->>U: prepare_return() and userret restore user state.<br/>sret precedes the stub ret.
    end
```

### Argument helpers: extraction versus validation

**An address argument is still an unchecked number after extraction.** The helpers in [`kernel/syscall.c`](../kernel/syscall.c) read the saved registers; `argfd()` adds descriptor validation in [`kernel/sysfile.c`](../kernel/sysfile.c).

| Helper      | Reads or converts                                     | Validation and result                                           |
| ----------- | ----------------------------------------------------- | --------------------------------------------------------------- |
| `argraw(n)` | Saved `a0` through `a5`, for indices `0` through `5`. | Unsupported index: kernel programming error, `panic("argraw")`. |
| `argint()`  | Narrows the saved value to a 32-bit `int`.            | Returns `void` here.                                            |
| `argaddr()` | Keeps a 64-bit address.                               | Does not dereference or validate it; returns `void` here.       |
| `argfd()`   | Calls `argint()` to extract the descriptor.           | Rejects `fd < 0`, `fd >= NOFILE`, or an empty `p->ofile[fd]`.   |

`argfd()` is local to `kernel/sysfile.c`. It returns `-1` on failure or `0` after filling any requested descriptor/file output arguments.

**Why the registers are available:** `uservec` saves them in memory; the hardware trap does not. It first saves user `a0` via `sscratch`, then uses `a0` as its trapframe pointer.

### When `write` actually copies the buffer

**Extracting `buf` does not copy its contents.** [`filewrite()`](../kernel/file.c) routes the write according to its destination:

| Destination        | Copy path                                                                   |
| ------------------ | --------------------------------------------------------------------------- |
| **Console device** | [`kernel/console.c`](../kernel/console.c) → `either_copyin()`.              |
| **Inode**          | [`kernel/fs.c`](../kernel/fs.c) → `either_copyin()`.                        |
| **Pipe**           | [`pipewrite()` in `kernel/pipe.c`](../kernel/pipe.c) → `copyin()` directly. |

With a user source, `either_copyin()` in [`kernel/proc.c`](../kernel/proc.c) selects `copyin()`. These helpers translate the user address before reading its contents; an ordinary dereference of `buf` would use the kernel's current page table.

### Following a user string with `copyinstr`

| Caller in `sys_exec()`  | Path through `kernel/syscall.c`                                                                                             | Destination                              |
| ----------------------- | --------------------------------------------------------------------------------------------------------------------------- | ---------------------------------------- |
| **Executable pathname** | `argstr(0, path, MAXPATH)` → `argaddr(0)` → `fetchstr()` → `copyinstr()`                                                    | Kernel `path[MAXPATH]`.                  |
| **An argument string**  | `fetchaddr()` first copies one pointer from user `argv` using `copyin()`; `fetchstr(uarg, argv[i], PGSIZE)` → `copyinstr()` | A kernel page allocated for that string. |

```mermaid
---
config:
  flowchart:
    wrappingWidth: 320
---
flowchart TD
    subgraph request["1. syscall.c requests a bounded string copy."]
        A["fetchstr() calls copyinstr(pagetable, psz, dst, srcva, max)."]
    end
    subgraph translate["2. vm.c translates one user page."]
        B["copyinstr() rounds srcva down to va0."]
        C{"Does walkaddr(pagetable, va0) return a physical address?"}
        D["walkaddr() uses walk(..., 0) and checks MAXVA, PTE_V, and PTE_U."]
        E{"Can vmfault(pagetable, psz, va0, 1) supply a lazy page?"}
    end
    subgraph transfer["3. vm.c copies through the kernel's RAM mapping."]
        F["Read at pa0 + (srcva - va0); copy at most this page's remainder or max."]
        G{"Was a NUL copied?"}
        H{"Are bytes left in max?"}
        I["Advance srcva to the next page."]
        J["copyinstr() returns 0; fetchstr() returns strlen(buf)."]
        X["copyinstr() and fetchstr() return -1."]
    end
    A -->|max is positive| B
    A -->|max is 0| X
    B --> D --> C
    C -->|Yes| F
    C -->|No| E
    E -->|Yes| F
    E -->|No| X
    F --> G
    G -->|Yes| J
    G -->|No| H
    H -->|Yes| I --> B
    H -->|No| X
    classDef request fill:#fff0f0,stroke:#aa6666,color:#111
    classDef routing fill:#f0f0ff,stroke:#6666aa,color:#111
    classDef movement fill:#f0fff0,stroke:#669966,color:#111
    classDef processing fill:#fffff0,stroke:#999944,color:#111
    classDef commit fill:#f0ffff,stroke:#449999,color:#111
    classDef failure fill:#ffd9d9,stroke:#aa5555,color:#111
    class A request
    class C,E,G,H routing
    class F movement
    class B,D,I processing
    class J commit
    class X failure
    style request fill:#fff0f0,stroke:#888,color:#111
    style translate fill:#f0f0ff,stroke:#888,color:#111
    style transfer fill:#f0fff0,stroke:#888,color:#111
```

#### User address translation: checks and kernel access

**The kernel walks the process's page table in software.** The `pagetable` argument is that data structure; the active `satp` still selects the kernel's address space.

| Check                                                           | Performed by                                                                       |
| --------------------------------------------------------------- | ---------------------------------------------------------------------------------- |
| **Address below `MAXVA`; entry present and valid; `PTE_U` set** | `walkaddr()` in [`kernel/vm.c`](../kernel/vm.c).                                   |
| **Original eight-byte pointer slot inside `p->sz`**             | `fetchaddr()`, with separate start/end checks to catch overflow.                   |
| **Writable destination (`PTE_W`)**                              | `copyout()`, after translation or lazy allocation; it also rejects `va0 >= MAXVA`. |
| **Process-size limit or `PTE_R` permission**                    | Neither is checked by `walkaddr()` itself.                                         |

`kvmmake()` in [`kernel/vm.c`](../kernel/vm.c) establishes the kernel's identity mapping of RAM. That makes **the translated physical address `pa0` plus offset** reachable by ordinary kernel loads. Loading from the original user virtual address would use the wrong address space.

#### `vmfault()`: lazy allocation during user-memory copies

**A missing user page may be allocated; a supervisor-only mapping cannot be converted into user memory.** This revision adds `psz` to the copy helpers and a lazy-allocation branch absent from the supplied §4.4 outline.

| Failed lookup in            | Retry             |
| --------------------------- | ----------------- |
| `copyinstr()` or `copyin()` | `vmfault(..., 1)` |
| `copyout()`                 | `vmfault(..., 0)` |

`vmfault()` follows this order:

1. Reject a page address at or beyond `psz`, or a page that is already mapped.
2. Allocate a zeroed page and map it `PTE_R | PTE_W | PTE_U`.
3. Return the physical address on success, or `0` on failure.

The first check prevents an existing supervisor-only trapframe mapping from becoming user memory. The helper's `read` parameter is currently unused.

#### User-memory copy contracts and partial failures

| Helper in `kernel/vm.c` | Direction                    | Successful stopping rule                                          | Result                                                                          |
| ----------------------- | ---------------------------- | ----------------------------------------------------------------- | ------------------------------------------------------------------------------- |
| `copyinstr()`           | User → kernel, across pages. | Copy a NUL before exhausting `max`; `max` includes space for NUL. | `0` after NUL; `-1` on failed translation/allocation or exhaustion without NUL. |
| `copyin()`              | User → kernel, across pages. | Copy exactly `len` bytes.                                         | `0` on completion; `-1` on failed lookup/lazy allocation.                       |
| `copyout()`             | Kernel → user, across pages. | Copy exactly `len` bytes to writable pages.                       | `0` on completion; `-1` on failure.                                             |

> [!WARNING]
> **A failed copy can leave a copied prefix.** Earlier pages or bytes are not rolled back when a later part fails.

**`sys_exec()` cleanup:** a failed copy returns an error and frees any allocated argument pages. It calls `kexec()` only after argument preparation succeeds; the later ELF checks are [below](#exec-hostile-elf-checks-and-failure-cleanup).

## Kernel-space traps: dispatch, preemption, and return

> [!NOTE]
> **On loan to [Chapter 4](book/ch04-traps-and-system-calls.md).** This is the canonical current walkthrough of book §4.5, checked against [`kernel/kernelvec.S`](../kernel/kernelvec.S), [`kernel/trap.c`](../kernel/trap.c), and [`yield()` / `sched()`](../kernel/proc.c). User-mode trap entry remains in [Lecture 1’s flow](12-os-overview-in-xv6.md#what-a-system-call-does-step-by-step).

```mermaid
---
config:
  flowchart:
    wrappingWidth: 320
---
flowchart TD
    subgraph entry["1. A trap interrupts S-mode kernel execution."]
        A["The hart records sepc, scause, and SPP; clears SIE; and jumps to kernelvec."]
        B["kernelvec reserves 256 bytes on the current kernel stack and saves registers."]
        C["kerneltrap() snapshots sepc, sstatus, and scause."]
    end
    subgraph dispatch["2. kernel/trap.c validates and handles the cause."]
        D{"Is SPP set and are interrupts disabled?"}
        E["devintr() services the interrupt and returns which_dev."]
        F{"What did devintr() return?"}
        G{"Does myproc() return a process?"}
        H["yield() marks it RUNNABLE; sched() switches to the scheduler."]
        I["The scheduler later resumes this same thread inside yield()."]
        P["panic() prints the invariant failure and never returns."]
        Q["kerneltrap() prints scause, sepc, and stval, then panics."]
    end
    subgraph resume["3. The interrupted kernel thread resumes."]
        J["kerneltrap() restores its saved sepc and sstatus, then returns."]
        K["kernelvec restores registers and sp, leaving tp unchanged."]
        L["sret resumes the interrupted kernel instruction in S-mode."]
    end
    A --> B --> C --> D
    D -->|No| P
    D -->|Yes| E --> F
    F -->|0: unrecognized trap| Q
    F -->|1: external device| J
    F -->|2: timer| G
    G -->|No: scheduler context| J
    G -->|Yes| H --> I --> J
    J --> K --> L
    classDef routing fill:#f0f0ff,stroke:#6666aa,color:#111
    classDef processing fill:#fffff0,stroke:#999944,color:#111
    classDef movement fill:#f0fff0,stroke:#669966,color:#111
    classDef commit fill:#f0ffff,stroke:#449999,color:#111
    classDef failure fill:#ffd9d9,stroke:#aa5555,color:#111
    class A,D,F,G,H routing
    class B,C,E processing
    class I movement
    class J,K,L commit
    class P,Q failure
    style entry fill:#f0f0ff,stroke:#888,color:#111
    style dispatch fill:#fffff0,stroke:#888,color:#111
    style resume fill:#f0ffff,stroke:#888,color:#111
```

### `kernelvec`: entry state and saved registers

**No trampoline switch is needed:** `satp` already selects the kernel page table, and `sp` names a valid kernel stack, either a process's stack or the scheduler's boot stack. Hardware switches neither one.

| Register or group                | What `kernelvec` preserves                                           | Why it works                                             |
| -------------------------------- | -------------------------------------------------------------------- | -------------------------------------------------------- |
| `ra`, `gp`, `t0`–`t6`, `a0`–`a7` | Saves them in a 256-byte stack frame.                                | They must survive the C trap handler.                    |
| `sp`                             | Reserves 256 bytes, then adds 256 to restore it; no saved `sp` slot. | The stack frame stays on the interrupted thread's stack. |
| `tp`                             | Neither saved nor restored.                                          | Keeps the current hart id after possible CPU migration.  |
| `s0`–`s11`                       | Preserved by the C calling convention.                               | The ABI makes these registers callee-saved.              |

This frame differs from the 31-register user trapframe. Compare the [saved-state primer](../../assembly-language/docs/riscv/07-memory-and-saved-state.md#saved-state-stack-frame-context-and-trapframe).

### Timer preemption: preserving the interrupted thread's stack

**Running another thread does not replace this thread's trap frame.** A timer interrupt can suspend the original thread along this path:

1. `yield()` switches through `sched()` to the scheduler; the `kerneltrap()` and `kernelvec` frame remain on the original thread's stack.
2. The scheduler may run other threads, whose trap returns use their own stacks.
3. When selected again, the original thread resumes through `sched()` and `yield()`, releases `p->lock`, and returns along its own trap path.

The detailed context-switch and lock-handoff diagrams remain canonical in [Chapter 8](book/ch08-scheduling.md#figures).

### `kerneltrap()`: which interrupts return and which traps panic

**`kerneltrap()` validates its entry state before dispatching.** Either violated invariant causes a panic:

| Invariant                            | Required value     |
| ------------------------------------ | ------------------ |
| **Trap came from supervisor mode**   | `sstatus.SPP != 0` |
| **Interrupts are disabled on entry** | `intr_get() == 0`  |

After those checks, `devintr()` determines the path:

| `devintr()` result | Cause and work                                                           | What happens next                                                       |
| ------------------ | ------------------------------------------------------------------------ | ----------------------------------------------------------------------- |
| `1`                | Supervisor external interrupt through the PLIC; services UART or virtio. | Returns along the interrupted trap path.                                |
| `2`                | Supervisor timer interrupt; runs `clockintr()`.                          | Calls `yield()` if `myproc() != 0`, then eventually returns.            |
| `0`                | Unrecognized cause, including kernel exceptions.                         | Prints `scause`, `sepc`, and `stval`, then calls `panic("kerneltrap")`. |

For PLIC interrupts, the handler also reports an unexpected nonzero IRQ and completes a nonzero claim. The [panic reference](06-build-artifacts.md#panic-invariant-failures-output-and-hart-shutdown) owns panic output and halt behavior.

**Kernel exceptions have no recovery branch here.** Unlike `usertrap()`, this handler does not resolve lazy page faults or kill just one process.

### Trap CSR restoration after timer preemption

**Timer preemption condition:** `which_dev == 2 && myproc() != 0`. This revision has no separate `p->state == RUNNING` test.

Other execution while this thread is suspended can overwrite the hart's trap CSRs. Restoring the snapshot before returning to `kernelvec` lets `sret` resume the interrupted kernel code with its previous interrupt state:

| CSR       | Treatment                                                             |
| --------- | --------------------------------------------------------------------- |
| `sepc`    | Restored to the interrupted instruction address.                      |
| `sstatus` | Restored to the saved status, including the previous interrupt state. |
| `scause`  | Used for diagnosis and dispatch; not restored.                        |

> [!IMPORTANT]
> **Switch `stvec` before enabling interrupts.** Device interrupts must use the trap vector appropriate to the current execution mode.

**User → kernel ordering:**

1. Hardware clears `sstatus.SIE` before entering `uservec`.
2. `usertrap()` writes `stvec = kernelvec`.
3. It saves `sepc` and inspects `scause` before its system-call branch calls `intr_on()`.

**Kernel → user ordering:** `prepare_return()` disables interrupts before pointing `stvec` back at `uservec`. During boot, `trapinithart()` installs `kernelvec`.

The first ordering protects the interval when S-mode has begun but `stvec` still names `uservec`. **Scope of protection:** `SIE` masks device interrupts, not synchronous exceptions.

## Kernel organization: threat model and trusted code

> [!NOTE]
> **Concepts on loan:** the OSTEP notes cover user/kernel mode but do not yet own the threat model or kernel organization. These two concepts stay here until an owning note exists; Lecture 12 reads about microkernels.

| Assumption      | Lecture 3's position                                                 | Consequence in this tree                                                                                     |
| --------------- | -------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------ |
| **User code**   | Actively malicious: tries to break isolation and trick system calls. | Every user pointer goes through `copyin()`/`copyout()`; `a7` is bounds-checked; `kexec()` validates the ELF. |
| **Kernel code** | Trusted, written by competent people, but still buggy.               | The whole of `kernel/` is the trusted computing base (TCB); any bug there may be an exploit.                 |

| Organization    | What runs in supervisor mode                                 | Trade-off the lecture names                                                                                                |
| --------------- | ------------------------------------------------------------ | -------------------------------------------------------------------------------------------------------------------------- |
| **Monolithic**  | All system calls, the file system, and drivers: one program. | Subsystems cooperate easily (one cache for file system and VM), but interactions are complex and drivers are not isolated. |
| **Microkernel** | A small kernel; services run as user processes.              | Less code in the TCB limits damage from bugs; services talk by message instead of by function call.                        |

![Two ellipses in user space above a red dividing line, both connected down to a thin box in kernel space](book/fig/mkernel.svg)

_“A microkernel with a file-system server” — [book figure](book/fig/README.md), `first.tex` `fig:mkernel`._

| Figure feature                                   | How to read it                                                                                    |
| ------------------------------------------------ | ------------------------------------------------------------------------------------------------- |
| **`shell` and `File server` above the red line** | Both are user processes. The file system runs outside the privileged kernel.                      |
| **Thin `Microkernel` box below the line**        | The kernel mediates communication between those processes.                                        |
| **Connection labelled “Send message”**           | The connection dips through the microkernel instead of calling the file system inside the kernel. |

**Schematic `read` exchange: two messages.** The microkernel row above corresponds to:

1. Shell → kernel → file server: request.
2. File server → kernel → shell: reply.

The figure does not specify every message a real file-system protocol might require. Compare [Lecture 1's figure](12-os-overview-in-xv6.md#what-a-system-call-does-step-by-step), where the request is a single system-call arrow into one large kernel.

**The file server still matters to security.** Moving it to user space removes it from the privileged kernel's TCB and limits its authority, but file-system integrity can still depend on trusting that service.

- **xv6's choice:** every file in `kernel/` links into one `kernel/kernel` binary. It is monolithic, and its user-facing kernel interface is the system-call interface.
- **The lecture's alternative:** a strongly typed language, such as Singularity's, can provide process isolation without hardware modes. xv6 does not take that approach.

## QEMU machine configuration and physical memory map

The lecture models the board as a 4-core SiFive HiFive Unleashed with 128 MB of RAM, a UART, and disk. This tree's `QEMUOPTS` in the [Makefile](../Makefile) is the authority for what QEMU actually emulates:

| Property          | Lecture 3                                  | This tree                                                                           |
| ----------------- | ------------------------------------------ | ----------------------------------------------------------------------------------- |
| **Machine**       | `qemu -machine virt`                       | `-machine virt -bios none`                                                          |
| **Cores**         | 4                                          | `CPUS := 3` by default; 1 under `LAB=fs`, 4 under `LAB=lock`                        |
| **RAM**           | 128 MB                                     | `-m 128M`                                                                           |
| **Kernel entry**  | the kernel's first instruction             | `_entry` at `0x80000000`, the ELF entry point (`readelf -h kernel/kernel`)          |
| **Disk**          | virtio instead of the board's storage      | `fs.img`, 2,048,000 bytes, attached as a virtio block device                        |
| **Kernel binary** | `ls -l kernel/kernel`                      | 276,264 bytes                                                                       |
| **Kernel source** | small enough to understand by semester end | 23 `.c` files, 4 `.S` files, 7,041 lines including headers (`wc -l kernel/*.[chS]`) |

The lecture's emulation sketch is a useful model for each emulated hart:

1. Read the next instruction.
2. Decode it.
3. Execute its effect on state such as a `uint64 reg[32]` register array.

The real QEMU does not interpret one instruction at a time; it translates blocks of RISC-V code into x86 code that runs on the host CPU. See [How QEMU runs xv6 on an x86 host](#how-qemu-runs-xv6-on-an-x86-host-tcg-not-a-board).

**`-bios none` omits separate firmware; QEMU's reset stub still runs.** The stub transfers to `_entry` in machine mode.

`_entry` is the kernel's first instruction; GDB attached at reset may see the stub first. Why the kernel is linked there is in [06-build-artifacts.md](06-build-artifacts.md#why-the-kernel-is-linked-at-0x80000000).

![Two tall columns, kernel virtual on the left and physical on the right, joined by dotted lines](book/fig/xv6_layout.svg)

_“On the left, xv6's kernel virtual address space; on the right, the RISC-V physical address space that xv6 expects to see.” — [book figure](book/fig/README.md), `mem.tex` `fig:xv6_layout`; also captioned in [ch03](book/ch03-page-tables.md#figures)._

Read the devices and RAM **bottom to top**:

| Region                                      | Address / placement                                                       | Connection to this build                                                                                                  |
| ------------------------------------------- | ------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------- |
| **PLIC**                                    | `0x0C000000`                                                              | QEMU `virt` interrupt controller.                                                                                         |
| **UART0**                                   | `0x10000000`                                                              | Console device registers.                                                                                                 |
| **Virtio disk**                             | `0x10001000`                                                              | `fs.img` is reached through device registers, not as RAM contents.                                                        |
| **RAM: kernel text, data, and free memory** | `KERNBASE` (`0x80000000`) through the exclusive `PHYSTOP` (`0x88000000`). | `_entry` begins the kernel text band. `-m 128M` matches `PHYSTOP = KERNBASE + 128 * 1024 * 1024` in `kernel/memlayout.h`. |
| **High kernel virtual mappings**            | Trampoline and kernel stacks at the top of the virtual column.            | Unmapped guard pages separate kernel stacks.                                                                              |

The two columns distinguish virtual placement from the physical machine map; high virtual mappings do not add physical RAM above `PHYSTOP`.

### How QEMU runs xv6 on an x86 host: TCG, not a board

**Yes: QEMU uses the host CPU, and no RISC-V chip exists anywhere in this setup.** The `virt` board, its RISC-V harts, RAM, UART, PLIC, timer, and virtio disk are all C code and data inside one Linux process.

```mermaid
flowchart TD
    W["Windows runs the Hyper-V hypervisor on the Intel Core Ultra 7 165H."]
    L["Hyper-V runs the WSL2 Linux kernel in a VM; its x86 instructions execute directly on the CPU through VT-x."]
    Q["qemu-system-riscv64 is an ordinary x86 Linux process with one host thread per emulated hart."]
    T["TCG translates each block of RISC-V guest code into x86 host code, caches it, and runs it."]
    X["xv6 runs in M-, S-, and U-mode on the emulated virt board and sees no hypervisor."]
    W --> L --> Q --> T --> X
    classDef processing fill:#fffff0,stroke:#999944,color:#111
    classDef routing fill:#f0f0ff,stroke:#6666aa,color:#111
    classDef movement fill:#f0fff0,stroke:#669966,color:#111
    class W,L routing
    class Q,T processing
    class X movement
```

**Two different mechanisms sit in this stack.** The lower layers use hardware virtualization, and the upper layer uses emulation:

| Layer               | Mechanism                                                                                                        | Why                                                                                                        |
| ------------------- | ---------------------------------------------------------------------------------------------------------------- | ---------------------------------------------------------------------------------------------------------- |
| **WSL2 on Windows** | Hardware virtualization: VT-x runs guest x86 instructions natively, and EPT translates guest physical addresses. | Guest and host share one ISA, so no translation is needed. `lscpu` reports `Hypervisor vendor: Microsoft`. |
| **xv6 inside QEMU** | Emulation with **TCG**, the **T**iny **C**ode **G**enerator: dynamic binary translation from RISC-V to x86.      | The ISAs differ, so the CPU cannot execute guest instructions. KVM would require a RISC-V host.            |

**Evidence from this machine** (QEMU 8.2.2):

```text
$ qemu-system-riscv64 -accel help
Accelerators supported in QEMU binary:
tcg
$ qemu-system-riscv64 -machine virt -accel kvm ...
qemu-system-riscv64: -accel kvm: invalid accelerator kvm
```

`/dev/kvm` exists in this WSL2 VM because Hyper-V exposes nested VT-x (the `vmx` flag below). It could accelerate an x86 guest, but not this RISC-V one. Counting QEMU's threads while xv6 boots showed 4 threads with `-smp 1` and 6 with `-smp 3`: each added hart adds one host thread, so the 3 harts can run in parallel on 3 of the 16 vCPUs.

**What emulation means for what you observe:**

- **Correct behaviour, not real timing.** TCG reproduces RISC-V instruction semantics, privilege modes, and Sv39 translation in software. It does not model pipelines, caches, or real instruction timing.
- **Time follows the host clock.** The `virt` board's timer runs at a nominal 10 MHz derived from the host clock, so `stimecmp` ticks are wall-clock intervals, not counted cycles.
- **Devices are software.** The UART is QEMU's 16550A model, and the "disk" is reads and writes on the host file `fs.img`.

#### xv6 under QEMU versus FreeRTOS on an ESP32

**An ESP32 runs firmware on real silicon; xv6 here runs on a simulated board.** The comparison:

| Property                | FreeRTOS on an ESP32                                                                                                 | xv6 in this repository                                                             |
| ----------------------- | -------------------------------------------------------------------------------------------------------------------- | ---------------------------------------------------------------------------------- |
| **Where code executes** | The chip's own core: Xtensa on the original ESP32 and S3, 32-bit RISC-V on the C3 and C6.                            | Host x86 code generated by TCG from RV64GC kernel code.                            |
| **How code gets there** | ESP-IDF cross-compiles firmware, and it is flashed to the board's SPI flash.                                         | `make qemu` passes `-kernel kernel/kernel`; QEMU copies the ELF into emulated RAM. |
| **Peripherals**         | Real GPIO, Wi-Fi, timers, and UART hardware.                                                                         | QEMU's device models: 16550A UART, PLIC, CLINT timer, virtio disk.                 |
| **Timing**              | Real cycles and interrupt latency; suitable for timing-sensitive control.                                            | Host-dependent; useful for correctness, not for timing measurements.               |
| **Isolation model**     | Tasks share one address space; no virtual memory or per-task privilege split. Protection needs an optional MPU port. | Per-process Sv39 page tables and U-mode/S-mode separation.                         |
| **Debugging**           | JTAG or the USB debug interface, reaching the physical chip.                                                         | QEMU's GDB stub; see [09-debugging-xv6.md](09-debugging-xv6.md).                   |

Even the RISC-V ESP32 variants could not boot this xv6: they are 32-bit cores without S-mode or an MMU, and xv6 needs RV64 with Sv39 paging. Espressif also publishes a QEMU fork that emulates some ESP32 chips, which is the same idea as `virt` applied to their boards.

### WSL2 host CPU compared with xv6's emulated machine

**The host is a 16-vCPU x86-64 VM; the guest is a 3-hart RV64 board with 128 MiB of RAM.** `lscpu` from the WSL2 host:

<details>
<summary>Full <code>lscpu</code> output from the WSL2 host</summary>

```text
❯ lscpu
Architecture:             x86_64
  CPU op-mode(s):         32-bit, 64-bit
  Address sizes:          46 bits physical, 48 bits virtual
  Byte Order:             Little Endian
CPU(s):                   16
  On-line CPU(s) list:    0-15
Vendor ID:                GenuineIntel
  Model name:             Intel(R) Core(TM) Ultra 7 165H
    CPU family:           6
    Model:                170
    Thread(s) per core:   2
    Core(s) per socket:   8
    Socket(s):            1
    Stepping:             4
    BogoMIPS:             6144.00
    Flags:                fpu vme de pse tsc msr pae mce cx8 apic sep mtrr pge mca cmov pat pse36 clflush mmx fxsr sse s
                          se2 ss ht syscall nx pdpe1gb rdtscp lm constant_tsc rep_good nopl xtopology tsc_reliable nonst
                          op_tsc cpuid pni pclmulqdq vmx ssse3 fma cx16 pcid sse4_1 sse4_2 x2apic movbe popcnt tsc_deadl
                          ine_timer aes xsave avx f16c rdrand hypervisor lahf_lm abm 3dnowprefetch invpcid_single ssbd i
                          brs ibpb stibp ibrs_enhanced tpr_shadow vnmi ept vpid ept_ad fsgsbase tsc_adjust bmi1 avx2 sme
                          p bmi2 erms invpcid rdseed adx smap clflushopt clwb sha_ni xsaveopt xsavec xgetbv1 xsaves avx_
                          vnni umip waitpkg gfni vaes vpclmulqdq rdpid movdiri movdir64b fsrm md_clear serialize flush_l
                          1d arch_capabilities
Virtualization features:
  Virtualization:         VT-x
  Hypervisor vendor:      Microsoft
  Virtualization type:    full
Caches (sum of all):
  L1d:                    384 KiB (8 instances)
  L1i:                    512 KiB (8 instances)
  L2:                     16 MiB (8 instances)
  L3:                     24 MiB (1 instance)
Vulnerabilities:
  Gather data sampling:   Not affected
  Itlb multihit:          Not affected
  L1tf:                   Not affected
  Mds:                    Not affected
  Meltdown:               Not affected
  Mmio stale data:        Not affected
  Reg file data sampling: Not affected
  Retbleed:               Mitigation; Enhanced IBRS
  Spec rstack overflow:   Not affected
  Spec store bypass:      Mitigation; Speculative Store Bypass disabled via prctl and seccomp
  Spectre v1:             Mitigation; usercopy/swapgs barriers and __user pointer sanitization
  Spectre v2:             Mitigation; Enhanced / Automatic IBRS; IBPB conditional; RSB filling; PBRSB-eIBRS SW sequence;
                           BHI BHI_DIS_S
  Srbds:                  Not affected
  Tsx async abort:        Not affected
```

</details>

The 16 CPUs are the WSL2 VM's vCPUs, capped by `processors=16` in `%UserProfile%\.wslconfig`; Hyper-V presents them as 8 cores × 2 threads, which is not the physical chip's topology. `free -h` inside WSL2 reports 31 GiB, matching `memory=32GB` in the same file.

| Property                        | WSL2 host (`lscpu`)                                            | xv6 guest on QEMU `virt`                                                                               |
| ------------------------------- | -------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------ |
| **ISA**                         | `x86_64`                                                       | RV64GC (`-march=rv64gc` in the Makefile)                                                               |
| **CPUs**                        | 16 vCPUs                                                       | 3 harts (`-smp 3`), each one QEMU host thread; `NCPU` = 8 is xv6's limit                               |
| **Byte order**                  | Little endian                                                  | Little endian                                                                                          |
| **Virtual addresses**           | 48 bits, four-level paging                                     | Sv39: 39 bits, three levels; xv6 uses only up to `MAXVA` = `1L << 38`                                  |
| **Physical memory**             | 46-bit addresses; 31 GiB RAM visible                           | `-m 128M`: `KERNBASE` `0x80000000` to `PHYSTOP` `0x88000000`                                           |
| **Privilege levels**            | Rings 0 and 3, plus VMX root and non-root for the hypervisor   | M-mode, S-mode, U-mode                                                                                 |
| **Hypervisor**                  | `Hypervisor vendor: Microsoft`; the `hypervisor` flag is set   | None visible; xv6 assumes bare metal                                                                   |
| **System-call instruction**     | `syscall` flag                                                 | `ecall`                                                                                                |
| **No-execute pages**            | `nx` flag                                                      | A PTE without `PTE_X`                                                                                  |
| **Kernel access to user pages** | `smap`: the kernel faults on user pages unless it opts in      | `sstatus.SUM` stays clear, so `copyin()` and `copyout()` translate through the user page table instead |
| **Address-space tags**          | `pcid`                                                         | The `satp` ASID field; `MAKE_SATP` leaves it 0, so xv6 flushes with `sfence.vma`                       |
| **Timer**                       | `tsc_deadline_timer`                                           | `stimecmp` from the Sstc extension                                                                     |
| **Interrupt controller**        | `apic`, `x2apic`                                               | PLIC for devices, CLINT for timers and software interrupts                                             |
| **Caches**                      | L1d 384 KiB, L1i 512 KiB, L2 16 MiB, L3 24 MiB                 | Not modelled; guest memory accesses go through the host's caches                                       |
| **Speculative-execution bugs**  | Mitigated (`Spectre v1`, `Spectre v2`, `Retbleed`, and others) | xv6 has no mitigations, and TCG does not reproduce RISC-V speculation; see `lectures/l-meltdown.txt`   |

**Read the table as two machines, not one.** Only the left column is hardware; the right column is what QEMU presents to xv6. A host-CPU feature such as `avx2` or `smap` never reaches xv6, and a guest feature such as Sv39 or `ecall` exists only in QEMU's software model.

## Kernel boot: from entry to the first user process

> [!TIP]
> **Assembly new to you?** Start with the [assembly course](../../assembly-language/README.md), then its [readiness checks](../../assembly-language/docs/riscv/08-readiness-and-next-steps.md). This section is an overview, not a line-by-line tutorial. The [boot-study proposal](boot/README.md) specifies the missing lessons and source coverage.

**Boot path:** `_entry` → kernel `start()` → `main()` → `userinit()` → `scheduler()` → `forkret()` → `/init`. Lecture 3 starts its trace at `main()`; this tree's path begins two steps earlier, in machine mode.

M-mode means machine mode, S-mode supervisor mode, and U-mode user mode. A **hart** is a hardware thread with its own register state (see the [glossary](04-terminology.md)).

```mermaid
---
config:
  flowchart:
    wrappingWidth: 320
---
flowchart TD
    subgraph machine["1. QEMU starts every hart in M-mode with paging off."]
        R["QEMU loads kernel/kernel into RAM at 0x80000000, attaches fs.img as a virtio disk, and its reset stub reaches _entry."]
        A["entry.S sets sp = stack0 + (hartid + 1) * 4096 and calls start()."]
        B["start.c prepares supervisor state and the timer; mret enters main() in S-mode."]
    end
    subgraph setup["2. main.c initializes shared state once and activates each hart."]
        C{"Is this hart 0?"}
        D["Hart 0 initializes subsystems; userinit() creates a RUNNABLE slot without user code."]
        E["Hart 0 publishes started = 1 with release ordering."]
        O["Other harts wait with acquire ordering, then activate paging, traps, and PLIC."]
    end
    subgraph process["3. The scheduler starts the first process in S-mode."]
        F["scheduler() switches to the context whose ra is forkret."]
        G["forkret() releases the process lock; its first invocation runs fsinit(), then kexec(/init)."]
        H{"Did kexec() succeed?"}
        X["forkret() calls panic(exec)."]
    end
    subgraph user["4. The first user program starts, then launches the shell."]
        I["prepare_return() and trampoline userret enter /init at its ELF entry in U-mode."]
        J["user/ulib.c start() calls user/init.c main(); init opens the console and dups fd 0 twice."]
        K["init forks; the child execs sh while init waits, reaps orphans, and restarts an exited shell."]
        L["Each later request, such as init calling fork(), runs a user/usys.S stub: li a7, SYS_fork, then ecall enters the kernel."]
    end
    R --> A --> B --> C
    C -->|Yes| D --> E --> F
    C -->|No| O --> F
    E -.->|The publication ends their wait.| O
    F --> G --> H
    H -->|No: returns -1| X
    H -->|Yes: returns argc| I --> J --> K
    K -.->|"Each fork() and exec() here is a system call."| L
    classDef processing fill:#fffff0,stroke:#999944,color:#111
    classDef routing fill:#f0f0ff,stroke:#6666aa,color:#111
    classDef commit fill:#f0ffff,stroke:#449999,color:#111
    classDef movement fill:#f0fff0,stroke:#669966,color:#111
    classDef failure fill:#ffd9d9,stroke:#aa5555,color:#111
    classDef request fill:#fff0f0,stroke:#aa6666,color:#111
    class A,D processing
    class L request
    class B,C,F,H,I routing
    class E commit
    class R,G,J,K movement
    class O,X failure
    style machine fill:#fffff0,stroke:#888,color:#111
    style setup fill:#fffff0,stroke:#888,color:#111
    style process fill:#f0f0ff,stroke:#888,color:#111
    style user fill:#f0fff0,stroke:#888,color:#111
```

**Sources behind the diagram:**

| Stage                                                 | Source                                                                                                    |
| ----------------------------------------------------- | --------------------------------------------------------------------------------------------------------- |
| **Machine-mode entry and supervisor setup**           | [`kernel/entry.S`](../kernel/entry.S), [`kernel/start.c`](../kernel/start.c).                             |
| **Shared initialization**                             | [`kernel/main.c`](../kernel/main.c).                                                                      |
| **Process creation, scheduling, and first execution** | [`userinit()` / `allocproc()` / `forkret()`](../kernel/proc.c), [`kernel/swtch.S`](../kernel/swtch.S).    |
| **Entry to user mode**                                | [`kernel/trampoline.S`](../kernel/trampoline.S).                                                          |
| **User entry and shell launch**                       | [`user/ulib.c`](../user/ulib.c), [`user/init.c`](../user/init.c).                                         |
| **System-call entry from `/init` and `sh`**           | [`user/usys.S`](../user/usys.S) (generated by `user/usys.pl`), [`kernel/syscall.h`](../kernel/syscall.h). |

**Physical starting point:** RAM starts at `0x80000000` on QEMU's `virt` board. Low physical addresses contain devices and the reset ROM; `-bios none` omits separate firmware but retains QEMU's reset stub. The [memory-map reference](06-build-artifacts.md#why-the-kernel-is-linked-at-0x80000000) owns the exact regions.

### Boot stacks and the M-mode to S-mode transition

**Each hart gets a downward-growing 4096-byte stack slice.** In `stack0[4096 * NCPU]`, `stack0 + (hartid + 1) * 4096` selects that slice's exclusive upper bound before C runs. Where those slices sit, and how the rest of RAM changes hands afterwards, is drawn in [Physical memory ownership across the boot stages](#physical-memory-ownership-across-the-boot-stages).

Kernel `start()` prepares the transition to S-mode:

| Setup                     | Value or work                                                                      |
| ------------------------- | ---------------------------------------------------------------------------------- |
| **Target privilege**      | `mstatus.MPP = S`                                                                  |
| **Next instruction**      | `mepc = main`                                                                      |
| **Paging**                | `satp = 0`                                                                         |
| **Trap delegation**       | `medeleg = mideleg = 0xffff`                                                       |
| **Supervisor facilities** | Supervisor interrupts, PMP access, hardware A/D updates, and the supervisor timer. |
| **Hart identity**         | `tp = mhartid`                                                                     |
| **Transfer**              | `mret` enters `main()` in S-mode.                                                  |

The complete register-write table remains in [Chapter 2's start() walkthrough](book/ch02-operating-system-organization.md#start-everything-machine-mode-has-to-do-first). **Timer version:** this tree uses `stimecmp`, rather than the older machine-mode clock-interrupt path.

### Multicore boot: shared initialization and per-hart activation

**Hart 0 initializes shared state; every hart can schedule the first process.** The release/acquire publication prevents other harts from using unfinished shared state.

**How many harts xv6 runs on** is set by the build, capped by a compile-time constant:

| Setting                   | Value                                                                                               | Source                                              |
| ------------------------- | --------------------------------------------------------------------------------------------------- | --------------------------------------------------- |
| **Harts QEMU creates**    | `-smp $(CPUS)`: 3 by default, 1 under `LAB=fs`, 4 under `LAB=lock`; `make CPUS=1 qemu` overrides it | [Makefile](../Makefile) `CPUS`                      |
| **Most xv6 supports**     | `NCPU` = 8, which sizes `cpus[NCPU]` and `stack0[4096 * NCPU]`                                      | `kernel/param.h`, `kernel/proc.c`, `kernel/start.c` |
| **Check between the two** | None: a hart id of 8 or more would index past both arrays                                           | `kernel/entry.S`, `mycpu()` in `kernel/proc.c`      |

**Why `main()` tests `cpuid() == 0`:** QEMU releases every hart at `_entry` at the same time, so all of them reach `main()` concurrently. The shared setup must run exactly once: two harts running `kinit()` would free every page onto the free list twice, and two running `virtio_disk_init()` would reset the disk under each other.

- **Election without a lock:** each hart's id is already in `tp` (copied from `mhartid` by `start()`), so the test needs no shared memory and cannot race.
- **Why 0 specifically:** QEMU numbers harts from 0, so hart 0 exists under any `-smp` value, including `CPUS=1`. It is chosen by id, not by arrival order; another hart may reach `main()` first and simply wait.
- **What every hart still does itself:** `satp`, `stvec`, and the PLIC enable bits are per-hart state, so the others run `kvminithart()`, `trapinithart()`, and `plicinithart()` once `started` is published.

| Actor                 | Work before scheduling                                                                                                          |
| --------------------- | ------------------------------------------------------------------------------------------------------------------------------- |
| **Hart 0**            | Initializes console/printing, allocator, and kernel page table; activates that page table.                                      |
| **Hart 0, continued** | Initializes process table, traps, PLIC, buffer/inode/file tables, and virtio disk; calls `userinit()` and publishes completion. |
| **Other harts**       | Wait for the release/acquire publication; activate the existing kernel page table, trap vector, and their PLIC enables.         |
| **Every hart**        | Enters `scheduler()`; any hart may schedule the first process.                                                                  |

**The first process slot contains no user program yet:**

- `allocproc()` gives it `context.ra = forkret`.
- `userinit()` sets its working directory to `/` and marks it `RUNNABLE`, without loading user code.

### `forkret()`: loading `/init` and entering user mode

**The first entry into U-mode uses the trap-return machinery**, even though this process has not made an earlier user system call:

1. The first `forkret()` runs `fsinit(ROOTDEV)` in process context, because initialization may sleep.
2. It calls `kexec("/init", {"/init", 0})`, places the result (`argc`) in `p->trapframe->a0`, and panics if the result is `-1`.
3. It calls `prepare_return()` and jumps to trampoline `userret` with the user `satp`.
4. The ELF entry, `start()` in `user/ulib.c`, calls `main(argc, argv)` and exits with its return value.

| Name                 | Role                                    |
| -------------------- | --------------------------------------- |
| **Kernel `start()`** | Machine-mode setup in `kernel/start.c`. |
| **User `start()`**   | `/init`'s ELF entry in `user/ulib.c`.   |

### Measured kernel symbol addresses in the boot path

| Symbol      | Address (`nm -n kernel/kernel`) | Role in the path                                  |
| ----------- | ------------------------------- | ------------------------------------------------- |
| `_entry`    | `0x80000000`                    | first kernel instruction after QEMU’s reset stub  |
| `main`      | `0x800002e8`                    | `mepc` target of `start()`'s `mret`               |
| `forkret`   | `0x80000dcc`                    | `allocproc()` stores it in `p->context.ra`        |
| `userinit`  | `0x8000105a`                    | creates the only process not made by `fork`       |
| `scheduler` | `0x80001206`                    | every hart ends `main()` here                     |
| `start`     | `0x800050a8`                    | the last machine-mode C function                  |
| `uservec`   | `0x80006000`                    | the trampoline page, mapped again at `TRAMPOLINE` |

### Physical memory ownership across the boot stages

**Boot never moves the kernel image; it only changes who owns the free RAM above it.** Before `kinit()` nothing manages that RAM, and afterwards every allocation peels pages off the top.

![Four columns showing physical RAM from 0x80000000 to 0x88000000 at four boot stages, with the free region becoming a free list and then losing pages from the top](images/boot-physical-memory.svg)

_Measured with GDB at `CPUS=3`. [View the SVG](images/boot-physical-memory.svg) or open [the editable Excalidraw drawing](images/boot-physical-memory.excalidraw) in Excalidraw. Thick borders mark what each stage changed; bands are not to scale._

| Stage                            | What changes                                                                                             | Measured value                                                                |
| -------------------------------- | -------------------------------------------------------------------------------------------------------- | ----------------------------------------------------------------------------- |
| **`_entry` and `start()`**       | Each hart sets `sp` inside `stack0`, which lives in the kernel's `.bss`. Paging is off.                  | `stack0` = `0x8001b470`; boot `sp` = `0x8001c470`, `0x8001d470`, `0x8001e470` |
| **`kinit()`**                    | `freerange(end, PHYSTOP)` hands every page from `PGROUNDUP(end)` to `PHYSTOP` to `kfree()`.              | 32,732 pages; free-list head `0x87fff000`                                     |
| **`kvminit()`**                  | `kvmmake()` allocates the root table, lower-level tables, and one kernel stack per `proc[]` slot.        | root `kernel_pagetable` = `0x87fff000`; 166 pages; head `0x87f59000`          |
| **`kvminithart()`**              | `satp` switches to `kernel_pagetable`. RAM is direct-mapped, so no address above changes meaning.        | `KSTACK(0)` = `0x3fffffd000` (virtual only)                                   |
| **`virtio_disk_init()`**         | Three queue pages: `desc`, `avail`, `used`.                                                              | `0x87f59000`–`0x87f57000`                                                     |
| **`userinit()` → `allocproc()`** | The first process slot gets a trapframe and a user page table holding only `TRAMPOLINE` and `TRAPFRAME`. | trapframe `0x87f56000`; user root `0x87f55000`; head `0x87f52000`             |

**Why allocations come from the top:** `kfree()` pushes each page onto the head of `kmem.freelist`, and `freerange()` walks upward, so the last page freed, `0x87fff000`, becomes the first one `kalloc()` returns.

**Why the kernel keeps working when paging turns on:** `kvmmake()` maps `KERNBASE` through `PHYSTOP` at identical virtual addresses, so every pointer created with `satp = 0` stays valid. Only the trampoline and the kernel stacks gain extra high virtual addresses; those do not add RAM above `PHYSTOP`.

**The first user page table is built but not used:** `satp` still holds `kernel_pagetable` at the end of `main()`. It switches to the replacement page table built by `kexec()` only when `forkret()` jumps to `userret`; `kexec()` frees the original `0x87f55000` table.

> [!NOTE]
> The page counts depend on `NPROC`, `CPUS`, and the build. Reproduce them with `make qemu-gdb` and these GDB commands, deleting each breakpoint after it hits so another hart does not stop there first:
>
> ```text
> hbreak kvminithart
> continue
> print/x kernel_pagetable
> print/x kmem.freelist
> delete
> hbreak scheduler
> continue
> print/x proc[0].trapframe
> print/x proc[0].pagetable
> print/x kmem.freelist
> ```

### `/init`: console descriptors and shell lifecycle

**`/init` prepares fds 0, 1, and 2 before starting the shell.** The order is:

1. Open `console`. If that fails, call `mknod("console", CONSOLE, 0)` and retry the open: a fresh `fs.img` may lack the device file.
2. Use the opened descriptor as fd 0, then call `dup(0)` twice for fds 1 and 2.
3. Let later processes inherit the three descriptors through `fork`.

| Process             | Shell lifecycle                                                                       |
| ------------------- | ------------------------------------------------------------------------------------- |
| **Child**           | Calls `exec("sh", argv)`.                                                             |
| **Parent (`init`)** | Calls `wait(0)`, reaps orphan exits, and restarts the shell when its pid is returned. |

**Failure behavior:** `fork` failure, a failed child `exec`, or a negative `wait` prints a diagnostic and exits that caller with status `1`.

## `fork()` and `kfork()`: process creation and table exhaustion

**`fork()` creates a child; exhausting the shared table returns `-1` while the kernel keeps running.** `kfork()` in `kernel/proc.c` follows the lecture's order:

1. `allocproc()` scans `proc[NPROC]` for an `UNUSED` slot, assigns a pid, allocates a trapframe page and an empty page table, and points `context.ra` at `forkret`. It returns `0` when no slot is free or `kalloc()` fails.
2. `uvmcopy()` copies the parent's `p->sz` bytes of user memory into the child's page table.
3. `*(np->trapframe) = *(p->trapframe)` copies saved registers, then `np->trapframe->a0 = 0` makes `fork()` return `0` in the child.
4. `filedup()` shares every open file and `idup()` shares the working directory.
5. The child is marked `RUNNABLE`; the parent gets the child's pid.

The lecture's homework question, "what happens if out of proc's?", is answered by [`forkfill`](../user/forkfill.c):

```text
$ forkfill
forkfill: fork returned -1 after 61 children
forkfill: reaped 61 children
forkfill: fork works again
```

| Quantity                 | Value | Source                                                           |
| ------------------------ | ----- | ---------------------------------------------------------------- |
| **`NPROC`**              | 64    | `kernel/param.h`; 10 under `LAB_FS`                              |
| **Already in use**       | 3     | `init`, `sh`, and `forkfill` itself                              |
| **Children before `-1`** | 61    | measured, and the same on a second run once the first was reaped |

| Event                               | Result                                                                           |
| ----------------------------------- | -------------------------------------------------------------------------------- |
| **No process slot is available**    | `allocproc()` returns `0`, `kfork()` returns `-1`, and the kernel keeps running. |
| **A child exits but is not reaped** | It remains a `ZOMBIE` and still occupies its slot.                               |
| **The parent reaps that child**     | The parent's `wait()` reaches `freeproc()`, making the slot available again.     |

> [!WARNING]
> **One user can exhaust the process table for everyone.** xv6 has no per-user process limit: this is the lecture's resource-isolation problem.

## `exec()`: hostile ELF checks and failure cleanup

**`exec()` checks hostile input before replacing the current image.** The responsibilities are:

| Stage                                  | Checks and ownership                                                                                                                                         |
| -------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| **Copy arguments into kernel storage** | The [argument-copy walkthrough](#following-a-user-string-with-copyinstr) owns `sys_exec()`'s pathname, pointer-array, string-copy, and failure-cleanup flow. |
| **Limit argument count**               | `MAXARG` is 32, **including the terminating `0` slot**.                                                                                                      |
| **Validate the new image**             | Once inputs are in kernel storage, `kexec()` checks the ELF and argument stack as listed below.                                                              |

| Check in `kexec()`                   | It rejects                                                              |
| ------------------------------------ | ----------------------------------------------------------------------- |
| **`elf.magic != ELF_MAGIC`**         | a file that is not ELF at all                                           |
| **`ph.memsz < ph.filesz`**           | a segment claiming more file bytes than memory to hold them             |
| **`ph.vaddr + ph.memsz < ph.vaddr`** | a segment whose end wraps around the 64-bit address space               |
| **`ph.vaddr % PGSIZE != 0`**         | a segment that does not start on a page boundary                        |
| **`sp < stackbase`**                 | arguments that would overflow the `USERSTACK` pages into the guard page |

The wrap-around check addresses the lecture's “don't be tricked into overwriting kernel memory”: without it, a huge `memsz` could give `uvmalloc()` a range whose end is below its start.

> [!IMPORTANT]
> **Replace the old image only after success.** Each listed `kexec()` failure goes to `bad:`, frees any half-built page table, and returns `-1`. The old image stays intact because `p->pagetable` is replaced only after all checks pass.

The resulting stack and `argv` layout is drawn in [10-c-in-xv6.md](10-c-in-xv6.md#argv-and-the-calling-convention).

## Lecture 3 differences: CPU count and system-call conventions

| Lecture 3 says                       | This tree                                                                                                             |
| ------------------------------------ | --------------------------------------------------------------------------------------------------------------------- |
| **4 cores**                          | 3 emulated harts by default (`CPUS` in the Makefile).                                                                 |
| **`ecall <n>`**                      | `ecall` takes no operand; the number travels in `a7`, set by the preceding `li` in `user/usys.S`.                     |
| **`k<syscallname>()` in the kernel** | Only for process calls; see [A system call is a number](#system-call-dispatch-numbers-handlers-and-invalid-requests). |

## Homework 3: a full process table versus `exec`, `fork`, and `kill`

**Only `fork()` needs a free `struct proc`.** With every `proc[NPROC]` entry in use, `exec()` works, `fork()` returns `-1`, and `kill()` frees nothing by itself, so a `fork()` right after it still returns `-1`. The question is from Homework 3, [_kernel design_](https://pdos.csail.mit.edu/6.1810/2026/homework/xv6-design.html); its reading list is in [STUDY-PROGRESS.md](../STUDY-PROGRESS.md#homework-and-assigned-reading).

| Call made while no slot is `UNUSED` | Result                                                 | Why, in this tree                                                                                                                                           |
| ----------------------------------- | ------------------------------------------------------ | ----------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **`exec()`**                        | Works normally; the pid is unchanged.                  | `kexec()` in `kernel/exec.c` works on `myproc()`. It builds a new page table with `proc_pagetable(p)` and frees the old one, so it needs pages, not a slot. |
| **`fork()`**                        | Returns `-1`; the caller and the kernel keep running.  | `allocproc()` in `kernel/proc.c` scans all `NPROC` entries, finds no `UNUSED` one, and returns `0`, so `kfork()` returns `-1`.                              |
| **`kill(pid)`, then `fork()`**      | `kill()` returns `0`; the `fork()` still returns `-1`. | `kkill()` only sets `p->killed = 1` and makes a `SLEEPING` victim `RUNNABLE`. The victim's slot stays occupied until it exits **and** its parent waits.     |

`exec()` can still return `-1` when `kalloc()` runs out of pages, or for any [hostile-ELF check](#exec-hostile-elf-checks-and-failure-cleanup), but never because the process table is full.

### Why `kill()` alone never frees a process slot

```mermaid
flowchart TD
    K["kill(pid): kkill() sets p->killed = 1 and wakes a SLEEPING victim."]
    R["The victim runs again and reaches usertrap() in kernel/trap.c."]
    X["killed(p) is true, so usertrap() calls kexit(-1)."]
    Z["kexit() marks the victim ZOMBIE: it still holds its slot, trapframe, page table, and user memory."]
    W{"Has the victim's parent called wait()?"}
    S["The slot stays occupied: fork() keeps returning -1."]
    F["kwait() calls freeproc(), which frees the memory and sets state = UNUSED."]
    N["The next allocproc() scan can take the slot, so fork() succeeds."]

    K --> R --> X --> Z --> W
    W -->|No| S
    S -.->|The parent waits later.| W
    W -->|Yes| F --> N

    classDef request fill:#fff0f0,stroke:#c66
    classDef processing fill:#fffff0,stroke:#cc6
    classDef decision fill:#f0f0ff,stroke:#66c
    classDef stall fill:#ffd9d9,stroke:#c33
    classDef commit fill:#f0ffff,stroke:#6cc
    class K request
    class R,X processing
    class W decision
    class Z,S stall
    class F,N commit
```

_Legend: request red-white, processing yellow, decision blue, stall or held resource red, commit cyan; every node also states its meaning in text._

**`kill()` is deliberately asynchronous.** The victim may be running on another hart or sleeping inside the kernel while holding locks, so destroying it on the spot could leave shared state inconsistent. xv6 instead lets the victim exit itself at a safe point: `usertrap()` checks `killed(p)` on system-call entry and again before returning to user space, and sleep loops such as `piperead()` in `kernel/pipe.c` and `sys_pause()` in `kernel/sysproc.c` check it after every wakeup.

**Exiting is not enough either.** `kexit()` is still running on the slot's kernel stack until `sched()` switches away, so the slot cannot be handed out yet, and the parent is owed the exit status. The `ZOMBIE` therefore keeps the slot until the parent's `wait()` reaches `freeproc()`.

### When `fork()` after `kill()` can succeed

| Situation                                                | `fork()` right after `kill()`                                                                                                                               |
| -------------------------------------------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **The caller is the victim's parent and has not waited** | Always `-1`: nobody else can reap the victim. This is the measured case below.                                                                              |
| **Another parent is already blocked in `wait()`**        | A race: `-1` unless the victim exits and its parent reaps it on another hart before `allocproc()` scans the table. The homework's answer cannot rely on it. |
| **The victim's parent never waits**                      | `-1` indefinitely. If that parent later exits, `reparent()` hands the zombie to `init`, whose `wait()` loop in `user/init.c` reaps it.                      |
| **The pid already belongs to a `ZOMBIE`**                | `kkill()` still matches the pid and returns `0`, but changes nothing: the slot is waiting for `wait()`, not for an exit.                                    |
| **The caller kills itself**                              | The `fork()` never runs: `usertrap()` sees `killed(p)` right after `syscall()` returns and calls `kexit(-1)`.                                               |

### Homework 3 evidence: `hwdesign` on a full table

[`user/hwdesign.c`](../user/hwdesign.c) runs all three questions in one process. A probe child is forked **before** the table fills, so it already owns a slot when it is told to `exec("echo", ...)`; the program then refills the probe's freed slot, kills one of its own blocked children, and forks before waiting:

```text
$ hwdesign
hwdesign: table full: fork returned -1 after 60 children
hwdesign: probe exec'd echo while the table was full
hwdesign: exec used the probe's own slot; probe exited 0
hwdesign: refilled 1 freed slot
hwdesign: kill(65) returned 0; fork right after returned -1
hwdesign: wait reaped killed pid 65, status -1
hwdesign: fork after that wait succeeded
hwdesign: reaped 60 children
```

| Line                                    | What it proves                                                                                                                                     |
| --------------------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------- |
| **60 children**                         | `NPROC` 64 minus `init`, `sh`, `hwdesign`, and the probe; one fewer than [`forkfill`](#fork-and-kfork-process-creation-and-table-exhaustion)'s 61. |
| **`echo`'s line, then exit status `0`** | `exec()` succeeded while `fork()` was failing for everyone.                                                                                        |
| **`kill(...)` `0`, then `-1`**          | Marking the victim did not return its slot.                                                                                                        |
| **Status `-1` from the killed pid**     | The victim left through `kexit(-1)` in `usertrap()`, and only this `wait()` freed its slot.                                                        |

`./grade-exercises` pins every line; inserting a `wait(0)` between the `kill()` and the `fork()` turns the test red, because the fork then succeeds.

### Homework 3 traps: tempting wrong answers

| Tempting answer                                         | Why it is wrong                                                                                   |
| ------------------------------------------------------- | ------------------------------------------------------------------------------------------------- |
| **`exec()` fails too, because it starts a new program** | `exec()` replaces the caller's image in place; there is no `allocproc()` call in `kernel/exec.c`. |
| **`kill()` destroys the process immediately**           | It sets a flag. The victim exits later, from its own kernel thread, at the next `killed()` check. |
| **An exited process frees its slot**                    | `kexit()` leaves a `ZOMBIE`; only the parent's `wait()` calls `freeproc()`.                       |
| **A full table panics the kernel**                      | `allocproc()` returns `0` and `fork()` returns `-1`. The failure is per call, not per system.     |

> [!NOTE] **At the shell, a full table ends the shell, not the system.** By reading the code (not run): `fork1()` in `user/sh.c` calls `panic("fork")`, which prints `fork` and calls `exit(1)`. `init`'s `wait()` then reaps the shell, and its next `fork()` reuses the shell's own freed slot to start a new one.

## Questions about page-table permissions and trap diagnostics

### Why can the user page table contain kernel pages at all?

**The trap path needs both mappings before it can switch page tables.** `uservec` in `kernel/trampoline.S` starts with the user's `satp` still active:

| Mapping        | Why it must be present                    | Why user code cannot use it |
| -------------- | ----------------------------------------- | --------------------------- |
| **Trampoline** | Contains the code `uservec` is executing. | `PTE_U` is clear.           |
| **Trapframe**  | Receives the saved user registers.        | `PTE_U` is clear.           |

Leaving `PTE_U` clear keeps U-mode out while S-mode can still use the mappings. [ch04](book/ch04-traps-and-system-calls.md) will own the full path.

### Why does `isolate csr` report `stval=0x100027f3`?

| Experiment          | Meaning of the observed `stval`                                       | Source evidence                                                                               |
| ------------------- | --------------------------------------------------------------------- | --------------------------------------------------------------------------------------------- |
| **`isolate csr`**   | `0x100027f3` is the faulting instruction's encoding in this QEMU run. | `user/isolate.asm` shows `100027f3` at `0x44`, equal to `sepc`, decoded as `csrr a5,sstatus`. |
| **`isolate kload`** | `0x3ffffff000` is the refused address.                                | This case is a load page fault rather than an illegal instruction.                            |

For illegal instructions, the architecture permits implementations to report instruction bits in `stval`; do not assume every RISC-V implementation supplies a nonzero encoding. Here the disassembly explains the measured value.
