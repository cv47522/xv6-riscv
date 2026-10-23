/*
 * rfork_ctx.S - save the registers that the child needs to "return" from rfork().
 *
 *   int rfork_save_context(struct rfork_ctx *ctx);
 *
 * Works like setjmp: returns 0 to the parent.  The stub on the other machine
 * loads these registers, sets rax = 1 and jumps to ctx->rip, so the same call
 * appears to return 1 in the child.
 *
 * Only the callee-saved registers matter: everything else is dead across a
 * function call by the x86-64 ABI, and all memory (including the stack frame
 * of the caller) comes from the parent's snapshot.
 */
        .text
        .globl  rfork_save_context
        .type   rfork_save_context, @function
rfork_save_context:
        movq    %rbx,   0(%rdi)
        movq    %rbp,   8(%rdi)
        movq    %r12,  16(%rdi)
        movq    %r13,  24(%rdi)
        movq    %r14,  32(%rdi)
        movq    %r15,  40(%rdi)
        leaq    8(%rsp), %rax           /* rsp as it will be after 'ret' */
        movq    %rax,  48(%rdi)
        movq    (%rsp), %rax            /* return address */
        movq    %rax,  56(%rdi)
        xorl    %eax, %eax              /* parent sees 0 */
        ret
        .size   rfork_save_context, .-rfork_save_context

        .section .note.GNU-stack,"",@progbits
