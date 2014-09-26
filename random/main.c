#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <stdlib.h>

#define NCYCLES	500000000

#define PRNG_BUFSZ 256

typedef struct random_s {
        struct random_data	rand_state;
        char			rand_statebuf[PRNG_BUFSZ];
} random_t;

random_t r;

double
diff(struct timespec st, struct timespec end)
{
	struct timespec tmp;

	if ((end.tv_nsec - st.tv_nsec) < 0) {
		tmp.tv_sec = end.tv_sec - st.tv_sec - 1;
		tmp.tv_nsec = 1e9 + end.tv_nsec - st.tv_nsec;
	} else {
		tmp.tv_sec = end.tv_sec - st.tv_sec;
		tmp.tv_nsec = end.tv_nsec - st.tv_nsec;
	}

	return tmp.tv_sec + tmp.tv_nsec * 1e-9;
}

int
main(int argc, char *argv[])
{
	struct timespec st, end;
	int i;
	int j;
	int c = 0;
	unsigned int seed = 123;

	printf("Measuring cost of random number generation:\n");

        if (initstate_r(seed, (char *) &r.rand_statebuf, PRNG_BUFSZ, &r.rand_state) != 0) { 
                perror("initstate_r");
                exit(EXIT_FAILURE);
        }

	clock_gettime(CLOCK_REALTIME, &st);

	for (i = 0; i < NCYCLES; i++) {
		if (random_r(&r.rand_state, &j) != 0) {
			perror("random_r");
			exit(EXIT_FAILURE);
		}
		c += j;
	}

	clock_gettime(CLOCK_REALTIME, &end);

	printf("total time for %d cycles: %f s\n", NCYCLES, diff(st, end));
	printf("per cycle: %f ns\n", diff(st, end) / NCYCLES * 1e9);
	printf("(ignore this line; it's just to fool the compiler) c is %d\n", c);

	return 0;
}

