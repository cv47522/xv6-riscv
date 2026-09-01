#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int i;

  // user/sh.c builds argv; kernel/exec.c passes argc and argv to main.
  // argv[0] names echo, so output starts at argv[1].
  for (i = 1; i < argc; i++) {
    write(1, argv[i], strlen(argv[i]));
    if (i + 1 < argc) {
      write(1, " ", 1);
    } else {
      write(1, "\n", 1);
    }
  }
  exit(0);
}
