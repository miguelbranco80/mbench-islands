/*
 * FIXME: Add optional & inexpensive check to see if threads are properly assigned to cores. Use bitmap init'ed to 0 and set to 1 when misaligned, to know if >1 misalignment occurred.
 */
#define _GNU_SOURCE

#include <pthread.h>
#include <numaif.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sched.h>
#include <time.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#if defined(USE_CUSTOM_SPINLOCK)
#include "spinlock.h"
#elif defined(USE_K42)
#include "k42.h"
#elif defined(USE_TICKETLOCK)
#include "ticketlock.h"
#endif

#define NCORES 16
#define NSOCKETS 4
#define NRUNS 10

#if defined(USE_SYNC_FETCH_ADD)
#define NLOOPS 100000000
#else
#define NLOOPS 10000000
#endif

struct counter_aligned { /* single element per cache line */
	unsigned long c;
	char fill[64-sizeof(unsigned long)];
} __attribute((aligned(64)));

struct counter_aligned *counters[NCORES];

#if defined(USE_PTHREAD_SPINLOCK)
typedef pthread_spinlock_t LOCK;
#elif defined(USE_PTHREAD_MUTEX)
typedef pthread_mutex_t LOCK;
#elif defined(USE_TATAS) || defined(USE_TAS)
typedef volatile int LOCK;
#elif defined(USE_CUSTOM_SPINLOCK)
typedef spinlock LOCK;
#elif defined(USE_K42)
typedef k42lock LOCK;
#elif defined(USE_TICKETLOCK)
typedef ticketlock LOCK;
#endif

#if !defined(USE_SYNC_FETCH_ADD)
LOCK *locks[NCORES];
#endif

int mapping[NCORES];

void *worker(void *arg)
{
	unsigned long i;
	unsigned long *c;
#if !defined(USE_SYNC_FETCH_ADD)
	LOCK *lock;
#endif
	long tid = (long) arg;
	
	if (mapping[tid] < 0)
		return NULL; /* thread inactive */

	c = &counters[mapping[tid]]->c;
#if !defined(USE_SYNC_FETCH_ADD)
	lock = locks[mapping[tid]];
#endif

	for (i=0; i<NLOOPS; i++) {
#if defined(USE_PTHREAD_SPINLOCK)
		pthread_spin_lock(lock);
		*c += 1;
		pthread_spin_unlock(lock);
#elif defined(USE_PTHREAD_MUTEX)
		pthread_mutex_lock(lock);
		*c += 1;
		pthread_mutex_unlock(lock);
#elif defined(USE_TATAS) || defined(USE_TAS)
	#if defined(USE_TATAS)
		while (*lock)
			;
	#endif
		while (__sync_lock_test_and_set(lock, 1))
			;
		*c += 1;
		__sync_lock_release(lock);
#elif defined(USE_CUSTOM_SPINLOCK)
		spin_lock(lock);
		*c += 1;
		spin_unlock(lock);
#elif defined(USE_K42)
		k42_lock(lock);
		*c += 1;
		k42_unlock(lock);
#elif defined(USE_TICKETLOCK)
		ticket_lock(lock);
		*c += 1;
		ticket_unlock(lock);
#elif defined(USE_SYNC_FETCH_ADD)
		__sync_add_and_fetch(c, 1);
#endif
	}

	return NULL;
}

double diff(struct timespec st, struct timespec end)
{
	struct timespec tmp;

	if ((end.tv_nsec-st.tv_nsec)<0) {
		tmp.tv_sec = end.tv_sec - st.tv_sec - 1;
		tmp.tv_nsec = 1e9 + end.tv_nsec - st.tv_nsec;
	} else {
		tmp.tv_sec = end.tv_sec - st.tv_sec;
		tmp.tv_nsec = end.tv_nsec - st.tv_nsec;
	}

	return tmp.tv_sec + tmp.tv_nsec*1e-9;
}

float run()
{
	pthread_t threads[NCORES];
	pthread_attr_t a;
	struct timespec st, end;
#if defined(USE_AFFINITY)
	cpu_set_t c;
#endif
	long i, j;

	pthread_attr_init(&a);
	
	for (i=0; i<NCORES; i++)
		counters[i]->c = 0;

	clock_gettime(CLOCK_REALTIME, &st);

	for (i=1; i<NCORES; i++) {
#if defined(USE_AFFINITY)
		CPU_ZERO(&c);
		CPU_SET(i, &c);
		pthread_attr_setaffinity_np(&a, sizeof(c), &c);
#endif
		pthread_create(&threads[i], &a, worker, (void *) i);
	}

#if defined(USE_AFFINITY)
	CPU_ZERO(&c);
	CPU_SET(0, &c);
	pthread_setaffinity_np(pthread_self(), sizeof(c), &c);
#endif
	worker((void *) 0);

	for (i=1; i<NCORES; i++) {
		pthread_join(threads[i], NULL);
	}

	clock_gettime(CLOCK_REALTIME, &end);

	/* check if each counter has final expected value */
	for (i=0; i<NCORES; i++) {
		int c = 0;
		for (j=0; j<NCORES; j++) {
			if (mapping[j] == i)
				c += 1;
		}
		assert(counters[i]->c == c*NLOOPS);
	}

	return diff(st, end);
}


int main(int argc, char *argv[])
{
	long i, j, k;
	char logf[100];

	sprintf(logf, "log-%s", (rindex(argv[0], '/') != NULL) ? rindex(argv[0], '/')+1 : argv[0]);

	FILE *f = fopen((argc<2) ? logf : argv[1], "a");

	/* */

#if defined(USE_AFFINITY)
	fprintf(f, "Using affinity.\n");
#endif

	/* allocate memory for counters and set their NUMA policy  */

	for (i=0; i<NCORES; i++) {
		unsigned long nodemask = 0; /* FIXME: Support more cores than fit in unsigned long? */

		counters[i] = valloc(sizeof(struct counter_aligned));
		if (!counters[i]) {
			printf("valloc error: %s\n", strerror(errno));
			exit(2);
		}

		nodemask = 1 << (i/NSOCKETS);
		if (mbind(counters[i], sizeof(struct counter_aligned), MPOL_BIND,
			  &nodemask, sizeof(nodemask), MPOL_MF_MOVE | MPOL_MF_STRICT) != 0) {
			printf("mbind error: %s\n", strerror(errno));
			exit(2);
		}
	}
	
	/* allocate memory for locks and set their NUMA policy */
#if !defined(USE_SYNC_FETCH_ADD)
	for (i=0; i<NCORES; i++) {
		unsigned long nodemask = 0; /* FIXME: Support more cores than fit in unsigned long? */

		locks[i] = valloc(sizeof(LOCK));
		if (!locks[i]) {
			printf("valloc error: %s\n", strerror(errno));
			exit(2);
		}

		nodemask = 1 << (i/NSOCKETS);
		if (mbind(locks[i], sizeof(LOCK), MPOL_BIND,
	  		  &nodemask, sizeof(nodemask), MPOL_MF_MOVE | MPOL_MF_STRICT) != 0) {
			printf("mbind error: %s\n", strerror(errno));
			exit(2);
		}
	}
#endif

	/* init locks */

#if defined(USE_PTHREAD_SPINLOCK)
	for (i=0; i<NCORES; i++) {
		pthread_spin_init(locks[i], 0);
	}
	fprintf(f, "Using pthread spinlock.\n");
#elif defined(USE_PTHREAD_MUTEX)
	for (i=0; i<NCORES; i++) {
		pthread_mutex_init(locks[i], NULL);
	}
	fprintf(f, "Using pthread mutex.\n");
#elif defined(USE_TATAS) || defined(USE_TAS)
	for (i=0; i<NCORES; i++) {
		*locks[i] = 0;
	}
	#if defined(USE_TATAS)
	fprintf(f, "Using TATAS w/ GCC __sync_lock_test_and_set built-in.\n");
	#else
	fprintf(f, "Using TAS w/ GCC __sync_lock_test_and_set built-in.\n");
	#endif
#elif defined(USE_CUSTOM_SPINLOCK)
	fprintf(f, "Using custom spinlock.\n");
#elif defined(USE_K42)
	fprintf(f, "Using k42.\n");
#elif defined(USE_TICKETLOCK)
	fprintf(f, "Using ticketlock.\n");
#elif defined(USE_SYNC_FETCH_ADD)
	fprintf(f, "Using GCC __sync_add_and_fetch built-in.\n");
#endif

	/* run loops */

	fprintf(f, "Looping %d times per thread\n", NLOOPS);
	for (i=0; i<NCORES; i++) {
		for (j=i+1; j<NCORES; j++) {
	
			for (k=0; k<NCORES; k++)
				mapping[k] = ((k == i) | (k == j)) ? i : -1;
			
			fprintf(f, "Mapping");
			for (k=0; k<NCORES; k++)
				fprintf(f, " %d", mapping[k]);
			fprintf(f, "\n");
			fflush(f);

			for (k=0; k<NRUNS; k++) {
				fprintf(f, "Took %lf\n", run());
				fflush(f);
			}
		}
	}

	/* de'init locks */
	
	for (i=0; i<NCORES; i++) {
#if defined(USE_PTHREAD_SPINLOCK)
		pthread_spin_destroy(locks[i]);
#elif defined(USE_PTHREAD_MUTEX)
		pthread_mutex_destroy(locks[i]);
#endif
	}

	pthread_exit(NULL);
}
