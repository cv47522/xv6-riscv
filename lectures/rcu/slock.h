#include <stdint.h>

struct spinlock {
	uint32_t locked;       // Is the lock held?
};

void lock_init(struct spinlock *lk);
void lock_acquire(struct spinlock *lk);
void lock_release(struct spinlock *lk);
