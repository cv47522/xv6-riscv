#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sched.h>

#include <sys/time.h>

#include "slock.h"
#include "rwlock.h"
#include "rcu.h"

#define CACHE_LINE_SZ 64

int ncore = 2;
int ltype = 0;

char *label[] = {"spinlock", "rwlock", "rcu"};

void error(char *s) {
	fprintf(stderr, "error: %s\n", s);
	exit(1);
}

double now()
{
	struct timeval tv;
	gettimeofday(&tv, 0);
	return tv.tv_sec + tv.tv_usec / 1000000.0;
}

struct list_elem {
	int n;
	int refcnt;
	struct list_elem *next;
};

struct list {
	struct list_elem *head;
};

void list_insert(struct list *l, struct list_elem *e) {
	e->next = l->head;
	l->head = e;
}

void list_print(struct list *l) {
	printf("l: ");
	for (struct list_elem *e = l->head; e != NULL; e = e->next) {
		printf("%d ", e->n);
	}
	printf("\n");
}

struct list_elem*  __attribute__ ((noinline)) list_search(struct list *l, int n) {
	for (struct list_elem *e = l->head; e != NULL; e = rcu_dereference(e->next)) {
		if (e->n == n && e->refcnt == 0)
			return e;
	}
}

struct list *new_list(int n) {
	struct list *l = (struct list *) malloc(sizeof(struct list));
	l->head = 0;
	for (int i = 0; i < n; i++) {
		struct list_elem *e = malloc(sizeof(struct list_elem));
		e->n = i;
		e->refcnt = 0;
		e->next = NULL;
		list_insert(l, e);
	}
	return l;
}

struct core {
	int cid;
	double s;
};


#define TIME_TO_RUN 2 // sec
#define NOPS 10000000

struct list *thel __attribute__((aligned(CACHE_LINE_SZ)));
struct spinlock slk __attribute__((aligned(CACHE_LINE_SZ)));
struct rwspinlock rwlk __attribute__((aligned(CACHE_LINE_SZ)));

void *run_core(void* core) {
	struct core *mycore = (struct core *) core;

	// pin to an actual core
	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(mycore->cid, &cpuset);
	if (pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) < 0)
		error("couldn't set affininity\n");

	int cont = 1;
	double start = now();
	for (int i = 0; i < NOPS; i++) {
		if (ltype == 0) lock_acquire(&slk);
		if (ltype == 1) rwlock_r_acquire(&rwlk);
		if (ltype == 2) rcu_read_lock();
		struct list_elem *e = list_search(thel, 0);
		// assert(e->n == 0);
		if (ltype == 0) lock_release(&slk);
		if (ltype == 1) rwlock_r_release(&rwlk);
		if (ltype == 2) rcu_read_unlock();
	}
	printf("%d list_search took %0.2fs\n", NOPS, now()-start);
}

#define N 10

void main(int argc, char *argv[]) {
	if(argc != 3) {
		error("<ncore> <lock type:0,1,2>");
	}
	ncore = atoi(argv[1]);
	ltype = atoi(argv[2]);
	printf("ncore %d ltype %s\n", ncore, label[ltype]);
	pthread_t *threads = (pthread_t *) malloc(ncore * sizeof(pthread_t));
	struct core *cores = (struct core *) malloc(ncore * sizeof(struct core));
	thel = new_list(N);
	lock_init(&slk);
	for (int i = 0; i < ncore; i++) {
		cores[i].cid = i;
		pthread_create(&threads[i], NULL, run_core, (void*) &cores[i]);
	}
	for (int i = 0; i < ncore; i++) {
		pthread_join(threads[i], NULL);
	}
}
