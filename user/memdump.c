#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"

// Q: Where imple pipe for example like `$ echo a | memdump i`? How does memdump read `a` from pipe when main() reads `i` from  stdin instead?

void memdump(char *fmt, char *data, int len);

int
main(int argc, char *argv[])
{
  if (argc == 1) {
    // Layout: docs/labs/util-memdump.md#the-five-starter-layouts
    printf("Example 1:\n");
    int a[2] = {61810, 2026};
    // Byte-view rationale: docs/labs/util-memdump.md#why-char-data
    memdump("ii", (char *)a, sizeof(a));

    printf("Example 2:\n");
    memdump("S", "a string", sizeof("a string"));

    printf("Example 3:\n");
    char *s = "another";
    // Pointer-slot rationale: docs/labs/util-memdump.md#why-s-needs-s
    memdump("s", (char *)&s, sizeof(s)); // Q: Isn't s already of type char*?

    // ABI layout: docs/labs/util-memdump.md#structure-layout
    struct sss {
      char *ptr;
      int num1;
      short num2;
      char byte;
      // Initialization: docs/labs/util-memdump.md#array-initialization
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
    memdump("sccccc", (char *)&example, sizeof(example)); // Q: Why didn't memdump print example.num*?
  } else if (argc == 2) {
    // format in argv[1], up to 512 bytes of data from standard input.
    // Capacity rationale: docs/labs/util-memdump.md#why-512-bytes
    char data[512];
    int n = 0;
    memset(data, '\0', sizeof(data));
    while (n < sizeof(data)) {
      // Q: Why keep reading? Doesn't reading once already fill the buffer (provide edge cases)?
      int nn = read(0, data + n, sizeof(data) - n); // Read FD 0 (stdin)
      if (nn <= 0)
        break;
      n += nn;
    }
    memdump(argv[1], data, n);
  } else {
    printf("Usage: memdump [format]\n");
    exit(1);
  }
  exit(0);
}


// Cursor-type rationale: docs/labs/util-memdump.md#why-char-data
void
memdump(char *fmt, char *data, int len)
{
  // Your code here.  `data` holds `len` valid bytes.
  int i = 0;
  char *data_ptr = data;

  while (fmt[i] != '\0') {
    char format = fmt[i];
    switch (format) {
      case 'i':
        if (strlen(data_ptr) < sizeof(uint32)) {
          printf("Error: Not enough data for 'i' format\n");
          exit(1);
        }
        // Cast to int pointer so that pointer arithmetic works correctly, then dereference to get the int value
        printf("%d\n", *((uint32 *) data_ptr));
        data_ptr += sizeof(uint32); // Move the pointer forward by the size of an int
        break;
      case 'h':
        if (strlen(data_ptr) < sizeof(uint16)) {
          printf("Error: Not enough data for 'h' format\n");
          exit(1);
        }
        printf("%d\n", *((uint16 *) data_ptr));  // Q: Is using nested parenthesis a best practice?
        data_ptr += sizeof(uint16);
        break;
      case 'S':
        printf("%s\n", data_ptr);
        break;
      case 's':
        if (strlen(data_ptr) < sizeof(char *)) {
          printf("Error: Not enough data for 's' format\n");
          exit(1);
        }
        printf("%s\n", *((char **) data_ptr)); // Q: Diff between S & s (they look the same to me) // Q: I don't understand why we need to cast w/ char** (draw .excalidraw)?
        data_ptr += sizeof(char *); // Move the pointer forward by the size of a char pointer
        break;
      case 'c':
        if (strlen(data_ptr) < sizeof(char)) {
          printf("Error: Not enough data for 'c' format\n");
          exit(1);
        }
        printf("%c\n", *data_ptr); // Q: No need to cast data since it's already a char pointer
        data_ptr += sizeof(char);
        break;
      case 'p':
        if (strlen(data_ptr) < sizeof(uint64)) {
          printf("Error: Not enough data for 'p' format\n");
          exit(1);
        }
        printf("%lx\n", *((uint64 *) data_ptr));
        data_ptr += sizeof(uint64); // Or: data_ptr += sizeof(void *); // Q: Which is better to use here?
        break;
    }

    i++;
  }

}
