// ex3.c: create a new process with fork()

// TODO: $ make qemu
// (xv6 shell) $ ex2createfile
// $ ls | grep dummy
// $ cat dummy.file
#include "kernel/types.h"
#include "user/user.h"

int
main(void)
{
  int pid = fork();
  printf("fork() returned pid %d\n", pid);

  if (pid == 0) {
    printf("child: hello from child\n");
  } else { // pid is child pid.
    printf("parent: hello from parent\n");
  }
  return 0;
}
