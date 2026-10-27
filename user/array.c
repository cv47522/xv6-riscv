/*
 * array.c: index an array two ways and watch pointer arithmetic scale.
 *
 * Based on MIT 6.1810 Lecture 2's array.c:
 * https://pdos.csail.mit.edu/6.1810/2026/lec/l-c/array.c.
 * Lecture text: lectures/l-c.txt, "C arrays".
 *
 * ---------------------------------------------------------------------------
 *  PREREQUISITES
 * ---------------------------------------------------------------------------
 *
 *   Kind  Read first
 *   ----  ------------------------------------------------------------------
 *   note  ../c-programming/C_Array.md (array[i] == *(array + i))
 *   note  ../c-programming/C_Pointer_Arithmetic.md
 *   code  user/ptr.c (addresses and the user address space)
 *
 * ---------------------------------------------------------------------------
 *  RUNNING IT  (inside xv6; addresses are from this tree)
 * ---------------------------------------------------------------------------
 *
 *    % make qemu
 *    $ array
 *    First element is at index 0: a[0]=1
 *    a[1]=3
 *    a[2]=5
 *    a=0x0000000000001010
 *    a+1=0x0000000000001014
 *    a+2=0x0000000000001018
 *    &a[2]=0x0000000000001018
 *    b=0x0000000000001000
 *    b+1=0x0000000000001001
 *
 *    % riscv64-linux-gnu-nm -n user/_array | grep ' D '
 *    0000000000001000 D b
 *    0000000000001010 D a
 *
 *  The addresses are known before the program runs: both arrays are
 *  initialized globals with link-time addresses. b lands below a although
 *  a is declared first: GCC puts objects of 8 bytes or less in .sdata (b
 *  is 3) and the rest in .data (a is 12), and user/user.ld emits .sdata
 *  first, then ALIGN(16), then .data. Declared as locals instead, they
 *  would print stack addresses near 0x4Fxx that shift with argv.
 *
 * ---------------------------------------------------------------------------
 *  MEMORY  (.data page, low address at the top)
 * ---------------------------------------------------------------------------
 *
 *    0x1000  b[0] 'a'   <- b
 *    0x1001  b[1] 'b'   <- b + 1     char: + 1 moves 1 byte
 *    0x1002  b[2] 'c'
 *    0x1003  ...        gap before ALIGN(16) in user/user.ld; b[3]
 *      |                compiles and reads a byte no array owns
 *    0x1010  a[0] 1     <- a
 *    0x1014  a[1] 3     <- a + 1     int: + 1 moves sizeof(int) = 4
 *    0x1018  a[2] 5     <- a + 2 == &a[2]
 *
 * ---------------------------------------------------------------------------
 *  TWO SPELLINGS OF ONE ACCESS
 * ---------------------------------------------------------------------------
 *
 *   Index form  Pointer form  Meaning
 *   ----------  ------------  ---------------------------------------------
 *   a[0]        *a            the first element
 *   a[i]        *(a + i)      element i; C defines the first as the second
 *   &a[i]       a + i         address of element i
 *
 *  An array name in an expression decays to a pointer to its first
 *  element, so `a + 2` is computed as address(a) + 2 * sizeof(int). There
 *  is no bounds check and no length stored anywhere: a[3] compiles and
 *  silently touches whatever follows.
 *
 * ---------------------------------------------------------------------------
 *  WHERE EACH PIECE LIVES IN THIS TREE
 * ---------------------------------------------------------------------------
 *
 *   Idea                           File
 *   -----------------------------  ----------------------------------------
 *   .data placement and alignment  user/user.ld
 *   strlen() uses index access     user/ulib.c
 *   strcmp() uses pointer access   user/ulib.c
 *   exercise details               docs/07-exercises.md
 *   lecture 2 companion            docs/10-c-in-xv6.md
 *
 * ---------------------------------------------------------------------------
 *  KEY POINTS
 * ---------------------------------------------------------------------------
 *
 *  * An array is contiguous same-type elements: no length, no bounds check.
 *  * a[i] and *(a + i) are the same expression.
 *  * Pointer arithmetic counts elements, not bytes.
 *
 *  Next: str.c, where an array of char gains a terminating 0.
 */

#include "kernel/types.h"
#include "user/user.h"

/*
 * Q: Why are a and b outside main()?
 * A: Because the lecture's array.c declares them there, and it changes
 * what the addresses show:
 *
 *   Declared    Lives in      Address known      Printed here
 *   ----------  ------------  -----------------  -----------------------
 *   globally    .data         at link time (nm)  0x1000 and 0x1010
 *   in main()   user stack    only at run time   near 0x4Fxx, moves with
 *                                                the size of argv
 *
 * As globals, `nm -n user/_array` confirms the printed values before the
 * program runs, and grade-exercises checks exactly that. Either way the
 * arithmetic is identical: a + 1 moves 4 bytes and b + 1 moves 1.
 */
int a[3] = {1, 2, 3};        // an array of 3 int's, in .data
char b[3] = {'a', 'b', 'c'}; // an array of 3 char's, in .data

int
main(void)
{
  printf("First element is at index 0: a[0]=%d\n", a[0]);

  a[1] += 1;    // index access
  *(a + 2) = 5; // pointer access
  printf("a[1]=%d\na[2]=%d\n", a[1], a[2]);

  // pointers to array elements
  printf("a=%p\na+1=%p\na+2=%p\n&a[2]=%p\n", (void *)a, (void *)(a + 1),
         (void *)(a + 2), (void *)&a[2]);

  // pointer arithmetic uses type
  printf("b=%p\nb+1=%p\n", (void *)b, (void *)(b + 1));

  exit(0);
}
