// ex2createfile.c: create a file, write to it.

// TODO: $ make qemu
// (xv6 shell) $ ex2createfile
// $ ls | grep dummy
// $ cat dummy.file
#include "kernel/types.h"
#include "user/user.h"

int
main(void)
{
  char *argv[] = { "echo", "this", "is", "echo", 0 }; // why add 0?
  // exec() never returns on success, so the next line is never reached.
  exec("echo", argv);
  printf("exec() failed\n");
  return 1; // Failure
}
