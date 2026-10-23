#include "rwlock.h"

void
initrwlock(struct rwspinlock *rwlk)
{
	rwlk->n = 0;
}

void rwlock_r_acquire(struct rwspinlock *rwlk) {
	while (1) {
		uint32_t v = __atomic_load_n(&rwlk->n, __ATOMIC_ACQUIRE);
		if (v & 0x80000000) {
			// Pending or active writer; retry.
			continue;
		}
		if (__atomic_compare_exchange_n(&rwlk->n, &v, v+1, 0, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED)) {
			break;
		}
	}
}

void rwlock_r_release(struct rwspinlock *rwlk) {
	__atomic_sub_fetch(&rwlk->n, 1, __ATOMIC_RELEASE);
}


void rwlock_w_acquire(struct rwspinlock *rwlk) {
	uint32_t v;

	while (1) {
		v = __atomic_fetch_or(&rwlk->n, 0x80000000, __ATOMIC_ACQUIRE);
		if (v & 0x80000000) {
			// Concurrent writer; retry.
		} else {
			break;
		}
	}

	while (v & 0x7fffffff) {
		// Pending readers; retry;
		v = __atomic_load_n(&rwlk->n, __ATOMIC_ACQUIRE);
	}	
}

void rwlock_w_release(struct rwspinlock *rwlk) {
	__atomic_store_n(&rwlk->n, 0, __ATOMIC_RELEASE);

}
