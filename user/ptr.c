/*
 * ptr.c: take addresses, write through pointers, and print where things live.
 *
 * Based on MIT 6.1810 Lecture 2's ptr.c:
 * https://pdos.csail.mit.edu/6.1810/2026/lec/l-c/ptr.c.
 * Lecture text: lectures/l-c.txt, "C pointers" and "ls.c / argv".
 *
 * ---------------------------------------------------------------------------
 *  PREREQUISITES
 * ---------------------------------------------------------------------------
 *
 *   Kind  Read first
 *   ----  ------------------------------------------------------------------
 *   note  ../c-programming/C_Pointers.md
 *   note  ../c-programming/C_Pointer_Declarations.md (int **, int (*)())
 *   note  docs/10-c-in-xv6.md (user address space, argv on the stack)
 *
 * ---------------------------------------------------------------------------
 *  RUNNING IT  (inside xv6; addresses are from this tree, LAB=util)
 * ---------------------------------------------------------------------------
 *
 *    % make qemu
 *    $ ptr x yz
 *    p=0x0000000000001000           &g: .data, first page after text
 *    q=0x0000000000004F5C           &l: the stack, just below argv
 *    g=11
 *    l=13
 *    *c=11                          one byte of g: little-endian low byte
 *    st.a=10
 *    st_ptr->a=10
 *    pp=0x0000000000004F50          &p: a pointer has an address too
 *    *pp=0x0000000000001000         == p
 *    **pp=11                        == g
 *    func=0x0000000000000000        main is linked at address 0
 *    argv=0x0000000000004FB0
 *    argv[0]=0x0000000000004FF0 "ptr"
 *    argv[1]=0x0000000000004FE0 "x"
 *    argv[2]=0x0000000000004FD0 "yz"
 *    argv[3]=0x0000000000000000     the terminator kexec() wrote
 *
 *  `p` and `func` never move: the linker fixed them (`nm -n user/_ptr`
 *  prints `0000000000001000 D g` and `0000000000000000 T main`). Every
 *  stack address moves with the arguments: plain `ptr` prints
 *  q=0x...4F8C and argv=0x...4FE0, because fewer strings sit above them.
 *
 * ---------------------------------------------------------------------------
 *  WHERE EACH PRINTED ADDRESS LIVES  (top of the user address space down)
 * ---------------------------------------------------------------------------
 *
 *    0x5000  sz: top of the stack, initial sp in kernel/exec.c kexec()
 *      |
 *    0x4FF0  "ptr\0"       each string 16-byte aligned (sp -= sp % 16)
 *    0x4FE0  "x\0"
 *    0x4FD0  "yz\0"
 *    0x4FB0  argv[0..3]    pointer array; argv[3] == 0; a1 = this address
 *      |
 *    0x4F5C  l             main's frame: l, p, q, pp, st
 *    0x4F50  p             (pp holds this address)
 *      |     ... rest of the USERSTACK pages (2 under LAB_UTIL) ...
 *    0x2000  guard page    uvmclear() drops PTE_U; touching it faults
 *    0x1000  .data: g      user/user.ld aligns .data to a page
 *    0x0000  .text: main   user/user.ld starts at 0x0, main first
 *
 * ---------------------------------------------------------------------------
 *  POINTER EXPRESSIONS IN THIS FILE
 * ---------------------------------------------------------------------------
 *
 *   Expression  Type              Value is            Reads memory?
 *   ----------  ----------------  ------------------  ----------------------
 *   &g          int *             g's address         no
 *   *p = 11     int (lvalue)      writes 4 bytes      writes at p
 *   *(char *)p  char              g's low byte        1 byte at p
 *   st_ptr->a   int               (*st_ptr).a         4 bytes at st_ptr + 0
 *   &p          int **            p's own address     no
 *   **pp        int               g                   8 bytes, then 4
 *   &main       int (*)(int,      main's entry        no
 *               char **)
 *
 *  The type is a hint to the compiler about how wide a load is and how far
 *  `+ 1` moves; it says nothing about what is really at the address.
 *  `*(char *)p` reads 11 because RISC-V is little-endian: the low byte of
 *  the int comes first in memory.
 *
 * ---------------------------------------------------------------------------
 *  WHERE EACH PIECE LIVES IN THIS TREE
 * ---------------------------------------------------------------------------
 *
 *   Idea                           File
 *   -----------------------------  ----------------------------------------
 *   text at 0x0, .data page        user/user.ld
 *   start() calls exit(main())     user/ulib.c
 *   argv strings and array         kernel/exec.c  kexec()
 *   a0 = argc, a1 = argv           kernel/exec.c, docs/riscv-calling.pdf
 *   %p prints 16 hex digits        user/printf.c  printptr()
 *   exercise details               docs/07-exercises.md
 *   lecture 2 companion            docs/10-c-in-xv6.md
 *
 * ---------------------------------------------------------------------------
 *  KEY POINTS
 * ---------------------------------------------------------------------------
 *
 *  * A pointer is an address; its type only picks load width and scaling.
 *  * Globals have link-time addresses; locals and argv depend on the call.
 *  * A valid function pointer here equals 0, so `if (func)` would lie.
 *
 *  Next: str.c (strings, and the overflow it deliberately commits).
 */

#include "kernel/types.h"
#include "user/user.h"

int g = 3; // global: .data, so `nm -n user/_ptr` shows its address

int
main(int argc, char *argv[])
{
  int l = 5; // local: on the user stack; uninitialized locals hold garbage
  int *p, *q;

  // take address of variable
  p = &g;
  q = &l;
  printf("p=%p\nq=%p\n", (void *)p, (void *)q);

  // assign using pointer
  *p = 11;
  *q = 13;
  printf("g=%d\nl=%d\n", g, l);

  /*
   * Q: Why does printf("*c=%d") need no cast like *(int *)c?
   * A: The cast already happened once, on the line that set c. After it,
   * c is a char *, so *c is a 1-byte char. Passing a char to a variadic
   * function promotes it to int (the default argument promotions), which
   * is exactly what %d reads. *(int *)c would undo the point of the demo:
   *
   *   Expression   Bytes read at 0x1000   Prints
   *   -----------  ---------------------  ------
   *   *c           1                      11, only because 11 < 256
   *   *(int *)c    4                      11, the whole of g
   *
   * Set g = 256 and they differ: *c prints 0 (the low byte of 0x100).
   */
  char *c = (char *)p;
  printf("*c=%d\n", *c);

  // struct
  struct two {
    int a;
    int b;
  } st;
  st.a = 10;
  struct two *st_ptr = &st;
  printf("st.a=%d\nst_ptr->a=%d\n", st.a, st_ptr->a);

  // can take address of any variable
  int **pp;
  pp = &p; // take address of a pointer variable
  printf("pp=%p\n*pp=%p\n**pp=%d\n", (void *)pp, (void *)*pp, **pp);

  int (*func)(int, char **);
  func = &main; // take address of a function
  // -pedantic rejects function-to-void* casts; go through an integer.
  printf("func=%p\n", (void *)(uint64)func);

  // argv points at the pointer array kexec() built; argv[argc] is 0.
  printf("argv=%p\n", (void *)argv);
  for (int i = 0; i < argc; i++)
    printf("argv[%d]=%p \"%s\"\n", i, (void *)argv[i], argv[i]);
  printf("argv[%d]=%p\n", argc, (void *)argv[argc]);

  exit(0);
}
