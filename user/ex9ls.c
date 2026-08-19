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
  int fds[2];
  char buffer[100];
  int n;

  // create a pipe, with two FDs in fds[0], fds[1].
  pipe(fds);

  // write to the pipe
  write(fds[1], "xyz\n", 4);

  // read from the pipe
  n = read(fds[0], buffer, sizeof(buffer));

  // display the results on the terminal
  write(1, buffer, n);

  return 0; // Success
}
