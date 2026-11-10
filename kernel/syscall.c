#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "syscall.h"
#include "defs.h"

// Fetch the uint64 at addr from the current process.
int
fetchaddr(uint64 addr, uint64 *ip)
{
  struct proc *p = myproc();
  if (addr >= p->sz ||
      addr + sizeof(uint64) > p->sz) // both tests needed, in case of overflow
    return -1;
  if (copyin(p->pagetable, p->sz, (char *)ip, addr, sizeof(*ip)) != 0)
    return -1;
  return 0;
}

/*
 * Fetch the nul-terminated string at addr from the current process.
 * Returns length of string, not including nul, or -1 for error.
 */
int
fetchstr(uint64 addr, char *buf, int max)
{
  struct proc *p = myproc();
  if (copyinstr(p->pagetable, p->sz, buf, addr, max) < 0)
    return -1;
  return strlen(buf);
}

/*
 * Q: What does argraw mean?
 * A: The raw saved register value, before argint/argaddr interpret it
 *    as an int or a user address.
 *
 * Q: How about registers beyond a5?
 * A: xv6 passes at most six arguments, in a0-a5. a6 is unused and a7
 *    holds the syscall number. uservec saves every other integer
 *    register (ordinary registers, not CSRs) only so userret can
 *    restore them. See the register table in
 *    docs/labs/13-syscall-sandbox.md.
 */
static uint64
argraw(int n)
{
  struct proc *p = myproc();
  switch (n) {
  case 0:
    return p->trapframe->a0;
  case 1:
    return p->trapframe->a1;
  case 2:
    return p->trapframe->a2;
  case 3:
    return p->trapframe->a3;
  case 4:
    return p->trapframe->a4;
  case 5:
    return p->trapframe->a5;
  }
  panic("argraw");
  return -1;
}

// Fetch the nth 32-bit system call argument.
void
argint(int n, int *ip)
{
  *ip = argraw(n);
}

/*
 * Retrieve an argument as a pointer.
 * Doesn't check for legality, since
 * copyin/copyout will do that.
 */
void
argaddr(int n, uint64 *ip)
{
  *ip = argraw(n);
}

/*
 * Fetch the nth word-sized system call argument as a null-terminated string.
 * Copies into buf, at most max.
 * Returns string length if OK (not including nul), -1 if error.
 */
int
argstr(int n, char *buf, int max)
{
  uint64 addr;
  argaddr(n, &addr);
  return fetchstr(addr, buf, max);
}

// Prototypes for the functions that handle system calls.
extern uint64 sys_fork(void);
extern uint64 sys_exit(void);
extern uint64 sys_wait(void);
extern uint64 sys_pipe(void);
extern uint64 sys_read(void);
extern uint64 sys_kill(void);
extern uint64 sys_exec(void);
extern uint64 sys_fstat(void);
extern uint64 sys_chdir(void);
extern uint64 sys_dup(void);
extern uint64 sys_getpid(void);
extern uint64 sys_sbrk(void);
extern uint64 sys_pause(void);
extern uint64 sys_uptime(void);
extern uint64 sys_open(void);
extern uint64 sys_write(void);
extern uint64 sys_mknod(void);
extern uint64 sys_unlink(void);
extern uint64 sys_link(void);
extern uint64 sys_mkdir(void);
extern uint64 sys_close(void);
extern uint64 sys_sync(void);
extern uint64 sys_interpose(void);

/*
 * An array mapping syscall numbers from syscall.h
 * to the function that handles the system call.
 */
static uint64 (*syscalls[])(void) = {
  // clang-format off
  [SYS_fork]    = sys_fork,
  [SYS_exit]    = sys_exit,
  [SYS_wait]    = sys_wait,
  [SYS_pipe]    = sys_pipe,
  [SYS_read]    = sys_read,
  [SYS_kill]    = sys_kill,
  [SYS_exec]    = sys_exec,
  [SYS_fstat]   = sys_fstat,
  [SYS_chdir]   = sys_chdir,
  [SYS_dup]     = sys_dup,
  [SYS_getpid]  = sys_getpid,
  [SYS_sbrk]    = sys_sbrk,
  [SYS_pause]   = sys_pause,
  [SYS_uptime]  = sys_uptime,
  [SYS_open]    = sys_open,
  [SYS_write]   = sys_write,
  [SYS_mknod]   = sys_mknod,
  [SYS_unlink]  = sys_unlink,
  [SYS_link]    = sys_link,
  [SYS_mkdir]   = sys_mkdir,
  [SYS_close]   = sys_close,
  [SYS_sync]    = sys_sync,
  [SYS_interpose] = sys_interpose,
  // clang-format on
};

/*
 * -------------------------------------------------------------------------
 *  SANDBOX CHECK
 * -------------------------------------------------------------------------
 *
 * Decide whether p's interpose() policy refuses call num. It runs before
 * the handler, because an error returned afterwards could not undo the
 * handler's side effects (a created file, a replaced image).
 *
 *   Bit num set?  num is open/exec?  Argument 0 == allowed_path?  Result
 *   ------------  -----------------  ---------------------------  -------
 *    no            -                  not read                     allow
 *    yes           no                 not read                     deny
 *    yes           yes                yes                          allow
 *    yes           yes                no, or unreadable            deny
 *
 * The pathname is matched as text over the whole string (bound MAXPATH),
 * so "./README" and "READ" do not match "README". It names the program
 * when exec is masked and the file when open is masked. Contract and edge
 * cases: docs/labs/14-syscall-sandbox-path.md.
 *
 * Test membership, not equality, so a combined mask such as 32896 covers
 * both open and exec. The caller has already bounded num below
 * NELEM(syscalls) (24), so the int shift is defined.
 */
static int
sandbox_denies(struct proc *p, int num)
{
  char path[MAXPATH];

  if ((p->mask & (1 << num)) == 0)
    return 0;
  if (num != SYS_open && num != SYS_exec) {
    printk("%d %s: sys call %d is forbidden\n", p->pid, p->name, num);
    return 1;
  }
  // Both sys_open and sys_exec take the path as argument 0.
  if (argstr(0, path, MAXPATH) < 0) {
    printk("%d %s: sys call %d is forbidden: unreadable path\n", p->pid,
           p->name, num);
    return 1;
  }
  if (strncmp(path, p->allowed_path, MAXPATH) != 0) {
    printk("%d %s: sys call %d is forbidden: path %s not allowed\n",
           p->pid, p->name, num, path);
    return 1;
  }
  return 0;
}

void
syscall(void)
{
  int num;
  struct proc *p = myproc();

  /*
   * The "Using gdb" lab swaps this line for `num = *(int *)0;` to panic.
   * That load reads unmapped address 0 in S-mode; `num = 0;` only assigns
   * a value, so the bounds check refuses it and nothing faults. See
   * docs/labs/12-syscall-using-gdb.md, "Why num = 0; does not panic".
   */
  num = p->trapframe->a7;
  if (num > 0 && num < NELEM(syscalls) && syscalls[num]) {
    if (sandbox_denies(p, num)) {
      p->trapframe->a0 = -1;
    } else {
      // Call the handler for num; its return value becomes user a0.
      p->trapframe->a0 = syscalls[num]();
    }
  } else {
    printk("%d %s: unknown sys call %d\n", p->pid, p->name, num);
    p->trapframe->a0 = -1;
  }
}
