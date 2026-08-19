# Chapter 8: Scheduling

---

[toc]

---

> **Theory:** [OSTEP — CPU Scheduling][ostep]. Read that first; this note does not re-explain it.

> **Placeholder — not yet written.** Chapter source: `../../../xv6-riscv-book/sched.tex`. Write it with the shape in [README.md](README.md#how-a-chapter-note-is-written), after checking [00-ostep-concordance.md](00-ostep-concordance.md) for what is already covered.

## Figures

Both of this chapter's figures are redrawn rather than copied — one is TikZ, the other ships only as a PDF; see [fig/README.md](fig/README.md#the-figures-that-are-redrawn-instead). Band colours follow the legend in [README.md](README.md#how-a-chapter-note-is-written).

```mermaid
sequenceDiagram
    participant U1 as Process 1 user space
    participant K1 as Process 1 kernel thread
    participant S as CPU scheduler thread
    participant K2 as Process 2 kernel thread
    participant U2 as Process 2 user space

    rect rgb(255, 240, 240)
    Note over U1,K1: Trap in — a system call or a timer interrupt
    U1->>K1: ecall or interrupt
    end

    rect rgb(240, 240, 255)
    Note over K1,S: Context switch out
    K1->>S: swtch(&p->context, &cpu->context)
    end

    rect rgb(255, 255, 240)
    Note over S,K2: The scheduler picks a RUNNABLE process
    S->>S: scan proc[] for RUNNABLE
    S->>K2: swtch(&cpu->context, &p->context)
    end

    rect rgb(240, 255, 255)
    Note over K2,U2: Trap return
    K2->>U2: sret
    end
```

_“Switching from one user process to another. In this example, xv6 runs with one CPU (and thus one scheduler thread).” — redrawn from `sched.tex` `fig:switch`. Four steps, and the one that surprises people is that there is no direct edge from `K1` to `K2`: xv6 never context-switches one process's kernel thread straight to another's. The scheduler thread is always in the middle, which is what makes the next figure's invariant statable at all._

```mermaid
sequenceDiagram
    participant P1 as Process 1
    participant S as Scheduler
    participant P2 as Process 2

    rect rgb(240, 240, 255)
    Note over P1,S: Process 1's p->lock: taken by the process, dropped by the scheduler
    P1->>P1: acquire(&p->lock)
    P1->>P1: p->state = RUNNABLE
    P1->>S: swtch(&p->context, ...)
    S->>S: release(&p->lock)
    end

    rect rgb(255, 255, 240)
    Note over S,S: Between the two spans, the scheduler holds no p->lock
    S->>S: find a RUNNABLE p
    end

    rect rgb(240, 255, 255)
    Note over S,P2: Process 2's p->lock: taken by the scheduler, dropped by the process
    S->>S: acquire(&p->lock)
    S->>S: p->state = RUNNING
    S->>P2: swtch(..., &p->context)
    P2->>P2: release(&p->lock)
    end
```

_“`swtch()` always has the scheduler thread as either source or destination, and the relevant `p->lock` is always held.” — redrawn from `sched.tex` `fig:inout`. Every `acquire` here is paired with a `release` in a **different thread**, which is why the lock cannot be held across the switch by the usual acquire-and-release-in-one-function discipline, and why `p->lock` is the one lock in the tree that discipline does not describe._

## What xv6 actually does

## Divergences

## Code walked through

## Questions I had

## Lab connection

[ostep]: ../../../operating-system/CPU_Scheduling.md
