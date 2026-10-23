#include <stdint.h>

struct rwspinlock {
	uint32_t n;
};

void rwlock_r_acquire(struct rwspinlock *rwlk);
void rwlock_r_release(struct rwspinlock *rwlk);


