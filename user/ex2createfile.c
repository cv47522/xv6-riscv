// ex2createfile.c: create a file, write to it.

#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "user/user.h"

int
main(void)
{
  int fd = open("dummy.file", O_WRONLY | O_CREATE | O_TRUNC);
  printf("open() returned fd %d\n", fd);

  write(fd, "Hello\n", 6); // Write the six bytes before the string terminator.
  return 0;
}
