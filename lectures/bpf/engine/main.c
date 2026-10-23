#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>

#include "bpf.h"

int
main(int argc, char **argv)
{
  if (argc < 2) {
    printf("Usage: %s code.bpf\n", argv[0]);
    exit(1);
  }

  struct prog *prog = bpf_load(argv[1]);
  // bpf_print_prog(prog);

  uint32_t buf[32];
  buf[0] = 0x00000001;  // syscall number
  buf[1] = 0xc000003e;  // arch
  buf[4] = 0x00000002;  // arg# 0, low 32 bits
  buf[5] = 0x00000000;  // arg# 0, high 32 bits

  struct runner *runner = bpf_prepare(prog, sizeof(buf));

  uint32_t result;
  for (int i = 0; i < 100*1000*1000; i++) {
    result = bpf_run(runner, (char *) buf);
  }
  printf("bpf_run: 0x%x\n", result);
}
