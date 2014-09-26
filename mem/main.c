/*
 * Usage:
 *  ./main [config file]
 *
 * A sample configuration file is:

blocksize = 1000000
nblocks = 2
block0_node = 0
block1_node = 0
nthreads = 2
thread0_core = 0
thread1_core = 2
thread0_block = 0
thread1_block = 1

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


int counters[MAXTHREADS];
int nthreads;

long *blocks[MAXTHREADS];
int blocksize;


struct worker_args {
	int block;
	int counter;
};


void
*measure_thread(void *arg)
{
	int c, i;
	int throughput;
        for (c = 1 ;; c++) {
                sleep(1);
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
	
	//printf("Thread going for block at %d and counter at %d\n", args->block, args->counter);
	for (;;) {
		int n = blocksize / 2;
		int i, j;
		for (i = 0, j = blocksize; i < n; i++, j--) {
			long swap;
			swap = block[i];
			block[i] = block[j];
			block[j] = swap;
		}
		__sync_fetch_and_add(&counters[args->counter], 1);
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
	get_int_config_value("blocksize", &blocksize);

	for (i = 0; i < nblocks; i++) {
		char k[80];
		sprintf(k, "block%d_node", i);

		int node;
		get_int_config_value(k, &node);

		unsigned long nodemask = 1 << node;

		// Allocate block in correct node

		blocks[i] = valloc(blocksize * sizeof(long));
		if (!blocks[i]) {
			perror("valloc");
			exit(EXIT_FAILURE);
		}
		if (mbind(blocks[i], blocksize * sizeof(long),
			  MPOL_BIND, &nodemask, sizeof(nodemask),
			  MPOL_MF_MOVE | MPOL_MF_STRICT) != 0) {
			perror("mbind");
			exit(EXIT_FAILURE);
		}

		// Set initial values

		int j;
		for (j = 0; j < blocksize; j++) {
			blocks[i][j] = j;
		}
	}

	//long blockbytes = (blocksize * sizeof(long) / MB);
	//printf("Total memory allocated: %ld MB (block size %ld MB)\n", nblocks * blockbytes, blockbytes);

	// Launch measure thread

	pthread_t th;
	pthread_create(&th, NULL, measure_thread, NULL);

	// Launch worker threads

	get_int_config_value("nthreads", &nthreads);

	struct worker_args args[nthreads];
	pthread_t threads[nthreads];
	pthread_attr_t a;
	pthread_attr_init(&a);
	for (i = 0; i < nthreads; i++) {
		char k[80];
		int block;
		sprintf(k, "thread%d_block", i);
		get_int_config_value(k, &block);
		if (block < 0) {
			threads[i] = 0;
			continue;
		}

		int cpu;
		sprintf(k, "thread%d_core", i);
		get_int_config_value(k, &cpu);

		cpu_set_t c;
		CPU_ZERO(&c);
		CPU_SET(cpu, &c);
		pthread_attr_setaffinity_np(&a, sizeof(c), &c);

		args[i].block = block;
		args[i].counter = i;

		pthread_create(&threads[i], &a, worker_thread, &args[i]);
	}

	for (i = 0; i < nthreads; i++) {
		if (threads[i]) {
			pthread_join(threads[i], NULL);
		}
	}

	return 0;
}

