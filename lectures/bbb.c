#include "kernel/types.h"
#include "user/user.h"

int
main(int ac, char **av)
{
  int x = 0;

  while (1) {
    x++;

    if ((x % 1000000) == 0)
      printf("b");
  }
}
