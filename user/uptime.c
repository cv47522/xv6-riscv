/*
 * uptime.c: print the clock ticks elapsed since boot.
 *
 * docs/labs/06-util-uptime.md derives it from sys_uptime() in
 * kernel/sysproc.c; grade-lab-util's uptime tests check the format below.
 */

#include "kernel/types.h" // defines uint, which user/user.h uses
#include "user/user.h"

int
main(int argc, char *argv[])
{
  /*
   * sys_uptime() reads no argument, so any word here would be ignored.
   * argv[0] also keeps -Wextra's unused-parameter check satisfied.
   */
  if (argc != 1) {
    fprintf(2, "Usage: %s\n", argv[0]);
    exit(1);
  }

  /*
   * Ticks, not seconds: clockintr() in kernel/trap.c picks the interval
   * (about 0.1 s today), so a conversion would hard-code a timer constant.
   * The kernel's uint arrives as int; %d goes negative only past 2^31 ticks.
   */
  printf("Uptime: %d ticks\n", uptime());
  exit(0);
}
