#include "kernel/types.h"
#include "user/user.h"

void memdump(char *fmt, char *data, int len);

int
main(int argc, char *argv[])
{
  if (argc == 1) {
    // Layout: docs/labs/03-util-memdump.md#the-five-starter-layouts
    printf("Example 1:\n");
    int a[2] = {61810, 2026};
    // Byte-view rationale: docs/labs/03-util-memdump.md#why-char-data
    memdump("ii", (char *)a, sizeof(a));

    printf("Example 2:\n");
    memdump("S", "a string", sizeof("a string"));

    printf("Example 3:\n");
    char *s = "another";
    // Pointer-slot rationale: docs/labs/03-util-memdump.md#why-s-needs-s
    memdump("s", (char *)&s, sizeof(s));

    // ABI layout: docs/labs/03-util-memdump.md#structure-layout
    struct sss {
      char *ptr;
      int num1;
      short num2;
      char byte;
      // Initialization: docs/labs/03-util-memdump.md#array-initialization
      char bytes[8];
    } example;

    example.ptr = "hello";
    example.num1 = 1819438967;
    example.num2 = 100;
    example.byte = 'z';
    strcpy(example.bytes, "xyzzy");

    printf("Example 4:\n");
    memdump("pihcS", (char *)&example, sizeof(example));

    printf("Example 5:\n");
    memdump("sccccc", (char *)&example, sizeof(example));

    /*
     * Four properties of `S` that neither the handout's examples nor the
     * piped path can check, because every region they build already carries a
     * NUL at or just past its end. Each region below is shaped so that a wrong
     * `S` prints something visibly different; see the banner for which.
     */

    printf("Edge 1 (no NUL inside len):\n");
    char no_term[16];
    memset(no_term, 'X', sizeof(no_term));
    /*
     * A counted copy, not strcpy(): strcpy() would write a terminator at index
     * 8, the one byte this case needs to stay non-zero. memcpy() would do here
     * too -- user/ulib.c forwards it to memmove() -- but the rest of the tree
     * calls memmove().
     */
    memmove(no_term, "abcdefgh", 8);
    /*
     * Describable extent is 8, but the bytes beyond it are non-zero, so a scan
     * that trusts a terminator instead of `len` runs on into the filler.
     */
    memdump("S", no_term, 8);

    printf("Edge 2 (NUL inside len, then another item):\n");
    /*
     * Double quotes make string literals, not characters.
     * "a" is a 2-byte array {'a','\0'}.
     * Quoting rules: docs/labs/03-util-memdump.md#array-initialization
     */
    char split[5] = {'a', 'b', '\0', 'c', 'd'};
    /*
     * The terminator is consumed along with the string, or `c` re-reads 'a'.
     * The last byte therefore has to differ from the first, or both outcomes
     * print the same character.
     */
    memdump("Sc", split, sizeof(split)); // "ab" 'c'
    memdump("Scc", split, sizeof(split)); // "ab" 'c' 'd'
    memdump("SS", split, sizeof(split)); // "ab" "cd"

    printf("Edge 3 (S consumes the region, then a short item):\n");
    /*
     * 3, not 2: "ab" is three bytes, so `len` 3 puts the terminator inside the
     * region and exercises dump_string()'s `n + 1` return. Stopping at 2 would
     * exhaust the region instead -- the branch Edge 1 already covers -- and
     * would print bytes indistinguishable from an `S` that ignores the NUL.
     * The width `S` reports is what leaves `remaining` at 0 here, so this is
     * the bounds check reading a width that get_item_width() never supplied,
     * and `c` prints no character: the shortage diagnostic is the assertion.
     */
    memdump("Sc", "ab", 3);

    printf("Edge 4 (empty region):\n");
    /*
     * Zero remaining is not a shortage for `S`; it prints nothing, and must
     * reach that answer without reading a byte to discover it.
     */
    memdump("S", "", 0);
  } else if (argc == 2) {
    /*
     * format in argv[1], up to 512 bytes of data from standard input.
     * Capacity rationale: docs/labs/03-util-memdump.md#why-512-bytes
     */
    char data[512];
    /*
     * Capacity as a signed count, because every other quantity here is one.
     * Comparing `n` against sizeof()'s unsigned type would convert `n`
     * instead.
     */
    int cap = sizeof(data);
    int n = 0;
    memset(data, '\0', sizeof(data));
    while (n < cap) {
      /*
       * read() returns what one call could deliver, not what was asked for:
       * a pipe hands over whatever the writer has flushed so far. Loop until
       * it reports 0 (EOF) so a split write still arrives whole.
       */
      int nn = read(0, data + n, cap - n); // Read FD 0 (stdin)
      if (nn <= 0)
        break;
      n += nn;
    }
    // n, not sizeof(data): capacity is not content length.
    memdump(argv[1], data, n);
  } else {
    printf("Usage: memdump [format]\n");
    exit(1);
  }
  exit(0);
}

/*
 * ---------------------------------------------------------------------------
 *  MEMDUMP: WALKING ONE BOUNDED BYTE REGION
 * ---------------------------------------------------------------------------
 *
 *  `data` is a cursor, not an object. A character pointer is the only kind
 *  allowed to inspect any object's byte representation, and `+ 1` on it
 *  advances exactly one byte -- the same unit `fmt` counts in. `len` has to
 *  travel separately because a pointer carries an address and a type, never
 *  an extent.
 *  Cursor-type rationale: docs/labs/03-util-memdump.md#why-char-data
 *
 *   Fmt  Width   Bytes at the cursor are read as   Printed with
 *   ---  ------  -------------------------------   ----------------------
 *    i        4  int                               %d
 *    h        2  short                             %d
 *    c        1  char                              %c
 *    p        8  uint64                            %lx
 *    s        8  char *, then followed             %s, after the hop
 *    S   varies  characters, in place              write(), byte-counted
 *
 *  `s` and `S` differ by exactly one dereference:
 *
 *      s:  cursor --> [ 8-byte pointer slot ] --> "text"
 *      S:  cursor --> "text"
 *
 *  Two bounds rules, both of which the grader's short-input cases check:
 *
 *   1. Every fixed-width row compares `remaining` with the width BEFORE
 *      the load.
 *      Checking afterwards is not a check: the invalid read already happened.
 *   2. `S` never scans past `remaining`. Nothing promises a NUL inside the
 *      valid region, so printf's `%s` -- which stops only at a NUL -- cannot
 *      be used here, and this tree implements no bounded string conversion to
 *      fall back on: docs/05-syscall-reference.md, "printf conversions".
 *
 *  Two alternatives for `S` that both survive `echo a | memdump S`, and why
 *  neither is correct. Each satisfies one half of "through the first NUL or
 *  through the end of the valid region, whichever comes first":
 *
 *    printf("%s\n", curr)      Honors the NUL, ignores the region. `%s` in
 *                              user/printf.c is `for (; *s; s++)`, whose only
 *                              stopping condition is a NUL, so it reads past
 *                              `remaining` into whatever follows. Piped input
 *                              hides this: main() zero-fills data[512], so a
 *                              short input leaves a NUL just past the valid
 *                              bytes and the scan stops there for the wrong
 *                              reason. A region not followed by a zero byte --
 *                              Edge 1 below, or a full 512-byte read --
 *                              exposes it.
 *    write(1, curr, remaining) Honors the region, ignores the NUL. It emits
 *                              the terminator itself as output -- visible
 *                              in Edge 3 below, whose region ends in one --
 *                              trails the
 *                              indeterminate tail of `example.bytes` behind
 *                              `xyzzy`, and consumes the whole region, so a
 *                              directive after `S` has nothing left.
 *
 *  The bound is the smaller of the two, which means it has to be found before
 *  anything is printed. dump_string() also returns what it consumed because
 *  get_item_width() reports 0 for `S` and cannot know the real width.
 *
 *  Not specified by the handout, decided here: an unrecognized format
 *  character ends the dump with a diagnostic. Skipping it silently would
 *  leave the cursor describing a different item than the reader expects for
 *  every character that follows.
 * ---------------------------------------------------------------------------
 */

/*
 * Bytes a format character consumes: 0 for the variable-width 'S', and -1
 * for a character the format language does not define.
 */
static int
get_item_width(char f)
{
  switch (f) {
  case 'c':
    return sizeof(char);
  case 'h':
    return sizeof(short);
  case 'i':
    return sizeof(int);
  case 'p':
    return sizeof(uint64);
  case 's':
    /*
     * The slot holds a pointer, so size the step by what gets dereferenced
     * rather than by uint64; the two agree on this ABI but need not.
     */
    return sizeof(void *);
  case 'S':
    return 0;
  default:
    return -1;
  }
}

/*
 * Print the characters at `curr` up to the first NUL or the end of the valid
 * region, whichever comes first, and report how many bytes were consumed.
 */
static int
dump_string(char *curr, int remaining)
{
  int n = 0;

  while (n < remaining && curr[n] != '\0')
    n++;
  /*
   * write() takes an explicit count, so the bound cannot be overrun; %s and
   * strlen() would both keep going until they found a NUL somewhere. It is
   * also one syscall rather than putc()'s one write() per character.
   */
  write(1, curr, n);
  printf("\n");
  /*
   * Consume the terminator too, but only when it is inside the region.
   * Otherwise no chars are printed after '\0' for "Edge 2"
   */
  return n < remaining ? n + 1 : n;
}

void
memdump(char *fmt, char *data, int len)
{
  char *curr = data;
  int remaining = len;

  for (int i = 0; fmt[i] != '\0'; i++) {
    int width = get_item_width(fmt[i]);

    if (width < 0) {
      fprintf(2, "memdump: unknown format '%c'\n", fmt[i]);
      return;
    }

    if (remaining < width) {
      fprintf(2, "memdump: not enough data for '%c'\n", fmt[i]);
      return;
    }

    switch (fmt[i]) {
    case 'i':
      printf("%d\n", *(int *)curr);
      break;
    case 'h':
      printf("%d\n", *(short *)curr);
      break;
    case 'c':
      printf("%c\n", *curr);
      break;
    case 'p':
      /*
       * %lx, not %x: user/printf.c reads %x as a uint32 and would drop the
       * high four bytes of the value.
       */
      printf("%lx\n", *(uint64 *)curr);
      break;
    case 's':
      /*
       * One hop. The cursor names the slot; the slot holds the address.
       * `len` covers the slot, not the string it reaches: the target lies
       * outside the region entirely, so this is the one row whose output is
       * unbounded. Following the pointer is what `s` means.
       */
      printf("%s\n", *(char **)curr);
      break;
    case 'S':
      /*
       * The only row whose width is not known until the bytes are examined,
       * so the helper reports it. Rejected alternatives: see the banner.
       */
      width = dump_string(curr, remaining);
      break;
    }

    curr += width;
    remaining -= width;
  }
}
