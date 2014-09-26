#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <inttypes.h>

char *g_FileName;
long g_BlockSize;
int g_NWorkers;
int g_NBlocksPerWorker;
char *g_Buf;

void*
worker(void *arg)
{
	FILE *f;
	size_t i, siz;
    uintptr_t id = (uintptr_t) arg;

	f = fopen(g_FileName, "r");
	if (0 == f) {
		perror("fopen");
		exit(1);
	}

	for (i=0; i<g_NBlocksPerWorker; i++) {
		if (fseek(f, (i*g_NWorkers+id)*g_BlockSize, SEEK_SET) == -1) {
			perror("fseek");
    		exit(1);
		}
		siz = fread(g_Buf+id*g_BlockSize, sizeof(char), g_BlockSize, f);
		if (feof(f)) {
			printf("EOF\n");
			break;
		}
		if (siz != g_BlockSize) {
			perror("fread");
    		exit(1);
		}
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
    uintptr_t i;
    pthread_attr_t a;
    cpu_set_t c;

    if (5 != argc) {
        printf("Invalid usage:\n");
        printf(" %s [file name] [block size] [nblocks per worker] [nworkers]\n", argv[0]);
        return 1;
    }
    
    g_FileName = argv[1];
    g_BlockSize = atol(argv[2]);
    g_NBlocksPerWorker = atoi(argv[3]);
    g_NWorkers = atoi(argv[4]);
    g_Buf = malloc(g_NWorkers*g_BlockSize*sizeof(char));
    
    printf("File name: %s\n", g_FileName);
    printf("Block size: %ld\n", g_BlockSize);
    printf("Number of blocks per worker: %d\n", g_NBlocksPerWorker);
    printf("Number of workers: %d\n", g_NWorkers);
    printf("(Total size: %ld bytes)\n", g_BlockSize*g_NBlocksPerWorker*g_NWorkers);

    pthread_t workers[g_NWorkers];
    
    pthread_attr_init(&a);
    for (i=0; i<g_NWorkers; i++) {
        //CPU_ZERO(&c);
        //CPU_SET(core, &c);
        //pthread_attr_setaffinity_np(&a, sizeof(c), &c);
        pthread_create(&workers[i], &a, worker, (void*)i);
    }

    for (i=0; i<g_NWorkers; i++) {
        pthread_join(workers[i], NULL);
    }
    
    free(g_Buf);
    
    printf("Done!\n");
	return 0;
}
