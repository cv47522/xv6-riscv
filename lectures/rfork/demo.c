/*
 * demo.c - remote fork demo.
 *
 * The parent runs here.  rfork() starts the child on the machine where
 * rforkd is running; the child's memory is fetched from the parent page by
 * page, as the child touches it.
 *
 *   $ ./rforkd &
 *   $ ./demo
 */
#include <stdio.h>
#include <stdlib.h>

#include "rfork.h"

int main(int argc, char **argv)
{
    (void)argc;
    rfork_init(argv);               /* run with ASLR off: parent and child must agree on addresses */

    char *msg = "hello from parent";
    
    int r = rfork();
    if (r < 0) {
        perror("rfork");
        return 1;
    }
    if (r == 0) {
	/* the child: same code, same stack, same libc, but on another machine */
	printf("%s\n", msg);
        exit(0);
    }

    printf("parent: rforked child\n");
    rfork_wait(r);                  /* optional: wait for the remote child to finish */
    return 0;
}
