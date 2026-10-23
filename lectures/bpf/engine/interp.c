#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>

#include "bpf.h"

struct runner {
  struct prog *prog;
  int datalen;
};

struct runner *
bpf_prepare(struct prog *prog, int datalen)
{
  struct runner *runner = malloc(sizeof(*runner));
  runner->prog = prog;
  runner->datalen = datalen;
  return runner;
}

uint32_t
bpf_run(struct runner *runner, char *data)
{
  int pc = 0;
  uint32_t a = 0;

  while (1) {
    if (pc >= runner->prog->nins) {
      printf("pc too large\n");
      exit(1);
    }

    struct instr *ins = &runner->prog->instrs[pc];
    pc++;

    switch (ins->opcode) {
      case BPF_LD_ABS:
        if (ins->k+4 > runner->datalen || ins->k+4 < ins->k) {
          printf("out of bounds load\n");
          exit(1);
        }

        a = *(uint32_t *)(data + ins->k);
        break;

      case BPF_JMP_EQ:
        if (a == ins->k) {
          pc += ins->jt;
        } else {
          pc += ins->jf;
        }
        break;

      case BPF_JMP_GE:
        if (a >= ins->k) {
          pc += ins->jt;
        } else {
          pc += ins->jf;
        }
        break;

      case BPF_JMP_GT:
        if (a > ins->k) {
          pc += ins->jt;
        } else {
          pc += ins->jf;
        }
        break;

      case BPF_RET_K:
        return ins->k;

      default:
        printf("unknown opcode 0x%x\n", ins->opcode);
        exit(1);
    }
  }
}
