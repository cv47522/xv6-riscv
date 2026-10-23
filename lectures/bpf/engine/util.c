#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>

#include "bpf.h"

struct prog*
bpf_load(char *pn)
{
  int fd = open(pn, O_RDONLY);
  if (fd < 0) {
    perror("open");
    exit(1);
  }

  struct prog *p = malloc(sizeof(*p));
  p->instrs = 0;
  p->nins = 0;

  while (1) {
    struct instr i;
    int cc = read(fd, &i, sizeof(i));
    if (cc != sizeof(i))
      break;

    p->nins++;
    p->instrs = realloc(p->instrs, p->nins * sizeof(struct instr));
    p->instrs[p->nins-1] = i;
  }

  close(fd);
  return p;
}

void
bpf_print_instr(struct instr *i)
{
  printf("opcode 0x%x, jt %d, jf %d, k 0x%x\n", i->opcode, i->jt, i->jf, i->k);
}

void
bpf_print_prog(struct prog *p)
{
  for (int i = 0; i < p->nins; i++) {
    printf("%d: ", i);
    bpf_print_instr(&p->instrs[i]);
  }
}
