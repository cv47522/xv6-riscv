#include "kernel/types.h" // defines uint, which user/user.h uses
#include "user/user.h"

/*
 * Ref: sibling c-programming repository, under notes/src/main/
 * fundamentals/command_line_arguments/cli_arg_basic.c
 */
int
main(int argc, char *argv[])
{
  /*
   * user/sh.c builds argv; kernel/exec.c passes argc and argv to main.
   * argv[0] names sleep; argv[1] carries the requested tick count.
   */
  if (argc != 2) {
    fprintf(2, "Usage: sleep TICKS\n");
    return 1;
  }

  /*
   * Q: Why can't atoi() distinguish "0" from invalid text? Give examples.
   * A: user/ulib.c starts at 0 and reports no conversion status: "0", "abc",
   * and "" all yield 0; "3x" yields 3. A zero result alone cannot validate
   * input. See "atoi is conversion, not validation" in
   * docs/05-syscall-reference.md.
   */
  int ticks = atoi(argv[1]);
  pause(ticks);
  return 0;
}
