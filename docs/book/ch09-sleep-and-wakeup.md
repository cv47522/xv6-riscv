# Chapter 9: Sleep and Wakeup

---

[toc]

---

> **Theory on loan.** [OSTEP — Condition Variables and Semaphores][ostep] is not written yet, so this note holds the concept itself for now. When that note is written, move the material there and replace this with a theory link — see [00-ostep-concordance.md](00-ostep-concordance.md).

> **Placeholder — not yet written.** Chapter source: `../../../xv6-riscv-book/sleep.tex`. Write it with the shape in [README.md](README.md#how-a-chapter-note-is-written), after checking [00-ostep-concordance.md](00-ostep-concordance.md) for what is already covered.

## Figures

The book's figure is TikZ, so it is redrawn here ([fig/README.md](fig/README.md#the-figures-that-are-redrawn-instead)). The gutter marks which lock is held on which lines:

```c
piperead() {
  acquire(&pipe->lock);            // ┐ holding pipe->lock
  while(no data in pipe->buffer) { // │
    sleep(&pipe, &pipe->lock) {    // │
      // in sleep()                // │
      acquire(&p->lock)            // │ ┐ holding p->lock
      release(&pipe->lock)         // ┘ │
      p->state = SLEEPING          //   │
      ...                          //   │
      swtch() {                    //   │
        // in scheduler()          //   │
        release(&p->lock)          //   ┘
```

_“Overlapping locks to avoid lost wake-up” — redrawn from `sleep.tex` `fig:overlap`. The two spans overlap by exactly two lines, and that overlap is the whole trick: no instant exists in which the sleeper holds neither lock, so no `wakeup()` can slip between deciding to sleep and being marked `SLEEPING`._

> [!IMPORTANT]
> **This tree does not do that.** `sleep()` here takes no arguments and there is no overlap; the divergence is large enough to belong in `## Divergences` when this note is written.

```c
piperead(struct pipe *pi, ...) {
  acquire(&pi->lock);              // ┐ holding pi->lock
  while (pi->nread == pi->nwrite && pi->writeopen) {
    sleep_prepare(&pi->nread);     // │ takes p->lock, sets p->chan, drops p->lock
    release(&pi->lock);            // ┘
    sleep();                       //   retakes p->lock; sleeps only if p->chan != 0
    acquire(&pi->lock);
  }
```

_`kernel/pipe.c` `piperead()` and `kernel/proc.c` `sleep_prepare()`, `sleep()`, `wakeup()`. The lost wakeup is closed by a recorded flag rather than by overlapping spans: `sleep_prepare()` publishes `p->chan` before `pi->lock` is dropped, `wakeup()` clears `p->chan` under `p->lock`, and `sleep()` re-checks it — so a wakeup that arrives in the gap is not missed, it is **found**, and `sleep()` returns without ever sleeping._

## What xv6 actually does

## Divergences

## Code walked through

## Questions I had

## Lab connection

[ostep]: ../../../operating-system/Condition_Variables_And_Semaphores.md
