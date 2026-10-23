#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>

#include "bpf.h"

struct runner {
  struct prog *prog;
};

struct runner *
bpf_prepare(struct prog *prog, int datalen)
{
  // Check that the last instruction is a return
  if (prog->nins == 0) {
    printf("no return instruction\n");
    exit(1);
  }

  if (prog->instrs[prog->nins-1].opcode != BPF_RET_K) {
    printf("last instruction not a return\n");
    exit(1);
  }

  for (int i = 0; i < prog->nins; i++) {
    struct instr *ins = &prog->instrs[i];
    switch (ins->opcode) {
      case BPF_LD_ABS:
        if (ins->k+4 > datalen || ins->k+4 < ins->k) {
          printf("out of bounds load\n");
          exit(1);
        }
        break;

      case BPF_JMP_EQ:
      case BPF_JMP_GE:
      case BPF_JMP_GT:
        if (i + 1 + ins->jt >= prog->nins) {
          printf("jt beyond end of program\n");
          exit(1);
        }
        if (i + 1 + ins->jf >= prog->nins) {
          printf("jf beyond end of program\n");
          exit(1);
        }
        break;

      case BPF_RET_K:
        break;

      default:
        printf("unknown opcode 0x%x\n", ins->opcode);
        exit(1);
    }
  }

  struct runner *runner = malloc(sizeof(*runner));
  runner->prog = prog;
  return runner;
}

uint32_t
bpf_run(struct runner *runner, char *data)
{
  int pc = 0;
  uint32_t a = 0;

  while (1) {
    struct instr *ins = &runner->prog->instrs[pc];
    pc++;

    switch (ins->opcode) {
      case BPF_LD_ABS:
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
    }
  }
}
