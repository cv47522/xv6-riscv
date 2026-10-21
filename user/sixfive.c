#include "kernel/types.h" // defines uint, which user/user.h uses
#include "kernel/fcntl.h"
#include "user/user.h"

#define SIXFIVE_INT_MAX 2147483647 // no <limits.h>; int is 32 bits on RV64

enum token_state {
  TOKEN_BOUNDARY,
  TOKEN_NUMBER,
  TOKEN_INVALID,
};

/*
 * ---------------------------------------------------------------------------
 *  STREAMING TOKEN STATE
 * ---------------------------------------------------------------------------
 *
 *   State           Meaning                         Boundary or EOF action
 *   --------------  ------------------------------  ----------------------
 *   TOKEN_BOUNDARY  no token is being scanned       nothing
 *   TOKEN_NUMBER    every byte so far is a digit    test the numeric value
 *   TOKEN_INVALID   a nondigit, or value > int max  discard the token
 *
 * Only bytes in " -\r\t\n./," are boundaries. Other nondigits invalidate the
 * entire token, so xv6 does not contribute the number 6. See the token
 * contract in docs/labs/02-util-sixfive.md.
 *
 * Overflow policy: a token whose value exceeds SIXFIVE_INT_MAX is invalid,
 * like a token containing a letter. Printing it would need a wider type or
 * the original digits, and signed overflow is undefined behaviour. Leading
 * zeros never overflow because they keep value at 0.
 *
 * Failure policy: an open or read failure reports the path on fd 2, the
 * remaining operands are still processed, and the exit status is 1 if any
 * of them failed, as most Unix filters do.
 */

static void
print_if_multiple(int value)
{
  if (value % 5 == 0 || value % 6 == 0)
    printf("%d\n", value);
}

// Returns 0 after EOF, or -1 after a read error.
static int
sixfive(int fd, char *name)
{
  /*
   * Q: Why not initialize state to TOKEN_BOUNDARY in this declaration?
   * A: That is valid C. The separate assignment matches xv6's usual style of
   * declaring locals first and then initializing the function's state.
   */
  enum token_state state;

  /*
   * Q: Why not initialize c to -1?
   * A: A positive read result overwrites c before the loop body uses it. On
   * EOF or error, the body does not run, so a sentinel value would be unused.
   */
  char c;

  /*
   * Q: Why not initialize n and value to -1?
   * A: read assigns n before the condition examines it, while decimal
   * accumulation requires value to begin at zero rather than at -1.
   */
  int n, value;

  state = TOKEN_BOUNDARY;
  value = 0;
  /*
   * Q: Why not read a buffer as wc.c does?
   * A: A buffer can work if state survives between reads and only n bytes are
   * examined. One-byte reads follow the handout and remove buffer boundaries.
   *
   * Q: Why can't read() receive c instead of &c?
   * A: read() needs the address of writable storage where it can copy bytes.
   * c is the byte value. Passing it would misinterpret that value as an
   * address rather than as writable storage.
   */
  while ((n = read(fd, &c, 1)) > 0) {
    /*
     * Q: Why is no string comparison needed after strchr()?
     * A: user/ulib.c returns a pointer into the searched string on a match,
     * or zero otherwise. The condition tests membership, not string equality;
     * no new string is returned. This version does not match the final NUL.
     *
     * Q: Why compare digit bounds instead of using atoi()?
     * A: c is one byte, while atoi() requires a pointer to a NUL-terminated
     * string. The bounds test classifies the byte without building a string.
     */
    if (strchr(" -\r\t\n./,", c)) {
      if (state == TOKEN_NUMBER)
        print_if_multiple(value);
      state = TOKEN_BOUNDARY;
      value = 0;
    } else if ('0' <= c && c <= '9') {
      if (state == TOKEN_BOUNDARY) {
        state = TOKEN_NUMBER;
      }
      // The check runs before the multiply so value never wraps.
      if (state == TOKEN_NUMBER &&
          value > (SIXFIVE_INT_MAX - (c - '0')) / 10)
        state = TOKEN_INVALID;
      if (state == TOKEN_NUMBER) {
        /*
         * Q: Why not use atoi() to extend value?
         * A: atoi() converts a string from the beginning; it cannot extend an
         * existing value with one byte. Subtracting '0' converts this digit,
         * and multiplying by 10 preserves the earlier decimal places.
         */
        value = value * 10 + (c - '0'); // value * 10 + c may overflow
      }
    } else {
      state = TOKEN_INVALID;
    }
  }

  if (n < 0) { // n == -1: Error
    fprintf(2, "sixfive: read error %s\n", name);
    return -1; // a partial token is not committed
  }
  if (state == TOKEN_NUMBER) // n == 0: EOF
    print_if_multiple(value);
  return 0;
}

int
main(int argc, char *argv[])
{
  /*
   * Q: Why not initialize fd and i to -1?
   * A: open() assigns fd before it is tested or used, and the for initializer
   * assigns i before its first comparison. Neither variable needs a sentinel.
   */
  int fd, i, failed;

  /*
   * Q: Why read descriptor 0 instead of reporting a usage error?
   * A: The handout defines no-argument invocation as reading standard input,
   * which the shell has already connected to a pipe or the terminal. cat.c and
   * wc.c take the same branch, and wc() passes an empty name because there is
   * no filename to report in a diagnostic.
   */
  if (argc < 2) {
    exit(sixfive(0, "") < 0);
  }

  /*
   * Q: Why not declare int i in the for initializer?
   * A: C99 permits that and limits i's scope to the loop. The existing form
   * follows wc.c's style; it does not change how this loop executes.
   */
  failed = 0;
  for (i = 1; i < argc; i++) {
    // open() returns the lowest unused descriptor, normally 3 in this process.
    if ((fd = open(argv[i], O_RDONLY)) < 0) {
      fprintf(2, "sixfive: cannot open %s\n", argv[i]);
      failed = 1;
      continue;
    }
    if (sixfive(fd, argv[i]) < 0)
      failed = 1;
    close(fd);
  }

  exit(failed);
}
