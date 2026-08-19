// ex4exec.c: replace this process with another program, using exec().
//
// Based on MIT 6.1810 Lecture 1's ex4.c, linked from the schedule on the
// course web site:
// https://pdos.csail.mit.edu/6.1810/2026/lec/l-overview/ex4.c.
// The host manual page for the same family is `man 3 exec`.
//
// ---------------------------------------------------------------------------
//  RUNNING IT  (inside xv6, not on the host)
// ---------------------------------------------------------------------------
//
//    % make qemu
//    $ ex4exec
//    this is echo
//    $
//
//  Read that output carefully: the program you typed printed nothing at
//  all.  `this is echo` came from user/echo.c, running in the very process
//  that started as ex4exec.  No new process was created -- `ex4exec` and
//  `echo` are two programs that took turns inside one pid.
//
// ---------------------------------------------------------------------------
//  WHAT exec() REPLACES, AND WHAT SURVIVES
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
//  The kept row is the important one.  Because exec() preserves the
//  descriptor table, a caller can rearrange descriptors FIRST and then
//  exec() a program that knows nothing about the rearrangement.  That is
//  exactly how the shell implements `>` and `|`, and it is why ex6redirect.c
//  works at all.
//
//  kexec() in kernel/exec.c does the replacing: it opens the file, checks
//  the ELF header, builds a brand-new page table, loads each program
//  segment, sets up a fresh stack holding the arguments, and only then
//  swaps the new page table in and frees the old one.  The swap is the last
//  step on purpose -- if anything fails before it, the original program is
//  still intact and exec() can return -1 into it.
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
//  "exec() does not return if successful" makes the code shape below
//  unusual: the line after exec() is the ERROR path, reached only when the
//  call failed.  No `if` is needed, and adding one would be misleading.
//
// ---------------------------------------------------------------------------
//  WHY THE ARGUMENT ARRAY ENDS IN 0
// ---------------------------------------------------------------------------
//
//  Because nothing else says how long it is.  exec() takes no count, so the
//  kernel walks the array until it finds a null pointer.  From sys_exec()
//  in kernel/sysfile.c:
//
//    for (i = 0;; i++) {
//      if (i >= NELEM(argv)) goto bad;                 // MAXARG (32) reached
//      if (fetchaddr(uargv + sizeof(uint64)*i, &uarg) < 0) goto bad;
//      if (uarg == 0) { argv[i] = 0; break; }          // <-- the terminator
//      ...
//    }
//
//  Omit the 0 and the loop keeps fetching whatever happens to follow the
//  array on the stack.  It does not run away forever -- MAXARG in
//  kernel/param.h stops it after 32 entries -- but it either copies
//  garbage as arguments or hits `bad` and returns -1.  A missing terminator
//  is therefore an exec that mysteriously fails or a program that receives
//  arguments nobody typed, which is a far worse failure than a crash.
//
//  This is the same rule as a C string's trailing NUL, one level up: an
//  array of char needs a sentinel byte, an array of char * needs a sentinel
//  pointer.
//
//  argv[0] is by convention the program's own name.  Nothing enforces it,
//  and user/echo.c ignores argv[0] entirely -- it prints argv[1] onward:
//
//    for (i = 1; i < argc; i++)
//      write(1, argv[i], strlen(argv[i]));
//
//  So `argv[0] = "echo"` is a courtesy to programs that DO look, and the
//  reason the output below starts at `this` and not at `echo`.  The kernel
//  counts the entries and hands the count to main() as argc.
//
// ---------------------------------------------------------------------------
//  WHY THIS PROGRAM IS NOT USEFUL ON ITS OWN
// ---------------------------------------------------------------------------
//
//  exec() consumed the caller.  A shell cannot work this way: if sh called
//  exec() directly it would BECOME `echo`, print, exit, and never read a
//  second command.  The fix is to exec() in a process you can afford to
//  lose -- a freshly forked child -- which is ex5forkexec.c, and which is
//  what user/sh.c actually does for every command you type.
//
// ---------------------------------------------------------------------------
//  WHERE EACH PIECE LIVES IN THIS TREE
// ---------------------------------------------------------------------------
//
//   Idea                        File
//   --------------------------  -------------------------------------------
//   the exec stub               user/usys.S  (load a7, then `ecall`)
//   sys_exec and the argv walk  kernel/sysfile.c
//   kexec(), ELF loading        kernel/exec.c
//   the ELF header layout       kernel/elf.h
//   MAXARG (32), MAXPATH        kernel/param.h
//   how a program sees argv     user/echo.c
//   the shell's exec call       user/sh.c  (runcmd(), case EXEC)
//   this exercise, written up   docs/07-exercises.md
//   the long-form notes         docs/book/ch01-operating-system-interfaces.md
//
// ---------------------------------------------------------------------------
//  THREE THINGS WORTH KEEPING
// ---------------------------------------------------------------------------
//
//  * exec() changes the PROGRAM, fork() changes the PROCESS COUNT.  They
//    are orthogonal, and Unix keeps them separate so that code can run
//    between them.
//  * exec() preserves file descriptors.  That single decision is what makes
//    shell redirection possible without any program cooperating in it.
//  * The statement after a successful exec() never runs, so it is the right
//    place for the failure message and nowhere else.

#include "kernel/types.h"
#include "user/user.h"

int
main(void)
{
  // argv[0] is the conventional program name; argv[1..3] are what echo will
  // actually print.  The trailing 0 is the terminator sys_exec() looks for
  // -- without it the kernel reads past the end of this array.
  char *argv[] = { "echo", "this", "is", "echo", 0 };

  // "echo" with no leading path: xv6 has no PATH variable and no search
  // rule, so the name is resolved by namei() against the current directory,
  // and every program sits in the root of fs.img.
  exec("echo", argv);

  // Only reachable if exec() failed -- on success this process is running
  // echo's instructions by now and these bytes are no longer mapped.
  // fd 2, because a diagnostic must not be mistaken for echo's output.
  fprintf(2, "ex4exec: exec echo failed\n");
  return 1;
}
