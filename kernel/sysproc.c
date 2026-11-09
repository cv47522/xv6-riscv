#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "vm.h"

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  kexit(n);
  return 0; // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return kfork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return kwait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int t;
  int n;

  argint(0, &n);
  argint(1, &t);
  addr = myproc()->sz;

  if (t == SBRK_EAGER || n < 0) {
    if (growproc(n) < 0) {
      return -1;
    }
  } else {
    /*
     * Lazily allocate memory for this process: increase its memory
     * size but don't allocate memory. If the processes uses the
     * memory, vmfault() will allocate it.
     */
    if (addr + n < addr)
      return -1;
    if (addr + n > TRAPFRAME)
      return -1;
    myproc()->sz += n;
  }
  return addr;
}

/*
 * ---------------------------------------------------------------------------
 *  PAUSE SYSTEM CALL: BEGINNER'S VIEW
 * ---------------------------------------------------------------------------
 *
 * user/sleep.c is a user program: it runs as a process and calls pause(n).
 * The generated stub in user/usys.S puts n in a0, the syscall number in a7,
 * and executes ecall. kernel/syscall.c dispatches here; argint(0, &n) reads
 * syscall argument zero from the saved a0. This function has no C arguments
 * because every syscall handler uses the same uint64 function(void) shape.
 *
 *   user/sleep.c -> pause(n) -> ecall -> sys_pause() -> user mode
 *
 * ticks is defined in kernel/trap.c and declared in kernel/defs.h. Hart 0
 * increments it on each timer interrupt. A hart is a hardware execution
 * context (a QEMU virtual CPU here), not a process or a software thread.
 * kernel/sysproc.c is part of one kernel image, not a separate program.
 *
 * Conceptually, this handler records the start tick, waits until n ticks
 * have elapsed, and returns. tickslock and the sleep/wakeup calls make that
 * wait safe without polling. docs/book/ch09-sleep-and-wakeup.md explains
 * those later concurrency details; they are not needed for user/sleep.c.
 */
uint64
sys_pause(void)
{
  int n;
  uint ticks0;

  /*
   * Q: Is the first argument in a0 "sleep" or a number?
   * A: The number. For pause(10), saved a0 holds 10 ticks and saved a7
   *    holds SYS_pause; the command name never reaches the kernel.
   *    Index 0 selects a0, and argint stores its low 32 bits in n.
   */
  argint(0, &n);
  if (n < 0)
    n = 0;
  acquire(&tickslock);
  ticks0 = ticks;
  while (ticks - ticks0 < n) {
    if (killed(myproc())) {
      release(&tickslock);
      return -1;
    }
    sleep_prepare(&ticks);
    release(&tickslock);
    sleep();
    acquire(&tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kkill(pid);
}

/*
 * return how many clock tick interrupts have occurred
 * since start.
 */
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

/*
 * Q: Why return uint64 rather than an int used as a boolean?
 * A: syscalls[] in kernel/syscall.c holds uint64 (*)(void) pointers, and
 *    syscall() stores the result in the 64-bit saved a0. The user-side
 *    int is a status, 0 for success and -1 for failure; read as a
 *    boolean, -1 would mean true.
 *
 * Rejecting a negative mask is this tree's choice, not the handout's;
 * it makes bit 31 unusable. The path argument (a1) is ignored until the
 * pathname exercise. The mask lives in struct proc, so kexec keeps it,
 * kfork copies it, and freeproc clears it.
 */
uint64
sys_interpose(void)
{
  int mask;

  argint(0, &mask);
  if (mask < 0)
    return -1;
  myproc()->mask = mask;
  return 0;
}
