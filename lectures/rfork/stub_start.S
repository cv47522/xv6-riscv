/*
 * stub_start.S - entry point of rfork-stub.
 *
 * The kernel gave us a stack at the top of the address space, which is
 * exactly where the parent's stack is going to be mapped.  So the very first
 * thing we do is move to a private stack inside our own .bss, which lives in
 * a corner of the address space (0x20000) that the parent does not use.
 */
        .text
        .globl  _start
        .type   _start, @function
_start:
        movq    $stub_stack_top, %rsp
        xorl    %ebp, %ebp
        call    stub_main
        movl    $60, %eax               /* exit(1) if stub_main ever returns */
        movl    $1, %edi
        syscall

        .bss
        .align  16
stub_stack:
        .skip   65536
stub_stack_top:

        .section .note.GNU-stack,"",@progbits
