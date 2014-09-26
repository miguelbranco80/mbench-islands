#include <stdio.h>
#include <pthread.h>
#include <time.h>

#define NCYCLES	500000000

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
	pthread_mutex_t mutex;
	int i;

	printf("Measuring cost of pthread mutex calls for *uncontended* accesses:\n");

	pthread_mutex_init(&mutex, NULL);

	clock_gettime(CLOCK_REALTIME, &st);

	for (i = 0; i < NCYCLES; i++) {
		pthread_mutex_lock(&mutex);
		pthread_mutex_unlock(&mutex);
	}

	clock_gettime(CLOCK_REALTIME, &end);

	printf("total time for %d lock/unlock: %f s\n", NCYCLES, diff(st, end));
	printf("time per lock/unlock: %f ns\n", diff(st, end) / NCYCLES * 1e9);

	return 0;
}

