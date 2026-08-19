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
  int status = 0; // Why need it?
  int pid = fork();
  if (pid == 0) {
    char *argv[] = { "echo", "THIS", "IS", "ECHO", 0 };
    exec("echo", argv);
    printf("exec failed!\n");
    exit(1); // Failure
  } else {
    printf("parent waiting\n");
    wait(&status);
    printf("the child exited with status %d\n", status);
  }
  return 0;
}
