clear_a:
  movl $0, %eax

bpf_ld_abs:
  movl 0x7fc0ffee(%rdi), %eax

bpf_jmp_eq:
  cmp $0xdeadbeef, %eax
  je one_target
  jmp another_target

bpf_jmp_ge:
  cmp $0xdeadbeef, %eax
  jge one_target
  jmp another_target

bpf_jmp_gt:
  cmp $0xdeadbeef, %eax
  jg one_target
  jmp another_target

bpf_ret_k:
  movl $0xdeadbeef, %eax
  ret

padding:
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;
  nop; nop; nop; nop; nop; nop; nop; nop;

one_target:
  nop

another_target:
  nop
