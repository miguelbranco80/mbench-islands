/*
 * Usage:
 *  ./main [config file]
 *
 * FIXME:
 * - Add optional & inexpensive check to see if threads are properly assigned to
 *   cores.
 *
 * Sample configuration file:

	ncounters = 2
	counter0_node = 0
	counter1_node = 1
	nthreads = 2
	thread0_core = 0
	thread1_core = 2
	thread0_counter = 0
	thread1_counter = 1

*/
#define _GNU_SOURCE

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <numaif.h>

#include "config.h"

#define MAXTHREADS	256
#define MAXCOUNTERS	256

#define CACHELINESIZE	64


/*
 * Per-thread worker structure. Contains counter information plus pointers to
 * locks. Care is taken to align structure to cache line size.
 */
struct counter_aligned {
	unsigned long long c;

#if defined(SYNC_FETCH_ADD)
	char padding[ CACHELINESIZE - \
			sizeof(unsigned long long) ];

#elif defined(PTHREAD_SPIN)
	pthread_spinlock_t spin;
	char padding[ CACHELINESIZE - \
			sizeof(unsigned long long) - \
			sizeof(pthread_spinlock_t) ];

#elif defined(PTHREAD_MUTEX)
	pthread_mutex_t mutex;
	char padding[ CACHELINESIZE - \
			sizeof(unsigned long long) - \
			sizeof(pthread_mutex_t) ];

#endif
} __attribute( ( aligned(CACHELINESIZE) ) );


/*
 * List of per-thread worker structures.
 */
struct counter_aligned *counters[MAXCOUNTERS];
int ncounters;


/* 
 * Throughput measurement thread. Sleeps for a while then wakes up and measures
 * the throughput, resetting counters in the process.
 */
void
*measure_thread(void *arg)
{
	int c, i;
	unsigned long long throughput;
        for (c = 1 ;; c++) {
                sleep(1);
                throughput = 0;
                for (i = 0; i < ncounters; i++) {
                        if (counters[i]->c) {

#if defined(SYNC_FETCH_ADD)
                                throughput += \
					__sync_lock_test_and_set(&counters[i]->c, 0);

#elif defined(PTHREAD_SPIN)
				pthread_spin_lock(&counters[i]->spin);
				throughput += counters[i]->c;
				counters[i]->c = 0;
				pthread_spin_unlock(&counters[i]->spin);

#elif defined(PTHREAD_MUTEX)
				pthread_mutex_lock(&counters[i]->mutex);
				throughput += counters[i]->c;
				counters[i]->c = 0;
				pthread_mutex_unlock(&counters[i]->mutex);
#endif
			}
		}
		printf("%lld\n", throughput);
                fflush(stdout);
        }
}


/*
 * Worker thread. Increments counters in a tight loop.
 */
void
*worker_thread(void *arg)
{
	struct counter_aligned *counter = arg;

	unsigned long long *pcounter = &counter->c;

#if defined(PTHREAD_SPIN)
	pthread_spinlock_t *pspin = &counter->spin;

#elif defined(PTHREAD_MUTEX)
	pthread_mutex_t *pmutex = &counter->mutex;
#endif

	for (;;) {
#if defined(SYNC_FETCH_ADD)
		__sync_fetch_and_add(pcounter, 1);

#elif defined(PTHREAD_SPIN)
		pthread_spin_lock(pspin);
		++(*pcounter);
		pthread_spin_unlock(pspin);

#elif defined(PTHREAD_MUTEX)
		pthread_mutex_lock(pmutex);
		++(*pcounter);
		pthread_mutex_unlock(pmutex);
#endif
	}

	return NULL;
}


int
main(int argc, char *argv[])
{
	int i;

	if (argc < 2) {
		printf("Invalid number of arguments!\n");
		printf("Usage: %s [config file]\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	if (read_config_file(argv[1]) != 0) {
		printf("Error reading configuration file.\n");
		exit(EXIT_FAILURE);
	}

	/*
	 * Launch throughput measurement thread.
	 */
	pthread_t th;
	pthread_create(&th, NULL, measure_thread, NULL);

	/*
	 * Initialize counters.
	 */
	if (get_int_config_value("ncounters", &ncounters) != 0) {
		printf("Error reading 'ncounters'.\n");
		exit(EXIT_FAILURE);
	}

	if (ncounters > MAXCOUNTERS) {
		printf("MAXCOUNTERS exceeded!\n");
		exit(EXIT_FAILURE);
	}

	for (i = 0; i < ncounters; i++) {
		counters[i] = valloc(sizeof(struct counter_aligned));
		if (!counters[i]) {
			perror("valloc");
			exit(EXIT_FAILURE);
		}
#if defined(AFFINITY)
		/*
		 * Allocate counter in the desired memory node.
		 */
		char k[80];
		int node;

		sprintf(k, "counter%d_node", i);
		if (get_int_config_value(k, &node) != 0) {
			printf("Error reading '%s'.\n", k);
			exit(EXIT_FAILURE);
		}

		unsigned long nodemask = 1 << node;
		if (mbind(counters[i], sizeof(struct counter_aligned),
			  MPOL_BIND, &nodemask, sizeof(nodemask) + 1,
			  MPOL_MF_MOVE | MPOL_MF_STRICT) != 0) {
			perror("mbind");
			exit(EXIT_FAILURE);
		}
#endif

		counters[i]->c = 0;
#if defined(PTHREAD_SPIN)
		pthread_spin_init(&counters[i]->spin, 0);
#elif defined(PTHREAD_MUTEX)
		pthread_mutex_init(&counters[i]->mutex, NULL);
#endif
	}

	/*
	 * Launch worker threads.
	 */
	int nthreads;
	if (get_int_config_value("nthreads", &nthreads) != 0) {
		printf("Error reading 'nthreads'.\n");
		exit(EXIT_FAILURE);
	}
	if (nthreads > MAXTHREADS) {
		printf("MAXTHREADS exceeded!\n");
		exit(EXIT_FAILURE);
	}

	pthread_t threads[nthreads];
	pthread_attr_t a;
	pthread_attr_init(&a);

	/*
	 * Apply (crappy version of) Fisher-Yates to create a randomized list
	 * with the threads to launch. This prevents a deterministic behavior
	 * when affinity is OFF: the OS will be launching threads in different
	 * orders across runs, making the execution vary more.
	 */
	int shuffle[nthreads];
	for (i = 0; i < nthreads; i++)
		shuffle[i] = 0;

	int random[nthreads];	/* holds randomized output list */
	int j = 0;
	while (j < nthreads) {
		int n = rand() % nthreads;
		i = 0;
		while (1) {
			if (shuffle[i] == 0) {
				--n;
				if (n < 0) break;
			}
			i = (i + 1) % nthreads;
		}
		shuffle[i] = 1;
		random[j++] = i;
	}

	/*
	 * Launch worker threads.
	 */
	for (i = 0; i < nthreads; i++) {
		char k[80];
		int counter;
		sprintf(k, "thread%d_counter", random[i]);

		if (get_int_config_value(k, &counter) != 0) {
			printf("Error reading '%s'.\n", k);
			exit(EXIT_FAILURE);
		}

#if defined(AFFINITY)
		/*
		 * Allocate worker thread in the desired CPU core.
		 */
		int cpu;
		sprintf(k, "thread%d_core", random[i]);
		if (get_int_config_value(k, &cpu) != 0) {
			printf("Error reading '%s'.\n", k);
			exit(EXIT_FAILURE);
		}

		cpu_set_t c;
		CPU_ZERO(&c);
		CPU_SET(cpu, &c);
		pthread_attr_setaffinity_np(&a, sizeof(c), &c);
#endif

		pthread_create(&threads[random[i]], &a, worker_thread,
				counters[counter]);
	}

	/*
	 * Wait for threads to finish.
	 */
	for (i = 0; i < nthreads; i++)
		pthread_join(threads[i], NULL);

	return 0;
}

