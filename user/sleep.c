#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int ticks;

  // user/sh.c builds argv; kernel/exec.c passes argc and argv to main.
  // argv[0] names sleep; argv[1] carries the requested tick count.
  if (argc != 2) {
    fprintf(2, "Usage: sleep ticks\n");
    return 1;
  }

  // atoi() cannot distinguish "0" from invalid text; this lab requires only
  // the missing-argument diagnostic. See docs/labs/util-sleep.md.
  ticks = atoi(argv[1]);
  pause(ticks);
  return 0;
}
