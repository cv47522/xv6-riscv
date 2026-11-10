// Saved registers for kernel context switches.
struct context {
  uint64 ra;
  uint64 sp;

  // callee-saved
  uint64 s0;
  uint64 s1;
  uint64 s2;
  uint64 s3;
  uint64 s4;
  uint64 s5;
  uint64 s6;
  uint64 s7;
  uint64 s8;
  uint64 s9;
  uint64 s10;
  uint64 s11;
};

/*
 * Per-CPU state.
 *
 * Q: Isn't one process per CPU?
 * A: One at a time, not one for life. cpus[NCPU] describes harts and
 *    proc[NPROC] describes processes; NPROC (64) exceeds NCPU (8).
 *    c->proc names the process this hart runs right now, and is 0
 *    while the hart runs scheduler() on c->context. After each sched()
 *    a process may resume on any hart, so nothing pins it to one.
 *    See docs/book/ch08-scheduling.md.
 */
struct cpu {
  struct proc *proc;      // The process running on this cpu, or null.
  struct context context; // swtch() here to enter scheduler().
  int noff;               // Depth of push_off() nesting.
  int intena;             // Were interrupts enabled before push_off()?
};

extern struct cpu cpus[NCPU];

/*
 * per-process data for the trap handling code in trampoline.S.
 * sits in a page by itself just under the trampoline page in the
 * user page table. not specially mapped in the kernel page table.
 * uservec in trampoline.S saves user registers in the trapframe,
 * then initializes registers from the trapframe's
 * kernel_sp, kernel_hartid, kernel_satp, and jumps to kernel_trap.
 * prepare_return() and userret in trampoline.S set up
 * the trapframe's kernel_*, restore user registers from the
 * trapframe, switch to the user page table, and enter user space.
 *
 * Q: Aren't these the same as the registers?
 * A: Same names, different storage. A hart has one register file, and
 *    usertrap() needs it, so uservec copies the 31 user registers (all
 *    but x0) into this page and userret copies them back. The kernel_*
 *    fields are not user registers: uservec loads them to enter the
 *    kernel. epc holds the user pc that usertrap() copies from sepc.
 *    Each offset must match the sd/ld offsets in kernel/trampoline.S.
 *
 * Q: Why do ra, sp, and s0-s11 also appear in struct context?
 * A: The two structs save different values at different boundaries:
 *
 *      Struct     Saved by  At                    Holds values of
 *      ---------  --------  --------------------  --------------------
 *      trapframe  uservec   any trap from U-mode  the user program
 *      context    swtch()   a call from sched()   a kernel thread
 *
 *    A trap can land between any two user instructions, so every
 *    register may be live and all are saved. swtch() is an ordinary
 *    call: the compiler has already saved caller-saved registers, so
 *    only ra, sp, and callee-saved s0-s11 remain. A process blocked in
 *    sleep() has both: user s0 in its trapframe, kernel s0 in context.
 */
struct trapframe {
  uint64 kernel_satp;    //   0: kernel page table
  uint64 kernel_sp;      //   8: top of process's kernel stack
  uint64 kernel_trap;    //  16: usertrap()
  uint64 epc;            //  24: saved user program counter
  uint64 kernel_hartid;  //  32: saved kernel tp
  uint64 ra;             //  40
  uint64 sp;             //  48
  uint64 gp;             //  56
  uint64 tp;             //  64
  uint64 t0;             //  72
  uint64 t1;             //  80
  uint64 t2;             //  88
  uint64 s0;             //  96
  uint64 s1;             // 104
  uint64 a0;             // 112
  uint64 a1;             // 120
  uint64 a2;             // 128
  uint64 a3;             // 136
  uint64 a4;             // 144
  uint64 a5;             // 152
  uint64 a6;             // 160
  uint64 a7;             // 168
  uint64 s2;             // 176
  uint64 s3;             // 184
  uint64 s4;             // 192
  uint64 s5;             // 200
  uint64 s6;             // 208
  uint64 s7;             // 216
  uint64 s8;             // 224
  uint64 s9;             // 232
  uint64 s10;            // 240
  uint64 s11;            // 248
  uint64 t3;             // 256
  uint64 t4;             // 264
  uint64 t5;             // 272
  uint64 t6;             // 280
};

enum procstate { UNUSED, USED, SLEEPING, RUNNABLE, RUNNING, ZOMBIE };

/*
 * Per-process state
 *
 * Q: What is chan?
 * A: A wait channel: a kernel address used only as a key naming the
 *    event a process waits for, such as &ticks, &pi->nread, or the
 *    waiting parent's own struct proc in kwait(). sleep_prepare(chan)
 *    stores the key; wakeup(chan) clears p->chan in every process
 *    holding the same key and makes SLEEPING ones RUNNABLE. Clearing
 *    it also records that the wakeup happened, so sleep() sleeps only
 *    while p->chan is still non-zero. See kernel/proc.c and
 *    docs/book/ch09-sleep-and-wakeup.md.
 */
struct proc {
  struct spinlock lock;

  // p->lock must be held when using these:
  enum procstate state; // Process state
  void *chan;           // If non-zero, sleeping on chan
  int killed;           // If non-zero, have been killed
  int xstate;           // Exit status to be returned to parent's wait
  int pid;              // Process ID

  // wait_lock must be held when using this:
  struct proc *parent; // Parent process

  // these are private to the process, so p->lock need not be held.
  /*
   * Q: How does the kernel stack differ from the user call stack?
   * A: Both hold ordinary call frames; they differ in who uses them:
   *
   *      Stack   Used while         Lives in                Set up by
   *      ------  -----------------  ----------------------  -----------
   *      user    U-mode code runs   user page table, below  kexec in
   *                                 a PTE_U-less guard      kernel/exec.c
   *      kernel  this process is    kernel page table at    procinit, once
   *              in a trap/syscall  KSTACK(slot), one page  at boot
   *
   *    uservec in kernel/trampoline.S switches sp to trapframe->kernel_sp
   *    because the user sp is untrusted, and kernel frames must not be
   *    visible to user code. See "Kernel stack vs. user stack" in
   *    docs/book/ch02-operating-system-organization.md.
   */
  uint64 kstack;               // Virtual address of kernel stack
  uint64 sz;                   // Size of process memory (bytes)
  pagetable_t pagetable;       // User page table
  struct trapframe *trapframe; // data page for trampoline.S
  struct context context;      // swtch() here to run process
  struct file *ofile[NOFILE];  // Open files
  struct inode *cwd;           // Current directory
  char name[16];               // Process name (debugging)
  int mask;                    // Mask of disallowed system calls
  char allowed_path[MAXPATH];  // Masked open/exec may use this path
};
