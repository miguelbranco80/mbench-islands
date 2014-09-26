#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "queue.h"

char *g_FileName;
size_t g_BlockSize;
int g_NWorkers;
size_t g_NBlocks;

char *g_Buf;

queue_t g_Queue;


void*
consumer(void *arg)
{
    int elem;
    
    while (1) {
        queue_pop(&g_Queue, &elem);
        if (elem == -1) {
            break;
        }
    
        // do something with the data...
        //printf("Got %d\n", elem);
    }

    pthread_exit(NULL);
}

void*
producer(void *arg)
{
    char *buf;
	FILE *f;
	int i;
	size_t siz;
    
    buf = malloc(g_BlockSize);
    
	f = fopen(g_FileName, "r");
	if (0 == f) {
		perror("fopen");
		exit(1);
	}

    for (i=0 ;; i++) {
		siz = fread(g_Buf + ( i * g_BlockSize ) % g_NBlocks, sizeof(char), g_BlockSize, f);
        if (feof(f)) {
            break;
        }
		
        queue_push(&g_Queue, &i);
	}

	int done = -1;
	for (i=0; i<g_NWorkers; i++) {
	    queue_push(&g_Queue, &done);
	}
	
	if (fclose(f) == -1) {
	    perror("fclose");
  		exit(1);
	}

    pthread_exit(NULL);
}


int
main(int argc, char* argv[])
{
    int i;
    pthread_attr_t a;
    //cpu_set_t c;

    if (5 != argc) {
        printf("Invalid usage:\n");
        printf(" %s [file name] [block size] [nworkers] [nblocks in memory]\n", argv[0]);
        return 1;
    }
    
    g_FileName = argv[1];
    g_BlockSize = atol(argv[2]);
    g_NWorkers = atoi(argv[3]);
    g_NBlocks = atoi(argv[4]);
    g_Buf = malloc(g_NBlocks*g_BlockSize*sizeof(char));
    
    printf("File name: %s\n", g_FileName);
    printf("Block size: %ld\n", g_BlockSize);
    printf("Number of workers: %d\n", g_NWorkers);
    
    queue_init(&g_Queue, sizeof(int), g_NBlocks);

    pthread_t producer_th;
    pthread_t consumer_ths[g_NWorkers];

    pthread_attr_init(&a);

    pthread_create(&producer_th, &a, producer, NULL);
    for (i=0; i<g_NWorkers; i++) {
        //CPU_ZERO(&c);
        //CPU_SET(core, &c);
        //pthread_attr_setaffinity_np(&a, sizeof(c), &c);
        pthread_create(&consumer_ths[i], &a, consumer, NULL);
    }

    pthread_join(producer_th, NULL);
    for (i=0; i<g_NWorkers; i++) {
        pthread_join(consumer_ths[i], NULL);
    }
    
    queue_destroy(&g_Queue);
        
    printf("Done!\n");
	return 0;
}

