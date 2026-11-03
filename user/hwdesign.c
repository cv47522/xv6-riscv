/*
 * hwdesign.c: Homework 3's three questions, run against a full proc[].
 *
 * MIT 6.1810 Homework 3, "kernel design":
 * https://pdos.csail.mit.edu/6.1810/2026/homework/xv6-design.html
 * With every struct proc in use, what happens on exec()? On fork()?
 * On kill() of an existing pid followed by fork()?
 *
 * ---------------------------------------------------------------------------
 *  PREREQUISITES
 * ---------------------------------------------------------------------------
 *
 *   Kind  Read first
 *   ----  ------------------------------------------------------------------
 *   code  user/forkfill.c (filling the table with blocked children)
 *   note  docs/11-os-design-in-xv6.md
 *         #homework-3-a-full-process-table-versus-exec-fork-and-kill
 *   code  kernel/proc.c allocproc(), kfork(), kkill(), kexit(), kwait()
 *   code  kernel/exec.c kexec()
 *
 * ---------------------------------------------------------------------------
 *  RUNNING IT  (inside xv6, LAB=util, NPROC = 64)
 * ---------------------------------------------------------------------------
 *
 *    $ hwdesign
 *    hwdesign: table full: fork returned -1 after 60 children
 *    hwdesign: probe exec'd echo while the table was full
 *    hwdesign: exec used the probe's own slot; probe exited 0
 *    hwdesign: refilled 1 freed slot
 *    hwdesign: kill(65) returned 0; fork right after returned -1
 *    hwdesign: wait reaped killed pid 65, status -1
 *    hwdesign: fork after that wait succeeded
 *    hwdesign: reaped 60 children
 *
 *  60 = NPROC - init - sh - hwdesign - probe. The pids vary per run.
 *
 * ---------------------------------------------------------------------------
 *  WHAT EACH STEP SHOWS
 * ---------------------------------------------------------------------------
 *
 *   Step               Kernel path                    Result
 *   -----------------  -----------------------------  ----------------------
 *   fork() when full   allocproc() finds no UNUSED    -1; kernel keeps going
 *   exec() when full   kexec() reuses myproc()        works: needs pages,
 *                                                     not a slot
 *   kill(); fork()     kkill() only sets p->killed    -1: the victim holds
 *                                                     its slot until wait()
 *   wait(); fork()     kwait() -> freeproc()          slot UNUSED; succeeds
 *
 *  kill() is asynchronous. The victim exits only when it next heads back
 *  to user space (usertrap() in kernel/trap.c), and even then kexit()
 *  leaves a ZOMBIE that still occupies its slot. Only the parent's wait()
 *  returns the slot, so this test is deterministic: the victim is our
 *  own child and we have not waited yet.
 *
 * ---------------------------------------------------------------------------
 *  KEY POINTS
 * ---------------------------------------------------------------------------
 *
 *  * exec() replaces the caller's memory; it never touches proc[].
 *  * kill() marks a process; it frees nothing by itself.
 *  * A slot is reusable only after exit() AND the parent's wait().
 */

#include "kernel/types.h"
#include "user/user.h"

static int p[2];  // fillers block reading p[0] until p[1] closes
static int go[2]; // the probe blocks reading go[0] until signalled

// Fork blocked children until fork() fails; return how many succeeded.
static int
fill(int *lastpid)
{
  int n, pid;
  char c;

  for (n = 0;; n++) {
    pid = fork();
    if (pid < 0)
      return n;
    if (pid == 0) {
      close(p[1]);
      close(go[1]);
      read(p[0], &c, 1); // returns 0 once the parent closes p[1]
      exit(0);
    }
    *lastpid = pid;
  }
}

int
main(void)
{
  char *echoargv[] = {"echo",    "hwdesign:", "probe", "exec'd", "echo",
                      "while",   "the",       "table", "was",    "full",
                      (char *)0};
  int probe, victim, pid, st, n, r;
  char c;

  if (pipe(p) < 0 || pipe(go) < 0) {
    fprintf(2, "hwdesign: pipe failed\n");
    exit(1);
  }

  // The probe exists before the table fills, so it already owns a slot.
  probe = fork();
  if (probe < 0) {
    fprintf(2, "hwdesign: probe fork failed\n");
    exit(1);
  }
  if (probe == 0) {
    close(p[1]);
    close(go[1]);
    read(go[0], &c, 1);
    exec("echo", echoargv);
    fprintf(2, "hwdesign: exec failed\n");
    exit(1);
  }
  close(go[0]);

  // Question 2: fork() with no UNUSED slot.
  n = fill(&victim);
  printf("hwdesign: table full: fork returned -1 after %d children\n", n);

  // Question 1: exec() in a process that already has a slot.
  write(go[1], "x", 1);
  close(go[1]);
  if (wait(&st) != probe) {
    fprintf(2, "hwdesign: expected the probe to exit first\n");
    exit(1);
  }
  printf("hwdesign: exec used the probe's own slot; probe exited %d\n", st);

  n = fill(&victim); // take back the probe's slot
  printf("hwdesign: refilled %d freed slot\n", n);

  // Question 3: kill() then fork() before anyone wait()s.
  r = kill(victim);
  pid = fork();
  if (pid == 0)
    exit(0);
  printf("hwdesign: kill(%d) returned %d; fork right after returned %d\n",
         victim, r, pid);
  if (pid > 0)
    wait(0);

  pid = wait(&st);
  printf("hwdesign: wait reaped killed pid %d, status %d\n", pid, st);
  if (pid != victim)
    fprintf(2, "hwdesign: reaped %d, not the victim\n", pid);

  pid = fork();
  if (pid < 0) {
    fprintf(2, "hwdesign: fork still fails\n");
    exit(1);
  }
  if (pid == 0)
    exit(0);
  wait(0);
  printf("hwdesign: fork after that wait succeeded\n");

  close(p[1]);
  for (n = 0; wait(0) >= 0; n++)
    ;
  close(p[0]);
  printf("hwdesign: reaped %d children\n", n);
  exit(0);
}
