/*
 * rfork.h - remote fork.
 *
 *   rfork_init(argv)  Re-executes the program with address space randomisation
 *                     turned off.  Parent and child must agree on where things
 *                     like the vdso live, so call this first thing in main().
 *
 *   rfork()           Like fork(), but the child runs on another machine (the
 *                     one where rforkd is listening, see RFORK_HOST/RFORK_PORT).
 *                     Returns 0 in the child, a positive handle in the parent,
 *                     -1 on error.  The child's address space is empty when it
 *                     starts; every page it touches is fetched from the parent
 *                     on demand through userfaultfd.
 *
 *   rfork_wait(h)     Waits until the remote child has exited.
 */
#ifndef RFORK_H
#define RFORK_H

void rfork_init(char **argv);
int  rfork(void);
int  rfork_wait(int handle);

#endif
