#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "user/user.h"

enum token_state {
  TOKEN_BOUNDARY,
  TOKEN_NUMBER,
  TOKEN_INVALID,
};

// ---------------------------------------------------------------------------
//  STREAMING TOKEN STATE
// ---------------------------------------------------------------------------
//
//   State           Meaning                         Boundary or EOF action
//   --------------  ------------------------------  ----------------------
//   TOKEN_BOUNDARY  no token is being scanned       nothing
//   TOKEN_NUMBER    every byte so far is a digit    test the numeric value
//   TOKEN_INVALID   the token contains a nondigit   discard the token
//
// Only bytes in " -\r\t\n./," are boundaries. Other nondigits invalidate the
// entire token, so xv6 does not contribute the number 6. See the full state
// model in docs/labs/util-sixfive.md.

static void
print_if_multiple(int value)
{
  if (value % 5 == 0 || value % 6 == 0)
    printf("%d\n", value);
}

static void
sixfive(int fd, char *name)
{
  // Q: Why not initialize state to TOKEN_BOUNDARY in this declaration?
  // A: That is valid C. The separate assignment matches xv6's usual style of
  // declaring locals first and then initializing the function's state.
  enum token_state state;

  // Q: Why not initialize c to -1?
  // A: A positive read result overwrites c before the loop body uses it. On
  // EOF or error, the body does not run, so a sentinel value would be unused.
  char c;

  // Q: Why not initialize n and value to -1?
  // A: read assigns n before the condition examines it, while decimal
  // accumulation requires value to begin at zero rather than at -1.
  int n, value;

  state = TOKEN_BOUNDARY;
  value = 0;
  // Q: Why not read a buffer as wc.c does?
  // A: A buffer can work if state survives between reads and only n bytes are
  // examined. One-byte reads follow the handout and remove buffer boundaries.
  while ((n = read(fd, &c, 1)) > 0) {
    // Q: Why is no string comparison needed after strchr()?
    // A: strchr() returns a pointer to the matching character or zero, not a
    // string. Nonzero is true in an if condition, and zero is false.
    // Q: Why compare digit bounds instead of using atoi()?
    // A: c is one byte, while atoi() requires a pointer to a NUL-terminated
    // string. The bounds test classifies the byte without building a string.
    if (strchr(" -\r\t\n./,", c)) {
      if (state == TOKEN_NUMBER)
        print_if_multiple(value);
      state = TOKEN_BOUNDARY;
      value = 0;
    } else if ('0' <= c && c <= '9') {
      if (state == TOKEN_BOUNDARY) {
        state = TOKEN_NUMBER;
        value = 0;
      }
      if (state == TOKEN_NUMBER) {
        // Q: Why not use atoi() to extend value?
        // A: Subtracting '0' converts this digit byte to its integer value.
        // Multiplying first preserves the place value of the earlier digits.
        value = value * 10 + c - '0';
      }
    } else {
      state = TOKEN_INVALID;
    }
  }

  if (n < 0) {
    fprintf(2, "sixfive: read error %s\n", name);
    exit(1);
  }
  if (state == TOKEN_NUMBER)
    print_if_multiple(value);
}

int
main(int argc, char *argv[])
{
  // Q: Why not initialize fd and i to -1?
  // A: open() assigns fd before it is tested or used, and the for initializer
  // assigns i before its first comparison. Neither variable needs a sentinel.
  int fd, i;

  if (argc < 2) {
    fprintf(2, "Usage: sixfive FILE1 [FILE2 ...]\n");
    return 1;
  }

  // Q: Why not declare int i in the for initializer?
  // A: C99 permits that. Declaring locals at the function top follows the
  // style used by wc.c and the rest of xv6.
  for (i = 1; i < argc; i++) {
    // open() returns the lowest unused descriptor, normally 3 in this process.
    if ((fd = open(argv[i], O_RDONLY)) < 0) {
      fprintf(2, "sixfive: cannot open %s\n", argv[i]);
      return 1;
    }
    sixfive(fd, argv[i]);
    close(fd);
  }

  return 0;
}
