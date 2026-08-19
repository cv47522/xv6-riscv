// ex2createfile.c: create a file, write to it.

// TODO: $ make qemu
// (xv6 shell) $ ex2createfile
// $ ls | grep dummy
// $ cat dummy.file
#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "user/user.h"

int
main(void)
{
  int pid = fork();
  if (pid == 0) {
    close(1); // Why clos stdout?
    open("dummy.out", O_WRONLY | O_CREATE | O_TRUNC);

    char *argv[] = { "echo", "ex6's", "redirected", "echo", 0 };
    exec("echo", argv);
    printf("exec failed!\n");
    exit(1); // Failure
  } else {
    printf("parent waiting\n");
    wait((int *)0); // Why cast it? Why not use &status?
  }
  return 0;
}
