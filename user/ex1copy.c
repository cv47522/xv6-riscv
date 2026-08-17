// ex1copy.c: copy standard input to standard output until EOF.
// Based on MIT 6.1810 Lecture 1's ex1.c:
// https://pdos.csail.mit.edu/6.1810/2026/lec/l-overview/ex1.c.

#include "kernel/types.h"
#include "user/user.h"

int
main(void)
{
  char buf[64];
  int n;

  // File descriptors 0, 1, and 2 are standard input, standard output, and
  // standard error. On the console, Enter makes one line available. After
  // buffered console input is consumed, another read waits. Ctrl-d at an
  // empty console input position returns zero for EOF; finite files and
  // closed input pipes return EOF instead of waiting.
  while ((n = read(0, buf, sizeof(buf))) > 0) {
    if (write(1, buf, n) != n) {
      fprintf(2, "ex1copy: write error\n");
      exit(1);
    }
  }
  if (n < 0) {
    fprintf(2, "ex1copy: read error\n");
    exit(1);
  }
  exit(0);
}
