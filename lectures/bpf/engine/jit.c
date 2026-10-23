#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>

#include "bpf.h"

struct runner {
  uint32_t (*f)(char *data);
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

  // conservative: 16 bytes of x86 code per BPF instruction.
  char *code = malloc(prog->nins * 16);
  int codelen;

  // offsets in code where each instruction starts.
  int *instr_off = malloc(sizeof(int) * prog->nins);

  // Two passes: first pass fills in instr_off, second pass fills in jumps.
  for (int pass = 0; pass < 2; pass++) {
    codelen = 0;

    // Zero out the %eax register, which stores BPF's A register.
    // movl $0, %eax
    code[codelen++] = 0xb8;
    code[codelen++] = 0x00;
    code[codelen++] = 0x00;
    code[codelen++] = 0x00;
    code[codelen++] = 0x00;

    for (int i = 0; i < prog->nins; i++) {
      instr_off[i] = codelen;

      struct instr *ins = &prog->instrs[i];
      switch (ins->opcode) {
        case BPF_LD_ABS:
          if (ins->k+4 > datalen || ins->k+4 < ins->k) {
            printf("out of bounds load\n");
            exit(1);
          }
          if (ins->k > 0xffffff) {
            printf("offset too large for JIT\n");
            exit(1);
          }

          // mov K(%rdi), %eax
          code[codelen++] = 0x8b;
          code[codelen++] = 0x87;
          code[codelen++] = (ins->k >> 0) & 0xff;
          code[codelen++] = (ins->k >> 8) & 0xff;
          code[codelen++] = (ins->k >> 16) & 0xff;
          code[codelen++] = (ins->k >> 24) & 0xff;
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

          // cmp $K, %eax
          code[codelen++] = 0x3d;
          code[codelen++] = (ins->k >> 0) & 0xff;
          code[codelen++] = (ins->k >> 8) & 0xff;
          code[codelen++] = (ins->k >> 16) & 0xff;
          code[codelen++] = (ins->k >> 24) & 0xff;

          // je JT
          // jge JT
          // jg JT
          uint32_t jt_off = instr_off[i+1+ins->jt] - codelen - 6;
          code[codelen++] = 0x0f;
          switch (ins->opcode) {
            case BPF_JMP_EQ:
              code[codelen++] = 0x84;
              break;
            case BPF_JMP_GE:
              code[codelen++] = 0x8d;
              break;
            case BPF_JMP_GT:
              code[codelen++] = 0x8f;
              break;
          }
          code[codelen++] = (jt_off >> 0) & 0xff;
          code[codelen++] = (jt_off >> 8) & 0xff;
          code[codelen++] = (jt_off >> 16) & 0xff;
          code[codelen++] = (jt_off >> 24) & 0xff;

          if (ins->jf != 0) {
            // jmp JT
            uint32_t jf_off = instr_off[i+1+ins->jf] - codelen - 5;
            code[codelen++] = 0xe9;
            code[codelen++] = (jf_off >> 0) & 0xff;
            code[codelen++] = (jf_off >> 8) & 0xff;
            code[codelen++] = (jf_off >> 16) & 0xff;
            code[codelen++] = (jf_off >> 24) & 0xff;
          }

          break;

        case BPF_RET_K:
          // mov $K, %eax
          code[codelen++] = 0xb8;
          code[codelen++] = (ins->k >> 0) & 0xff;
          code[codelen++] = (ins->k >> 8) & 0xff;
          code[codelen++] = (ins->k >> 16) & 0xff;
          code[codelen++] = (ins->k >> 24) & 0xff;

          // ret
          code[codelen++] = 0xc3;
          break;

        default:
          printf("unknown opcode 0x%x\n", ins->opcode);
          exit(1);
      }
    }
  }

  void *exec_code = mmap(NULL, codelen, PROT_READ | PROT_WRITE | PROT_EXEC,
                         MAP_ANON | MAP_PRIVATE, -1, 0);
  if (exec_code == MAP_FAILED) {
    perror("mmap");
    exit(-1);
  }

  memmove(exec_code, code, codelen);

  struct runner *runner = malloc(sizeof(*runner));
  runner->f = exec_code;
  return runner;
}

uint32_t
bpf_run(struct runner *runner, char *data)
{
  return runner->f(data);
}
