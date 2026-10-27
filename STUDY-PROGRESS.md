# MIT 6.1810 (2026) self-study progress

---

[toc]

---

| Start here           | Link or detail                                                                         |
| -------------------- | -------------------------------------------------------------------------------------- |
| **Course schedule**  | [Fall 2026](https://pdos.csail.mit.edu/6.1810/2026/schedule.html) · fetched 2026-10-23 |
| **Saved schedule**   | [Original HTML](lectures/schedule.html)                                                |
| **Lecture archive**  | [Texts, code, and file inventory](lectures/README.md)                                  |
| **Download records** | [Source URLs, fetch dates, and SHA-256 hashes](lectures/manifest.json)                 |
| **Coverage**         | Every calendar material, plus reading links from the homework pages                    |
| **Dates**            | Course dates below; checkmarks follow your own study pace                              |

| Publication gap           | What to revisit                                                                         |
| ------------------------- | --------------------------------------------------------------------------------------- |
| **Future material**       | The schedule warns that notes and videos may still be 2025 copies; updates will follow. |
| **Lecture / Homework 19** | Verification / xv6iris: no published links in this snapshot.                            |
| **Homework 16**           | The question is still `TBD`.                                                            |

## How to track progress

| Mark  | Meaning                                                         |
| ----- | --------------------------------------------------------------- |
| `[x]` | Done, with notes, code, commits, or test results to back it up. |
| `[ ]` | Still open: not started, in progress, or no proof yet.          |

> [!IMPORTANT]
> Downloading a file does not count as studying it.

| Item         | When to check it off                                                                                     |
| ------------ | -------------------------------------------------------------------------------------------------------- |
| **Lecture**  | Study the lecture and keep notes or practice code.                                                       |
| **Homework** | Read the material, then write or check an answer. No saved answers were found, so all 23 remain open.    |
| **Lab**      | Finish every required exercise and check. Keep optional challenges and course submission steps separate. |

When you finish an item:

1. Check its box and add the completion date.
2. Link your notes or code, or record a commit hash.
3. Keep the original course links so you can fetch updates later.

## Current checkpoint

**Recorded on 2026-10-23.** These are study milestones, not claims of attendance or homework submission.

| Area                      | Work on record                                                                                                         | Next step                                                                                       |
| ------------------------- | ---------------------------------------------------------------------------------------------------------------------- | ----------------------------------------------------------------------------------------------- |
| **Lecture 1 practice**    | [Nine programs](#lecture-1-example-practice); commits `965f54e` and `9d2ca97`; [exercise guide](docs/07-exercises.md). | Review the stale test assertion: [11/12 checks passed](#verification-evidence).                 |
| **C and memory practice** | [memdump](user/memdump.c); commits `b85fe54` and `dfc56d0`.                                                            | Study the three Lecture 2 demos and Homework 2; the full lecture is not yet confirmed complete. |
| **Util lab**              | Four of five required programs; 15/15 selected checks passed.                                                          | Finish `find -exec`, run the full grader, and record actual hours.                              |
| **Book reading**          | [Chapter 1 note](docs/book/ch01-operating-system-interfaces.md); commit `32541d6`.                                     | Read the remaining chapters; their files are scaffolding or references, not proof of reading.   |

Lecture 1 is checked based on the nine programs and Chapter 1 notes. Attendance and Homework 1 completion are unconfirmed. Other lectures stay open until you confirm studying them.

## Lectures

Main texts are below. FAQs, examples, and the UART manual follow in the extras table.

- [x] **Lecture 01 · 2026-09-09** — [Introduction](https://pdos.csail.mit.edu/6.1810/2026/lec/l-overview.txt) / [local](lectures/l-overview.txt)
- [ ] **Lecture 02 · 2026-09-14** — [C in xv6](https://pdos.csail.mit.edu/6.1810/2026/lec/l-c.txt) / [local](lectures/l-c.txt)
- [ ] **Lecture 03 · 2026-09-16** — [OS design](https://pdos.csail.mit.edu/6.1810/2026/lec/l-os.txt) / [local](lectures/l-os.txt)
- [ ] **Lecture 04 · 2026-09-21** — [Page tables](https://pdos.csail.mit.edu/6.1810/2026/lec/l-vm.txt) / [local](lectures/l-vm.txt)
- [ ] **Lecture 05 · 2026-09-23** — [Locking](https://pdos.csail.mit.edu/6.1810/2026/lec/l-lockv2.txt) / [local](lectures/l-lockv2.txt)
- [ ] **Lecture 06 · 2026-09-28** — [System call entry/exit](https://pdos.csail.mit.edu/6.1810/2026/lec/l-internal.txt) / [local](lectures/l-internal.txt)
- [ ] **Lecture 07 · 2026-10-05** — [System call interposition](https://pdos.csail.mit.edu/6.1810/2026/lec/l-janus.txt) / [local](lectures/l-janus.txt)
- [ ] **Lecture 08 · 2026-10-07** — [Page faults](https://pdos.csail.mit.edu/6.1810/2026/lec/l-pgfaults.txt) / [local](lectures/l-pgfaults.txt)
- [ ] **Lecture 09 · 2026-10-13** — [Transparent super pages](https://pdos.csail.mit.edu/6.1810/2026/lec/l-superpages.txt) / [local](lectures/l-superpages.txt)
- [ ] **Lecture 10 · 2026-10-14** — [Virtual memory for applications](https://pdos.csail.mit.edu/6.1810/2026/lec/l-uservm.txt) / [local](lectures/l-uservm.txt)
- [ ] **Lecture 11 · 2026-10-19** — [Device drivers](https://pdos.csail.mit.edu/6.1810/2026/lec/l-interrupt.txt) / [local](lectures/l-interrupt.txt)
- [ ] **Lecture 12 · 2026-10-21** — [OS Organization](https://pdos.csail.mit.edu/6.1810/2026/lec/l-organization.txt) / [local](lectures/l-organization.txt)
- [ ] **Lecture 13 · 2026-10-26** — [Thread switching](https://pdos.csail.mit.edu/6.1810/2026/lec/l-threads.txt) / [local](lectures/l-threads.txt)
- [ ] **Lecture 14 · 2026-11-02** — [Coordination](https://pdos.csail.mit.edu/6.1810/2026/lec/l-coordination.txt) / [local](lectures/l-coordination.txt)
- [ ] **Lecture 15 · 2026-11-04** — [Networking](https://pdos.csail.mit.edu/6.1810/2026/lec/l-net.txt) / [local](lectures/l-net.txt)
- [ ] **Lecture 16 · 2026-11-09** — [Tail latency](https://pdos.csail.mit.edu/6.1810/2026/lec/l-shenango.txt) / [local](lectures/l-shenango.txt)
- [ ] **Lecture 17 · 2026-11-16** — [File systems](https://pdos.csail.mit.edu/6.1810/2026/lec/l-fs.txt) / [local](lectures/l-fs.txt)
- [ ] **Lecture 18 · 2026-11-18** — [Crash recovery](https://pdos.csail.mit.edu/6.1810/2026/lec/l-crash.txt) / [local](lectures/l-crash.txt)
- [ ] **Lecture 19 · 2026-11-23** — Verification; no text published yet. Revisit the schedule.
- [ ] **Lecture 20 · 2026-11-30** — [Multi-Core scalability and RCU](https://pdos.csail.mit.edu/6.1810/2026/lec/l-rcu.txt) / [local](lectures/l-rcu.txt)
- [ ] **Lecture 21 · 2026-12-02** — [Containers and Virtual Machines](https://pdos.csail.mit.edu/6.1810/2026/lec/l-isolation.txt) / [local](lectures/l-isolation.txt)
- [ ] **Lecture 22 · 2026-12-07** — [Kernel extensibility](https://pdos.csail.mit.edu/6.1810/2026/lec/l-bpf.txt) / [local](lectures/l-bpf.txt)
- [ ] **Lecture 23 · 2026-12-09** — [Meltdown](https://pdos.csail.mit.edu/6.1810/2026/lec/l-meltdown.txt) / [local](lectures/l-meltdown.txt)

### Lecture extras

| Lecture        | Material                          | Links                                                                                                   |
| -------------- | --------------------------------- | ------------------------------------------------------------------------------------------------------- |
| **Lecture 01** | examples                          | [MIT](https://pdos.csail.mit.edu/6.1810/2026/lec/l-overview/) · [local](lectures/l-overview/index.html) |
| **Lecture 02** | examples                          | [MIT](https://pdos.csail.mit.edu/6.1810/2026/lec/l-c/) · [local](lectures/l-c/index.html)               |
| **Lecture 10** | FAQ                               | [MIT](https://pdos.csail.mit.edu/6.1810/2026/lec/uservm-faq.txt) · [local](lectures/uservm-faq.txt)     |
| **Lecture 10** | sqrt example                      | [MIT](https://pdos.csail.mit.edu/6.1810/2026/lec/mmap.c) · [local](lectures/mmap.c)                     |
| **Lecture 10** | rfork example                     | [MIT](https://pdos.csail.mit.edu/6.1810/2026/lec/rfork) · [local](lectures/rfork/index.html)            |
| **Lecture 10** | sqrts.c (named in the notes)      | [MIT](https://pdos.csail.mit.edu/6.1810/2026/lec/sqrts.c) · [local](lectures/sqrts.c)                   |
| **Lecture 11** | 16550.pdf                         | [MIT](https://pdos.csail.mit.edu/6.1810/2026/lec/16550.pdf) · [local](lectures/16550.pdf)               |
| **Lecture 12** | FAQ                               | [MIT](https://pdos.csail.mit.edu/6.1810/2026/lec/l4-faq.txt) · [local](lectures/l4-faq.txt)             |
| **Lecture 13** | aaa.c                             | [MIT](https://pdos.csail.mit.edu/6.1810/2026/lec/aaa.c) · [local](lectures/aaa.c)                       |
| **Lecture 13** | bbb.c                             | [MIT](https://pdos.csail.mit.edu/6.1810/2026/lec/bbb.c) · [local](lectures/bbb.c)                       |
| **Lecture 14** | coordination C example            | [MIT](https://pdos.csail.mit.edu/6.1810/2026/lec/l-coordination.c) · [local](lectures/l-coordination.c) |
| **Lecture 15** | receive-livelock FAQ              | [MIT](https://pdos.csail.mit.edu/6.1810/2026/lec/livelock-faq.txt) · [local](lectures/livelock-faq.txt) |
| **Lecture 20** | RCU examples                      | [MIT](https://pdos.csail.mit.edu/6.1810/2026/lec/rcu/) · [local](lectures/rcu/index.html)               |
| **Lecture 22** | BPF examples (2025 link in notes) | [MIT](https://pdos.csail.mit.edu/6.1810/2025/lec/bpf/) · [local](lectures/bpf/index.html)               |
| **Lecture 23** | FAQ                               | [MIT](https://pdos.csail.mit.edu/6.1810/2026/lec/meltdown-faq.txt) · [local](lectures/meltdown-faq.txt) |

Additional code includes links found inside the lecture texts. The [archive inventory](lectures/README.md#downloaded-file-inventory) records every downloaded file and its source URL.

## Homework and assigned reading

| Use                    | What to do                                                                         |
| ---------------------- | ---------------------------------------------------------------------------------- |
| **Handout link**       | Open it for the exact question, experiment, and optional challenges.               |
| **Reading list below** | Follow the assigned sections; use the linked local code where available.           |
| **Self-study answer**  | Keep your answer locally.                                                          |
| **Enrolled students**  | Submit an ASCII `homework.txt` to the matching Lecture N assignment on Gradescope. |

- [x] **Homework 01 · 2026-09-09** — [file descriptors](https://pdos.csail.mit.edu/6.1810/2026/homework/xv6-intro.html)
    - **Read:** [`user/cat.c`](user/cat.c) ([upstream](https://github.com/mit-pdos/xv6-riscv/blob/riscv/user/cat.c)).
    - **Book:** Chapter 1.
    - **Optional:** [historic AT&T Unix film](https://www.youtube.com/watch?v=tc4ROCJYbm0).

- [ ] **Homework 02 · 2026-09-14** — [kernel C](https://pdos.csail.mit.edu/6.1810/2026/homework/xv6-c.html)
    - **Read:** K&R, _The C Programming Language_, second edition ([book information](https://en.wikipedia.org/wiki/The_C_Programming_Language)): §§2.9, 5.1–5.6, and 6.4.
    - **Task:** Work the handout’s pointer/structure expressions under its explicit 32-bit assumption.

- [ ] **Homework 03 · 2026-09-16** — [kernel design](https://pdos.csail.mit.edu/6.1810/2026/homework/xv6-design.html)
    - **Read:** [`kernel/proc.h`](kernel/proc.h), [`kernel/defs.h`](kernel/defs.h), [`kernel/entry.S`](kernel/entry.S), [`kernel/main.c`](kernel/main.c), and [`user/init.c`](user/init.c).
    - **Skim:** [`kernel/proc.c`](kernel/proc.c) and [`kernel/exec.c`](kernel/exec.c).
    - **Book:** Chapter 2.

- [ ] **Homework 04 · 2026-09-21** — [page tables](https://pdos.csail.mit.edu/6.1810/2026/homework/xv6-pgtbl.html)
    - **Read:** [`kernel/memlayout.h`](kernel/memlayout.h), [`kernel/vm.c`](kernel/vm.c), [`kernel/kalloc.c`](kernel/kalloc.c), [`kernel/riscv.h`](kernel/riscv.h), and [`kernel/exec.c`](kernel/exec.c).
    - **Book:** Chapter 3.

- [ ] **Homework 05 · 2026-09-23** — [Locking question](https://pdos.csail.mit.edu/6.1810/2026/homework/xv6-locks.html)
    - **Read:** [`kernel/spinlock.h`](kernel/spinlock.h) and [`kernel/spinlock.c`](kernel/spinlock.c).
    - **Book:** Chapter 7.
    - **Task:** Perform the acquire-loop experiment.
    - **Optional:** Try the version without `volatile`.

- [ ] **Homework 06 · 2026-09-28** — [trampoline](https://pdos.csail.mit.edu/6.1810/2026/homework/xv6-trampoline.html)
    - **Read:** [`kernel/riscv.h`](kernel/riscv.h), [`kernel/trampoline.S`](kernel/trampoline.S), and [`kernel/trap.c`](kernel/trap.c).
    - **Task:** Inspect the compiled trampoline as instructed.
    - **Book:** Chapter 4.
    - **Build detail:** Use addresses from your own build.

- [ ] **Homework 07 · 2026-10-05** — [system call interposition](https://pdos.csail.mit.edu/6.1810/2026/homework/paper-janus.html)
    - **Read:** [A Secure Environment for Untrusted Helper Applications](https://pdos.csail.mit.edu/6.1810/2026/readings/janus-usenix96.pdf), §§1–3, then [Traps and Pitfalls: Practical Problems in System Call Interposition Based Security Tools](https://pdos.csail.mit.edu/6.1810/2026/readings/garfinkel-ndss-2003.pdf).
    - **Focus:** The question refers to §4 of the latter.

- [ ] **Homework 08 · 2026-10-07** — [Page-fault question](https://pdos.csail.mit.edu/6.1810/2026/homework/xv6-pgfault.html)
    - **Read:** [`kernel/sysproc.c`](kernel/sysproc.c), [`kernel/trap.c`](kernel/trap.c), and [`kernel/vm.c`](kernel/vm.c), focusing on heap growth, `usertrap()`, `vmfault()`, and `copyout()`.
    - **Book:** Chapter 5.

- [ ] **Homework 09 · 2026-10-13** — [superpages](https://pdos.csail.mit.edu/6.1810/2026/homework/paper-superpages.html)
    - **Read:** [Practical, transparent operating system support for superpages](https://pdos.csail.mit.edu/6.1810/2026/readings/navarro-super.pdf), Navarro et al. (OSDI 2002).
    - **Focus:** The question compares §§4.1 and 4.5.

- [ ] **Homework 10 · 2026-10-14** — [Application-VM question](https://pdos.csail.mit.edu/6.1810/2026/homework/paper-uvm.html)
    - **Read:** [VM primitives for user programs](https://pdos.csail.mit.edu/6.1810/2026/readings/appel-li.pdf), Appel and Li (1991); §4, “VM primitive performance,” may be skipped.

- [ ] **Homework 11 · 2026-10-19** — [Device-driver question](https://pdos.csail.mit.edu/6.1810/2026/homework/xv6-interrupt.html)
    - **Read:** [`kernel/console.c`](kernel/console.c), [`kernel/uart.c`](kernel/uart.c), and [`kernel/kernelvec.S`](kernel/kernelvec.S).
    - **Book:** Chapter 6.
    - **Reference:** [16550 manual](https://pdos.csail.mit.edu/6.1810/2026/lec/16550.pdf) ([local](lectures/16550.pdf)).
    - **Task:** Perform the interrupt-enable experiment.

- [ ] **Homework 12 · 2026-10-21** — [microkernels](https://pdos.csail.mit.edu/6.1810/2026/homework/paper-microkernel.html)
    - **Read:** [The Performance of micro-Kernel-Based Systems](https://pdos.csail.mit.edu/6.1810/2026/readings/microkernel.pdf).

- [ ] **Homework 13 · 2026-10-26** — [Thread switching](https://pdos.csail.mit.edu/6.1810/2026/homework/xv6-threads.html)
    - **Read:** [`kernel/proc.c`](kernel/proc.c), especially `yield()`, `sched()`, and `scheduler()`, and [`kernel/swtch.S`](kernel/swtch.S).
    - **Book:** Chapter 8.

- [ ] **Homework 14 · 2026-11-02** — [Coordination](https://pdos.csail.mit.edu/6.1810/2026/homework/xv6-coordination.html)
    - **Read:** `sleep()` and `wakeup()` in [`kernel/proc.c`](kernel/proc.c), `uartwrite()` and `uartintr()` in [`kernel/uart.c`](kernel/uart.c), and all of [`kernel/pipe.c`](kernel/pipe.c).
    - **Book:** Chapter 9.
    - **Optional:** UART challenge in the handout.

- [ ] **Homework 15 · 2026-11-04** — [Receive livelock](https://pdos.csail.mit.edu/6.1810/2026/homework/paper-livelock.html)
    - **Read:** [Eliminating Receive Livelock in an Interrupt-driven Kernel](https://pdos.csail.mit.edu/6.1810/2026/readings/mogul96usenix.pdf).
    - **Optional:** [FAQ](https://pdos.csail.mit.edu/6.1810/2026/lec/livelock-faq.txt) ([local](lectures/livelock-faq.txt)).

- [ ] **Homework 16 · 2026-11-09** — [Tales of the Tail](https://pdos.csail.mit.edu/6.1810/2026/homework/paper-latency.html)
    - **Read:** [Tales of the Tail: hardware, OS, and application-level sources of tail latency](https://pdos.csail.mit.edu/6.1810/2026/readings/latency-socc14.pdf).
    - **Status:** The question is `TBD` in the fetched handout.

- [ ] **Homework 17 · 2026-11-16** — [File systems](https://pdos.csail.mit.edu/6.1810/2026/homework/xv6-fs.html)
    - **Read:** [`kernel/buf.h`](kernel/buf.h), [`kernel/fs.h`](kernel/fs.h), [`kernel/bio.c`](kernel/bio.c), [`kernel/fs.c`](kernel/fs.c), [`kernel/sysfile.c`](kernel/sysfile.c), and [`kernel/file.c`](kernel/file.c).
    - **Book:** Chapter 10.
    - **Optional:** Second corruption experiment in the handout.

- [ ] **Homework 18 · 2026-11-18** — [Logging](https://pdos.csail.mit.edu/6.1810/2026/homework/xv6-log.html)
    - **Read:** [`kernel/log.c`](kernel/log.c) ([upstream](https://github.com/mit-pdos/xv6-riscv/blob/riscv/kernel/log.c)) and the logging sections of the [book’s File system chapter](https://mit-pdos.github.io/xv6-riscv-book/) (Chapter 10, §§10.4–10.6 in the homework).
    - **Focus:** These are also the schedule’s explicit Lecture 18 preparation.
    - **Task:** Perform the `log_write()` observation experiment.

- [ ] **Homework 19 · 2026-11-23** — xv6iris — no homework handout, reading link, or question published in the schedule yet.

- [ ] **Homework 20 · 2026-11-30** — [RCU](https://pdos.csail.mit.edu/6.1810/2026/homework/paper-rcu.html)
    - **Read:** [RCU Usage in the Linux kernel: one decade later](https://pdos.csail.mit.edu/6.1810/2026/readings/rcu-decade-later.pdf).
    - **Focus:** The question refers to Figure 8 and a lab read-lock implementation.

- [ ] **Homework 21 · 2026-12-02** — [Containers](https://pdos.csail.mit.edu/6.1810/2026/homework/paper-blending.html)
    - **Read:** [Blending Containers and Virtual Machines: a study of Firecracker and gVisor (2020)](https://pdos.csail.mit.edu/6.1810/2026/readings/containers-vm.pdf), §§1–3; later detailed comparisons are not required.

- [ ] **Homework 22 · 2026-12-07** — [BPF](https://pdos.csail.mit.edu/6.1810/2026/homework/paper-bpf.html)
    - **Read:** [The BSD Packet Filter: A New Architecture for User-level Packet Capture](https://pdos.csail.mit.edu/6.1810/2026/readings/bpf-usenix93.pdf).
    - **Optional:** [A thorough introduction to eBPF](https://lwn.net/Articles/740157/).

- [ ] **Homework 23 · 2026-12-09** — [meltdown](https://pdos.csail.mit.edu/6.1810/2026/homework/paper-meltdown.html)
    - **Read:** [Meltdown: Reading Kernel Memory from User Space](https://pdos.csail.mit.edu/6.1810/2026/readings/meltdown.pdf).

## Labs

- **Dates:** Assigned and due dates follow the schedule, even on holidays or hacking days.
- **Status:** Util is in progress; no completed work is on record for the later labs in this snapshot.

- [ ] **[util: Unix utilities](https://pdos.csail.mit.edu/6.1810/2026/labs/util.html)** — assigned 2026-09-09; due 2026-09-17. In progress: [four completed programs](#util-exercise-checklist); `find -exec` remains.
- [ ] **[syscall: System calls](https://pdos.csail.mit.edu/6.1810/2026/labs/syscall.html)** — assigned 2026-09-16; due 2026-09-24.
- [ ] **[pgtbl: Page tables](https://pdos.csail.mit.edu/6.1810/2026/labs/pgtbl.html)** — assigned 2026-09-23; due 2026-10-01.
- [ ] **[traps: Traps](https://pdos.csail.mit.edu/6.1810/2026/labs/traps.html)** — assigned 2026-09-30; due 2026-10-08.
- [ ] **[cow: Copy-on-write fork](https://pdos.csail.mit.edu/6.1810/2026/labs/cow.html)** — assigned 2026-10-07; due 2026-10-15.
- [ ] **[lock: Parallelism/locking](https://pdos.csail.mit.edu/6.1810/2026/labs/lock.html)** — assigned 2026-10-14; due 2026-10-22.
- [ ] **[net: Network driver](https://pdos.csail.mit.edu/6.1810/2026/labs/net.html)** — assigned 2026-10-21; due 2026-11-12.
- [ ] **[fs: File system](https://pdos.csail.mit.edu/6.1810/2026/labs/fs.html)** — assigned 2026-11-11; due 2026-11-19.
- [ ] **[mmap: Mmap](https://pdos.csail.mit.edu/6.1810/2026/labs/mmap.html)** — assigned 2026-11-18; due 2026-12-04.

### Util exercise checklist

[Util handout](https://pdos.csail.mit.edu/6.1810/2026/labs/util.html) · [Lab workflow](docs/03-lab-workflow.md)

- [x] **Boot xv6** — [setup](docs/01-environment-setup.md), [build and usage](docs/02-build-boot-and-usage.md); QEMU grader runs succeeded on 2026-10-23.
- [x] **sleep** — [source](user/sleep.c) · [derivation note](docs/labs/01-util-sleep.md)
- [x] **sixfive** — [source](user/sixfive.c) · [derivation note](docs/labs/02-util-sixfive.md)
- [x] **memdump** — [source](user/memdump.c) · [derivation note](docs/labs/03-util-memdump.md)
- [x] **find** — [source](user/find.c) · [derivation note](docs/labs/04-util-find.md)
- [ ] **exec extension** — finish `find -exec`; [derivation note](docs/labs/05-util-find-exec.md); three exec tests in [grade-lab-util](grade-lab-util).
- [ ] **Full verification and time record** — run the full grader; record actual hours in `time.txt`; save any required written answers.
- [ ] **Optional util challenges**
    - [x] **uptime** — [source](user/uptime.c) · [derivation note](docs/labs/06-util-uptime.md); four local tests in [grade-lab-util](grade-lab-util).
    - [ ] **find regular expressions** — [derivation note](docs/labs/07-util-find-regex.md).
    - [ ] **Shell** — [no prompt from a file](docs/labs/08-util-sh-prompt.md), [wait](docs/labs/09-util-sh-wait.md), [tab completion](docs/labs/10-util-sh-tab-completion.md), and [history](docs/labs/11-util-sh-history.md).
- [ ] **Optional institutional submission** — enrolled students follow the handout’s Gradescope and zipball steps after verification; not required for self-study.

| Program   | Commits                         | Selected checks passed on 2026-10-23 |
| --------- | ------------------------------- | ------------------------------------ |
| `sleep`   | `d4f7615`                       | 4/4                                  |
| `sixfive` | `5ec5b5e`, `d7c5fa1`, `2109fd1` | 4/4                                  |
| `memdump` | `b85fe54`, `dfc56d0`            | 3/3                                  |
| `find`    | `e1eb0f0`                       | 4/4                                  |

> [!NOTE]
> The selected checks exclude exec and time. A derivation note or temporary draft does not complete an exercise.

### Lecture 1 example practice

- **Originals:** [lectures/l-overview/](lectures/l-overview/), including `echo.c`; xv6 supplies [user/echo.c](user/echo.c).
- **Practice versions:** The nine personal xv6 programs below; suite commit `965f54e`, followed by documentation refinements.
- **Completion basis:** Implementation history; see the [current test limit](#verification-evidence).

- [x] **Example 1: ex1copy** — [MIT](https://pdos.csail.mit.edu/6.1810/2026/lec/l-overview/ex1.c), [saved original](lectures/l-overview/ex1.c), [practice implementation](user/ex1copy.c)
- [x] **Example 2: ex2create** — [MIT](https://pdos.csail.mit.edu/6.1810/2026/lec/l-overview/ex2.c), [saved original](lectures/l-overview/ex2.c), [practice implementation](user/ex2create.c)
- [x] **Example 3: ex3fork** — [MIT](https://pdos.csail.mit.edu/6.1810/2026/lec/l-overview/ex3.c), [saved original](lectures/l-overview/ex3.c), [practice implementation](user/ex3fork.c)
- [x] **Example 4: ex4exec** — [MIT](https://pdos.csail.mit.edu/6.1810/2026/lec/l-overview/ex4.c), [saved original](lectures/l-overview/ex4.c), [practice implementation](user/ex4exec.c)
- [x] **Example 5: ex5forkexec** — [MIT](https://pdos.csail.mit.edu/6.1810/2026/lec/l-overview/ex5.c), [saved original](lectures/l-overview/ex5.c), [practice implementation](user/ex5forkexec.c)
- [x] **Example 6: ex6redirect** — [MIT](https://pdos.csail.mit.edu/6.1810/2026/lec/l-overview/ex6.c), [saved original](lectures/l-overview/ex6.c), [practice implementation](user/ex6redirect.c)
- [x] **Example 7: ex7pipe** — [MIT](https://pdos.csail.mit.edu/6.1810/2026/lec/l-overview/ex7.c), [saved original](lectures/l-overview/ex7.c), [practice implementation](user/ex7pipe.c)
- [x] **Example 8: ex8pipefork** — [MIT](https://pdos.csail.mit.edu/6.1810/2026/lec/l-overview/ex8.c), [saved original](lectures/l-overview/ex8.c), [practice implementation](user/ex8pipefork.c)
- [x] **Example 9: ex9ls** — [MIT](https://pdos.csail.mit.edu/6.1810/2026/lec/l-overview/ex9.c), [saved original](lectures/l-overview/ex9.c), [practice implementation](user/ex9ls.c)

## Books, code handouts, and course references

- [ ] **Read the complete xv6 book** — [book and source](https://mit-pdos.github.io/xv6-riscv-book/); [local chapter notes and ownership rules](docs/book/README.md). Chapter 1 is evidenced below; the whole book remains open.
- [x] **Chapter 1 study note** — [Operating system interfaces](docs/book/ch01-operating-system-interfaces.md), commit `32541d6`; supports the Lecture 1 completion inference.
- [ ] **Read the complete xv6 kernel** — [code handout](https://github.com/mit-pdos/xv6-riscv), [local kernel](kernel/), and [local user programs](user/); source availability does not establish a completed read-through.
- [ ] **Course orientation and tools** — supporting links from the schedule navigation:
    - [overview](https://pdos.csail.mit.edu/6.1810/2026/overview.html)
    - [course structure](https://pdos.csail.mit.edu/6.1810/2026/general.html)
    - [tools](https://pdos.csail.mit.edu/6.1810/2026/tools.html)
    - [lab guidance](https://pdos.csail.mit.edu/6.1810/2026/labs/guidance.html)
    - [xv6 information](https://pdos.csail.mit.edu/6.1810/2026/xv6.html)
    - [references](https://pdos.csail.mit.edu/6.1810/2026/reference.html)

| Other course links                                                 | Use                                                                 |
| ------------------------------------------------------------------ | ------------------------------------------------------------------- |
| **[Piazza](https://piazza.com/mit/fall2026/61810)**                | Class-only support.                                                 |
| **[2025 course archive](https://pdos.csail.mit.edu/6.1810/2025/)** | Linked from the schedule; keep it separate from this 2026 snapshot. |

## Exams and practice

- [ ] **Midterm preparation and self-test · 2026-10-28** — [previous quizzes](https://pdos.csail.mit.edu/6.1810/2026/quiz.html)
- [ ] **Final preparation and self-test · 2026-12-16** — [previous quizzes](https://pdos.csail.mit.edu/6.1810/2026/quiz.html)

| Course detail | Midterm             | Final                          |
| ------------- | ------------------- | ------------------------------ |
| **Lectures**  | 1–12                | 13–23                          |
| **Labs**      | Through cow         | Net through mmap, as published |
| **Time**      | 1:05–2:25pm         | 9:00–11:00am                   |
| **Location**  | 50-340 (Walker)     | 50-340 (Walker)                |
| **Allowed**   | Book, notes, laptop | Book, notes                    |
| **Network**   | Closed              | Closed                         |

## Verification evidence

| Snapshot                     | Value                                                                                                  |
| ---------------------------- | ------------------------------------------------------------------------------------------------------ |
| **Date**                     | 2026-10-23                                                                                             |
| **Commit**                   | Local `HEAD` at the time: `e1eb0f0`                                                                    |
| **Code assessed**            | That commit and the working tree                                                                       |
| **Work already in progress** | Makefile, find/exec notes, and the temporary find program; none proves the exec extension is complete. |

| Command                                       | Result            | What the run covers                                    |
| --------------------------------------------- | ----------------- | ------------------------------------------------------ |
| `./grade-lab-util sleep sixfive memdump find` | Exit 0 · 15/15 OK | Four implemented exercises; excludes exec and time.    |
| `./grade-exercises`                           | Exit 1 · 11/12    | One stale ex3fork assertion; every other check passed. |

### Why the ex3fork check failed

| Branch     | Transcript        | [Existing test](grade-exercises) expects |
| ---------- | ----------------- | ---------------------------------------- |
| **Parent** | `parent: x is 11` | `parent, x is 11`                        |
| **Child**  | `child: x is 22`  | `child, x is 22`                         |

The test expects commas where the program prints colons, which explains the assertion failure. No source or grader change was made for this documentation task. The Lecture 1 checkmark records completed practice; it does not claim a fully passing suite.

### Check again after a code change

```bash
./grade-exercises
./grade-lab-util sleep sixfive memdump find
# After completing find -exec and recording actual time spent:
make grade
```
