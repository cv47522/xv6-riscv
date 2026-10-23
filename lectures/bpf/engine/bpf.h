struct instr {
  uint16_t opcode;
  uint8_t jt;
  uint8_t jf;
  uint32_t k;
};

struct prog {
  struct instr *instrs;
  int nins;
};

#define BPF_RET_K   0x06
#define BPF_LD_ABS  0x20
#define BPF_JMP_EQ  0x15
#define BPF_JMP_GE  0x35
#define BPF_JMP_GT  0x25

struct prog* bpf_load(char *pn);
void bpf_print_instr(struct instr *ins);
void bpf_print_prog(struct prog *prog);

struct runner;
struct runner *bpf_prepare(struct prog *prog, int datalen);
uint32_t bpf_run(struct runner *runner, char *data);
