// ex4exec.c: replace this process with the echo executable.
//
// Based on MIT 6.1810 Lecture 1's ex4.c:
// https://pdos.csail.mit.edu/6.1810/2026/lec/l-overview/ex4.c.
// Host reference: man 3 execv.
//
// ---------------------------------------------------------------------------
//  PREREQUISITES
// ---------------------------------------------------------------------------
//
//   Kind  Read first
//   ----  ------------------------------------------------------------------
//   note  ../operating-system/The_Process_Abstraction.md  (section 7.3)
//   code  ../operating-system/codes/src/main/virtualization/cpu-process-api/
//           exec_family/exec_execv.c
//
// ---------------------------------------------------------------------------
//  RUNNING IT  (inside xv6, not on the host)
// ---------------------------------------------------------------------------
//
//    % make qemu
//    $ ex4exec
//    this is echo
//
//  user/echo.c prints that line. ex4exec itself is gone after exec succeeds.
//
// ---------------------------------------------------------------------------
//  WHAT exec() REPLACES AND PRESERVES
// ---------------------------------------------------------------------------
//
//    before exec("echo", argv)
//    +-----------------------------+
//    | instructions: ex4exec       |
//    | data + stack: argv, ...     |  <-- REPLACED
//    | heap                        |
//    +-----------------------------+
//    | pid                     3   |
//    | FD table 0,1,2 -> console   |  <-- KEPT
//    | current directory       /   |
//    | parent                  sh  |
//    +-----------------------------+
//                  |
//                  |  kexec() builds a new page table, loads the
//                  |  segments, and swaps it in as the LAST step
//                  v
//    after
//    +-----------------------------+
//    | instructions: echo          |
//    | data + stack: fresh         |  <-- REPLACED
//    | (no heap yet)               |
//    +-----------------------------+
//    | pid                     3   |
//    | FD table 0,1,2 -> console   |  <-- KEPT
//    | current directory       /   |
//    | parent                  sh  |
//    +-----------------------------+
//
//  Preserved descriptors let a caller arrange `>` or `|` before exec(); the
//  new program need not know. This is why ex6redirect.c works.
//
//  kexec() in kernel/exec.c installs the new page table as its last step,
//  so any earlier failure leaves the old program intact to receive -1.
//
// ---------------------------------------------------------------------------
//  exec(path, argv)
// ---------------------------------------------------------------------------
//
//   Argument  Is                          Rule
//   --------  --------------------------  ----------------------------------
//   path      the file to load            an ELF binary in the file system;
//                                         xv6 has no PATH search, so the
//                                         name is used exactly as given
//   argv      array of char *             MUST end with a 0 pointer; passed
//                                         through to the new main()
//
//   Return    Means
//   -------   ---------------------------------------------------------------
//   (none)    on success there is nothing to return TO -- the caller's
//             instructions no longer exist
//     -1      the file is missing, unreadable, or not a valid ELF image;
//             the original program continues at the next statement
//
//  Success destroys the caller's instructions, so the next statement is
//  already the error path; no `if` is needed.
//
// ---------------------------------------------------------------------------
//  WHERE EACH PIECE LIVES IN THIS TREE
// ---------------------------------------------------------------------------
//
//   Idea                        File
//   --------------------------  -------------------------------------------
//   exec syscall stub           user/usys.S
//   argument-vector copy-in     kernel/sysfile.c  (sys_exec())
//   ELF loading and new stack   kernel/exec.c  (kexec())
//   echo's argv loop            user/echo.c
//   executable layout           user/user.ld
//   exercise details            docs/07-exercises.md
//
// ---------------------------------------------------------------------------
//  KEY POINTS
// ---------------------------------------------------------------------------
//
//  * exec() loads a new program into the current process; it does not fork.
//  * Descriptors survive, so redirection arranged before exec() still works.
//  * Success never returns. The next statement is already the failure path.

#include "kernel/types.h"
#include "user/user.h"

int
main(void)
{
  // argv[0] names echo; 0 is the required end marker for the vector.
  char *argv[] = { "echo", "this", "is", "echo", 0 };

  // No fork is needed because ex4exec has nothing left to do. A shell forks
  // first so the parent survives to wait and print another prompt; see
  // ex5forkexec.c.
  exec("echo", argv);

  // Reaching this statement proves exec() failed.
  fprintf(2, "ex4exec: exec echo failed\n");
  return 1;
}
