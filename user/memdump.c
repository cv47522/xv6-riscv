#include "kernel/types.h"
#include "user/user.h"

void memdump(char *fmt, char *data, int len);

int
main(int argc, char *argv[])
{
  if (argc == 1) {
    printf("Example 1:\n");
    memdump("ii", "abcdefgh", sizeof("abcdefgh"));

    printf("Example 2:\n");
    memdump("ii", "a", sizeof("a")); // 2 bytes: first 'i' is already short

    // Layout: docs/labs/03-util-memdump.md#the-five-starter-layouts
    printf("Example 3:\n");
    int a[2] = {61810, 2026};
    // Byte-view rationale: docs/labs/03-util-memdump.md#why-char-data
    memdump("ii", (char *)a, sizeof(a));

    printf("Example 4:\n");
    memdump("S", "a string", sizeof("a string"));

    printf("Example 5:\n");
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

    printf("Example 6:\n");
    memdump("pihcS", (char *)&example, sizeof(example));

    printf("Example 7:\n");
    memdump("sccccc", (char *)&example, sizeof(example));

    /*
     * These four `S` probes expose errors hidden by the handout examples and
     * piped input, whose regions have NUL at or just past the end. Each wrong
     * implementation prints visibly different output; see the banner.
     */

    printf("Edge 1 (no NUL inside len):\n");
    char no_term[16];
    memset(no_term, 'X', sizeof(no_term));
    /*
     * strcpy() would write a terminator at index 8, which must stay nonzero. memcpy()
     * also works (user/ulib.c forwards it to memmove()); memmove() matches
     * the rest of the tree.
     */
    memmove(no_term, "abcdefgh", 8);
    /*
     * 8, not 16: the 'X' bytes stand for memory just past the region. They
     * must exist and be non-zero so a scan that ignores len prints them;
     * passing 16 would make them valid data and the probe would test nothing.
     */
    memdump("S", no_term, 8);

    printf("Edge 2 (NUL inside len, then another item):\n");
    /*
     * Double quotes make string literals, not characters: "a" is {'a','\0'},
     * a 2-byte array.
     * Quoting rules: docs/labs/03-util-memdump.md#array-initialization
     */
    char split[5] = {'a', 'b', '\0', 'c', 'd'};
    /*
     * Consume the string and terminator, or `c` re-reads 'a'. The last byte
     * must differ from the first to distinguish the outcomes.
     */
    memdump("Sc", split, sizeof(split)); // "ab" 'c'
    memdump("Scc", split, sizeof(split)); // "ab" 'c' 'd'
    memdump("SS", split, sizeof(split)); // "ab" "cd"

    printf("Edge 3 (S consumes the region, then a short item):\n");
    /*
     * len 3 includes "ab"'s terminator and tests dump_string()'s n + 1 return.
     * len 2 would test region instead (Edge 1) -- and would print bytes
     * as an `S` that ignores NUL. `S` supplies the consumed width, unknown to
     * get_item_width(), leaving remaining == 0. The next `c` prints nothing;
     * its shortage diagnostic tests that width through the bounds check.
     */
    memdump("Sc", "ab", 3);

    printf("Edge 4 (empty region):\n");
    /*
     * With zero remaining, `S` must print nothing without reading or reporting
     * a shortage.
     */
    memdump("S", "", 0);
  } else if (argc == 2) {
    /*
     * format in argv[1], up to 512 bytes of data from standard input.
     * Capacity rationale: docs/labs/03-util-memdump.md#why-512-bytes
     */
    char buf[512] = {0};
    /*
     * Keep capacity signed like the other counts; comparing n with unsigned
     * sizeof() would convert n instead.
     *
     * user/echo.c writes its argument and "\n" separately, so one pipe read
     * may miss later bytes. Input beyond 512 bytes is left unread and ignored.
     */
    int cap = sizeof(buf);
    int total = 0;
    while (total < cap) {
      /*
       * A pipe read may return only bytes written so far, not the requested
       * count. Loop until EOF (0) to collect split writes.
       */
      int n = read(0, buf + total, cap - total); // Read FD 0 (stdin), advance cursor in buffer for next read w/ `buf + total`
      if (n <= 0)
        break;
      total += n;
    }
    // total, not sizeof(buf): capacity is not content length.
    memdump(argv[1], buf, total);
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
 *  `data` is a byte cursor. Only character pointers may inspect any object's
 *  representation; + 1 advances one byte, the unit `fmt` counts. A pointer
 *  carries an address and type, not an extent, so `len` travels separately.
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
 *   1. Compare `remaining` with each fixed width BEFORE loading; checking
 *      afterwards cannot prevent an invalid read.
 *   2. `S` never scans past `remaining`. Nothing promises a NUL inside the
 *      valid region, so printf's `%s` -- which stops only at a NUL -- cannot
 *      be used here, and this tree implements no bounded string conversion to
 *      fall back on: docs/05-syscall-reference.md, "printf conversions".
 *
 *  Two incorrect `S` alternatives pass `echo a | memdump S` but each enforces
 *  only one of the required bounds (first NUL or region end, whichever comes
 *  first):
 *
 *    printf("%s\n", curr)      Ignores the region: user/printf.c's
 *                              `for (; *s; s++)` stops only at NUL, possibly
 *                              past remaining. main() zero-fills its 512-byte
 *                              buffer, hiding this for short piped input with
 *                              a NUL just beyond the valid bytes. Nonzero
 *                              following bytes (Edge 1 above or a full
 *                              512-byte read) expose the overread.
 *    write(1, curr, remaining) Ignores NUL: emits the terminator (Edge 3
 *                              above), then any indeterminate tail after
 *                              `xyzzy` in example.bytes. Consumes the whole
 *                              region, leaving nothing for later directives.
 *
 *  Find the smaller bound before printing. dump_string() returns the consumed
 *  width because get_item_width() cannot know it and reports 0 for `S`.
 *
 *  Local policy, unspecified by the handout: an unknown format character ends
 *  the dump with a diagnostic; silently skipping it would misalign the cursor
 *  with the reader's expectations for every following item.
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
     * Step by the pointer type, not uint64: their sizes agree on this ABI
     * but need not elsewhere.
     */
    return sizeof(void *);
  case 'S':
    return 0;
  default:
    return -1;
  }
}

/*
 * Print from curr to the first NUL or region end, whichever comes first;
 * return the consumed byte count.
 */
static int
dump_string(char *curr, int remaining)
{
  int n = 0;

  while (n < remaining && curr[n] != '\0')
    n++;
  /*
   * write()'s count enforces the bound; %s and strlen() scan until NUL.
   * One write() also avoids putc()'s syscall per character.
   */
  write(1, curr, n);
  printf("\n");
  /*
   * Consume an in-region terminator so later directives reach the following
   * characters (Edge 2).
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
        printf("%d\n", *(int *)curr); // Prints: "1684234849", "1751606885"
        // printf("%c\n", *(data + byte_counter)); // Prints: "a", "e"
        // printf("%d\n", *(data + byte_counter)); // Prints: "97 (a)", "101 (e)"
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
         * `s` follows one pointer: curr names the slot, which holds the
         * string's address. len bounds the slot, not its external target;
         * this is the only format with unbounded output.
         */
        printf("%s\n", *(char **)curr);
        break;
      case 'S':
        /*
         * Only `S` needs inspection to find its width; the helper reports it.
         * Rejected alternatives: see the banner.
         */
        width = dump_string(curr, remaining);
        break;
    }

    curr += width;
    remaining -= width;
  }
}
