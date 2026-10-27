/*
 * str.c: C strings are 0-terminated char arrays, and nothing checks bounds.
 *
 * Based on MIT 6.1810 Lecture 2's str.c:
 * https://pdos.csail.mit.edu/6.1810/2026/lec/l-c/str.c.
 * Lecture text: lectures/l-c.txt, "C strings" and "common C bugs".
 *
 * ---------------------------------------------------------------------------
 *  PREREQUISITES
 * ---------------------------------------------------------------------------
 *
 *   Kind  Read first
 *   ----  ------------------------------------------------------------------
 *   note  ../c-programming/C_String.md (char[] versus char *)
 *   code  user/array.c (a[i] == *(a + i), no bounds check)
 *   note  docs/10-c-in-xv6.md (text is mapped without PTE_W)
 *
 * ---------------------------------------------------------------------------
 *  RUNNING IT  (inside xv6; addresses are from this tree)
 * ---------------------------------------------------------------------------
 *
 *    % make qemu
 *    $ str
 *    s=123
 *    s1=123
 *    s[0]=1
 *    *s=1
 *    s[2]=3
 *    *(s+2)=3
 *    s1[4]=0                         a stack byte past s1: garbage, 0 here
 *    s1=0x0000000000004FA8
 *    &s1[4]=0x0000000000004FAC
 *    s1[4] write: no fault           the overflow went unnoticed
 *    usertrap(): unexpected scause 0xf pid=3
 *                sepc=0x86 stval=0x914
 *    $                               "not reached" never printed
 *
 *  The two usertrap() lines come from the kernel, not from this program.
 *  scause 0xf is a store page fault; sepc 0x86 is the `sb` that does
 *  s[4] = 'D' (see user/str.asm); stval 0x914 is the address it tried to
 *  write: the literal "123" is at 0x910 in .rodata, so s + 4 is 0x914.
 *
 * ---------------------------------------------------------------------------
 *  TWO STRINGS, TWO PLACES
 * ---------------------------------------------------------------------------
 *
 *    0x4FA8  s1[0..3]  '1' '2' '3' 0     main's frame: RW-, PTE_U
 *    0x4FAC  s1[4]     past the end      still main's frame: write works
 *      |
 *      |     ... guard page, .data page ...
 *      |
 *    0x0914  s + 4     past the end      .rodata: R-X, no PTE_W
 *    0x0910  "123\0"   the literal       s points here
 *    0x0000  main      .text             same read-only segment
 *
 *   Write        Target page          Permission  Result
 *   -----------  -------------------  ----------  ------------------------
 *   s1[4] = 'D'  user stack           RW-         silent; undefined, and
 *                                                 may corrupt the frame
 *   s[4] = 'D'   text and .rodata     R-X         store fault; process
 *                                                 killed with status -1
 *
 *  `char s1[4] = {...}` copies the bytes into a stack array the program
 *  owns. `char *s = "123"` only stores the address of a literal that the
 *  linker put in the first LOAD segment, which kexec() maps with
 *  flags2perm() in kernel/exec.c: no ELF write flag, so no PTE_W. Any
 *  write there faults -- s[0] would too; being past the end is not what
 *  triggers it. vmfault() in kernel/vm.c refuses an already-mapped page,
 *  so usertrap() in kernel/trap.c prints the message and kills us.
 *
 *  WARNING: the s1[4] write landed in a slot the compiler left unused
 *  here. With a different frame it could overwrite a saved register or
 *  another local; C promises nothing about it.
 *
 * ---------------------------------------------------------------------------
 *  WHERE EACH PIECE LIVES IN THIS TREE
 * ---------------------------------------------------------------------------
 *
 *   Idea                           File
 *   -----------------------------  ----------------------------------------
 *   strlen() index, strcmp() ptr   user/ulib.c
 *   .rodata in the text segment    user/user.ld
 *   ELF flags to PTE bits          kernel/exec.c  flags2perm()
 *   the fault message and kill     kernel/trap.c  usertrap()
 *   exercise details               docs/07-exercises.md
 *   lecture 2 companion            docs/10-c-in-xv6.md
 *
 * ---------------------------------------------------------------------------
 *  KEY POINTS
 * ---------------------------------------------------------------------------
 *
 *  * A string is a char array whose end is marked by a 0 byte.
 *  * Out-of-bounds access is caught only by page permissions, never by C.
 *  * char[] owns writable bytes; char * to a literal points at read-only.
 *
 *  Still open: Lecture 3, kernel organization (lectures/l-os.txt).
 */

#include "kernel/types.h"
#include "user/user.h"

int
main(void)
{
  char *s = "123";
  char s1[4] = {'1', '2', '3', '\0'};

  // s and s1 are strings
  printf("s=%s\ns1=%s\n", s, s1);

  // can use index or pointer access
  printf("s[0]=%c\n*s=%c\n", s[0], *s);
  printf("s[2]=%c\n*(s+2)=%c\n", s[2], *(s + 2));

  // read beyond str end; DON'T DO THIS
  printf("s1[4]=%x\ns1=%p\n&s1[4]=%p\n", s1[4], (void *)s1, (void *)&s1[4]);

  // write beyond the stack array's end; DON'T DO THIS
  s1[4] = 'D';
  printf("s1[4] write: no fault\n");

  // write into the string literal; DON'T DO THIS
  s[4] = 'D';
  printf("not reached\n");

  exit(0);
}
