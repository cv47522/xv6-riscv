#include <stddef.h>
#include <stdatomic.h>

#include "slock.h"

void
lock_init(struct spinlock *lk)
{
	atomic_flag_clear(&lk->locked);
}

void
lock_acquire(struct spinlock *lk)
{
	while (atomic_flag_test_and_set_explicit(&lk->locked, __ATOMIC_ACQUIRE))
		;
}

void
lock_release(struct spinlock *lk)
{
	atomic_flag_clear_explicit(&lk->locked, __ATOMIC_RELEASE);
}
