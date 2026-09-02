#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"


void memdump(char *fmt, char *data);


int
main(int argc, char *argv[])
{
  if(argc == 1){
    // Layout: docs/labs/util-memdump.md#the-five-starter-layouts
    printf("Example 1:\n");
    int a[2] = { 61810, 2025 };
    // Byte-view rationale: docs/labs/util-memdump.md#why-char-data
    memdump("ii", (char*) a);

    printf("Example 2:\n");
    memdump("S", "a string");

    printf("Example 3:\n");
    char *s = "another";
    // Pointer-slot rationale: docs/labs/util-memdump.md#why-s-needs-s
    memdump("s", (char *) &s);

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
    memdump("pihcS", (char*) &example);

    printf("Example 5:\n");
    memdump("sccccc", (char*) &example);
  } else if (argc == 2){
    // format in argv[1], up to 512 bytes of data from standard input.
    // Capacity rationale: docs/labs/util-memdump.md#why-512-bytes
    char data[512];
    int n = 0;
    memset(data, '\0', sizeof(data));
    while(n < sizeof(data)){
      int nn = read(0, data + n, sizeof(data) - n);
      if(nn <= 0)
        break;
      n += nn;
    }
    memdump(argv[1], data);
  } else {
    printf("Usage: memdump [FORMAT]\n");
    return 1;
  }
  return 0;
}

// Cursor-type rationale: docs/labs/util-memdump.md#why-char-data
void
memdump(char *fmt, char *data)
{
  // Your code here.

}
