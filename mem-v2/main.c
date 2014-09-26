/*
 * See README.
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

#define KB		1024
#define MB		(1024 * KB)
#define GB		(1024 * MB)
#define MAXTHREADS	256

#define PRNG_BUFSZ	256

typedef struct random_s {
        struct random_data	rand_state;
        char			rand_statebuf[PRNG_BUFSZ];
} random_t;

int counters[MAXTHREADS];
int nthreads;

long *blocks[MAXTHREADS];
int blocksize;

int batchsize;

pthread_mutex_t *locks[MAXTHREADS];

random_t *randgen[MAXTHREADS];


struct worker_args {
	int		id;
	int		block;
	unsigned int	seed;
};


void
*measure_thread(void *arg)
{
	int c, i;
	int throughput;
        for (c = 1 ;; c++) {
                sleep(2);
                throughput = 0;
                for (i = 0; i < nthreads; i++) {
                        throughput += __sync_lock_test_and_set(&counters[i], 0);
		}
                //printf("%d\t%d\n", c, throughput);
		printf("%d\n", throughput);
                fflush(stdout);
        }
}


void
*worker_thread(void *arg)
{
	struct worker_args *args = arg;
	long *block = blocks[args->block];
	pthread_mutex_t *lock = locks[args->block];

	// initialize per-thread random number generator

	random_t r;
        if (initstate_r(args->seed, (char*) &r.rand_statebuf, PRNG_BUFSZ, &r.rand_state) != 0) { 
                perror("initstate_r");
                exit(EXIT_FAILURE);
        }

	int c;
	int i, j;
	long swap;
	int seq_i[batchsize];
	int seq_j[batchsize];

	for (;;) {
		for (c = 0; c < batchsize; c++) {
			if (random_r(&r.rand_state, &i) != 0) {
				perror("random_r");
				exit(EXIT_FAILURE);
			}
			seq_i[c] = (((double) i) / RAND_MAX) * blocksize;

			if (random_r(&r.rand_state, &j) != 0) {
				perror("random_r");
				exit(EXIT_FAILURE);
			}
			seq_j[c] = (((double) j) / RAND_MAX) * blocksize;
		}

		pthread_mutex_lock(lock);

		for (c = 0; c < batchsize; c++) {
			swap = block[seq_i[c]];
			block[seq_i[c]] = block[seq_j[c]];
			block[seq_j[c]] = swap;
		}

		pthread_mutex_unlock(lock);

		__sync_fetch_and_add(&counters[args->id], 1);
		//printf("[Thread %d] Using CPU %d\n", args->id, sched_getcpu());
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

	read_config_file(argv[1]);

	// Initialize blocks

	int nblocks;
	get_int_config_value("nblocks", &nblocks);
	nthreads = nblocks;
	get_int_config_value("blocksize", &blocksize);
	get_int_config_value("batchsize", &batchsize);

	for (i = 0; i < nblocks; i++) {

		// Allocate block in correct node

		blocks[i] = valloc(blocksize * sizeof(long));
		if (!blocks[i]) {
			perror("valloc");
			exit(EXIT_FAILURE);
		}
#if defined(AFFINITY)
		char k[80];
		sprintf(k, "block%d_node", i);

		int node;
		get_int_config_value(k, &node);

		unsigned long nodemask = 1 << node;

		if (mbind(blocks[i], blocksize * sizeof(long),
			  MPOL_BIND, &nodemask, sizeof(nodemask),
			  MPOL_MF_MOVE | MPOL_MF_STRICT) != 0) {
			perror("mbind");
			exit(EXIT_FAILURE);
		}
#endif

		// Set block initial values

		int j;
		for (j = 0; j < blocksize; j++) {
			blocks[i][j] = j;
		}

		// Allocate lock in correct node

		locks[i] = valloc(sizeof(pthread_mutex_t));
		if (!locks[i]) {
			perror("valloc");
			exit(EXIT_FAILURE);
		}
#if defined(AFFINITY)
		if (mbind(locks[i], sizeof(pthread_mutex_t),
			  MPOL_BIND, &nodemask, sizeof(nodemask),
			  MPOL_MF_MOVE | MPOL_MF_STRICT) != 0) {
			perror("mbind");
			exit(EXIT_FAILURE);
		}
#endif
		// Initialize lock

		pthread_mutex_init(locks[i], NULL);
		//printf("Initialized lock %d (address %p) in NUMA node %ld\n", i, locks[i], nodemask);
	}

	//long blockbytes = (blocksize * sizeof(long) / MB);
	//printf("Total memory allocated: %ld MB (block size %ld MB)\n", nblocks * blockbytes, blockbytes);

	// Launch measure thread

	pthread_t th;
	pthread_create(&th, NULL, measure_thread, NULL);

	// Apply (crappy version of) Fisher-Yates to sort list of threads to launch

	int shuffle[nthreads];
	for (i = 0; i < nthreads; i++) {
		shuffle[i] = 0;
	}
	int sorted[nthreads];
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
		sorted[j++] = i;
	}

	// Launch worker threads

	struct worker_args args[nthreads];
	pthread_t threads[nthreads];
#if defined(AFFINITY)
	pthread_attr_t a;
	pthread_attr_init(&a);
#endif
	for (i = 0; i < nthreads; i++) {
		int id = sorted[i];

		char k[80];
		int block;
		sprintf(k, "thread%d_block", id);
		get_int_config_value(k, &block);

#if defined(AFFINITY)
		int cpu;
		sprintf(k, "thread%d_core", id);
		get_int_config_value(k, &cpu);

		cpu_set_t c;
		CPU_ZERO(&c);
		CPU_SET(cpu, &c);
#endif

		args[id].id = id;
		args[id].block = block;
		args[id].seed = random();

#if defined(AFFINITY)
		pthread_create(&threads[id], &a, worker_thread, &args[id]);
#else
		pthread_create(&threads[id], NULL, worker_thread, &args[id]);
#endif
	}

	for (i = 0; i < nthreads; i++) {
		pthread_join(threads[i], NULL);
	}

	return 0;
}

