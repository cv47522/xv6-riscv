#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"

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

    printf("Edge 1:\n");
    char str[4] = {'a', 'b', '\0', 'c'};
    memdump("Sc", str, sizeof(str));

    printf("Edge 2:\n");
    memdump("S", "", 0);

    printf("Edge 3:\n");
    memdump("Sc", "ab", 3);

    printf("Edge 4:\n");
    char str2[8];
    memset(str2, 'X', sizeof(str2));
    memcpy(str2, "abcdefg", 7);
    memdump("S", str2, 7);
  } else if (argc == 2) {
    /*
     * format in argv[1], up to 512 bytes of data from standard input.
     * Capacity rationale: docs/labs/03-util-memdump.md#why-512-bytes
     */
    char data[512];
    int n = 0;
    memset(data, '\0', sizeof(data));
    while (n < sizeof(data)) {
      /*
       * read() returns what one call could deliver, not what was asked for:
       * a pipe hands over whatever the writer has flushed so far. Loop until
       * it reports 0 (EOF) so a split write still arrives whole.
       */
      int nn = read(0, data + n, sizeof(data) - n); // Read FD 0 (stdin)
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


static int
get_item_width(char c)
{
  switch (c) {
    case 'c':
      return sizeof(char);
    case 'h':
      return sizeof(short);
    case 'i':
      return sizeof(int);
    case 'p':
      return sizeof(long);
    case 's':
      return sizeof(void *);
    case 'S':
      return 0;
    default:
      return -1;
  }
}


static int
print_and_get_string_width(char *data, int remaining) {
  int n = 0;

  for (int i = 0; n < remaining && data[i] != '\0'; i++)
    n++;

  write(1, data, n);
  printf("\n");
  return n;
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
      case 'c':
        printf("%c\n", *curr);
        break;
      case 'h':
        printf("%d\n", *(short *)curr);
        break;
      case 'i':
        printf("%d\n", *(int *)curr);
        break;
      case 'p':
        printf("%lx\n", *(long *)curr);
        break;
      case 's':
        printf("%s\n", *(char **)curr);
        break;
      case 'S':
        /*
         * Q: Why does echo a | mem S seem to work with printf("%s", curr)?
         * A: main() zero-fills data, so this short input happens to have NUL
         * just beyond n. %s in user/printf.c ignores remaining and reads that
         * byte; full or nonterminated regions can overread. Omitting \n also
         * removes the output line ending, not the bounds problem. See
         * "Why a short echo can hide an overread" in
         * docs/labs/03-util-memdump.md.
         */
        width = print_and_get_string_width(curr, remaining);
        break;
    }
    curr += width;
    remaining -= width;
  }
}
