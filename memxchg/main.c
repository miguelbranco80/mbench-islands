
#define MUTEX_T pthread_mutex_t
#define MUTEX_INIT(l) pthread_mutex_init(l, NULL)
#define MUTEX_DESTROY(l) pthread_mutex_destroy(l)
#define MUTEX_LOCK(l) pthread_mutex_lock
#define MUTEX_UNLOCK(l) pthread_mutex_unlock

#define SPIN_T pthread_spinlock_t
#define SPIN_INIT(l) pthread_spin_init(l, 0)
#define SPIN_DESTROY(l) pthread_spin_destroy(l)
#define SPIN_LOCK pthread_spin_lock
#define SPIN_UNLOCK pthread_spin_unlock

/*
#define LOCK_INIT(l) MUTEX_INIT(l)
#define LOCK_DESTROY(l) MUTEX_DESTROY(l)
#define LOCK(l) MUTEX_LOCK(l)
#define UNLOCK(l) MUTEX_UNLOCK(l)
#define LOCK_T MUTEX_T
*/

#define LOCK_INIT(l) SPIN_INIT(l)
#define LOCK_DESTROY(l) SPIN_DESTROY(l)
#define LOCK(l) SPIN_LOCK(l)
#define UNLOCK(l) SPIN_UNLOCK(l)
#define LOCK_T SPIN_T

#define _GNU_SOURCE
#include <pthread.h>
#include <numaif.h>

//#include <malloc.h>
#define _XOPEN_SOURCE 600
#include <stdlib.h>

#include <stdio.h>
#include "dateutil.h"

#define KB 1024
#define MB (1024*KB)

#define CACHE_LINE_SIZE 64

#define L1 64*KB
#define L2 512*KB

#define BLOCK_SIZE (L1+L2)

#define NLOOPS 5000

enum FLAG
{
	not_ready,
	ready
};

typedef char data[CACHE_LINE_SIZE];

struct thread_arg {
	LOCK_T lock1;
	LOCK_T lock2;
	LOCK_T lock3;

	data *buf1;
	data *buf2;
	data *buf3;
	
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	enum FLAG flag;

	int siz;
//}  __attribute((aligned(CACHE_LINE_SIZE)));
};


void *producer(void *in)
{
	int i, j;
	struct thread_arg *arg = (struct thread_arg *) in;

/*
	for (i=0; i<arg->siz; i++)
		arg->buf1[i][0] = i;

	for (i=0; i<arg->siz; i++)
		arg->buf2[i][0] = i;
*/

	pthread_mutex_lock(&arg->mutex);
	arg->flag = ready;
	pthread_cond_signal(&arg->cond);
	pthread_mutex_unlock(&arg->mutex);

	for (j=0; j<NLOOPS-1; j++) {
/*
		pthread_spin_lock(&arg->spin3);
		for (i=0; i<arg->siz; i++)
			arg->buf3[i][0] = i;
		pthread_spin_unlock(&arg->spin3);
*/

		LOCK(&arg->lock1);
		for (i=0; i<arg->siz; i++)
			arg->buf1[i][0]= i;
		UNLOCK(&arg->lock1);

		LOCK(&arg->lock2);
		for (i=0; i<arg->siz; i++)
			arg->buf2[i][0] = i;
		UNLOCK(&arg->lock2);
	}

	pthread_exit(NULL);
}


void *consumer(void *in)
{
	int i, j;
	data tmp;
	struct thread_arg *arg = (struct thread_arg *) in;

	pthread_mutex_lock(&arg->mutex);
	while (arg->flag != ready)
		pthread_cond_wait(&arg->cond, &arg->mutex);
	pthread_mutex_unlock(&arg->mutex);

	for (j=0; j<NLOOPS; j++) {
		LOCK(&arg->lock1);
		for (i=0; i<arg->siz; i++)
			tmp[0] = arg->buf1[i][0];
		UNLOCK(&arg->lock1);

		LOCK(&arg->lock2);
		for (i=0; i<arg->siz; i++)
			tmp[0] += arg->buf2[i][0];
		UNLOCK(&arg->lock2);
	}

	pthread_exit(NULL);
}


int main(int argc, char* argv[])
{
	pthread_t threads[2];
	pthread_attr_t attr;
	cpu_set_t c;
	unsigned long nodemask;
	struct thread_arg arg;
	struct timespec st, end;
	int i;

	if (argc != 4) {
		printf("Missing arguments: %s [memsocket] [core0] [core1]\n", argv[0]);
		return 2;
	}

	arg.siz = BLOCK_SIZE/sizeof(data);

	if (!(arg.buf1 = (data *) valloc(arg.siz*sizeof(data)))) {
		perror("valloc");
		return 2;
	}
	if (!(arg.buf2 = (data *) valloc(arg.siz*sizeof(data)))) {
		perror("valloc");
		return 2;
	}
	if (!(arg.buf3 = (data *) valloc(arg.siz*sizeof(data)))) {
		perror("valloc");
		return 2;
	}
	nodemask = 1 << atoi(argv[1]);
	if (mbind(arg.buf1, arg.siz*sizeof(data), MPOL_BIND,
		  &nodemask, sizeof(nodemask), MPOL_MF_MOVE | MPOL_MF_STRICT) != 0) {
		perror("mbind");
		return 2;
	}
	if (mbind(arg.buf2, arg.siz*sizeof(data), MPOL_BIND,
		  &nodemask, sizeof(nodemask), MPOL_MF_MOVE | MPOL_MF_STRICT) != 0) {
		perror("mbind");
		return 2;
	}
	if (mbind(arg.buf3, arg.siz*sizeof(data), MPOL_BIND,
		  &nodemask, sizeof(nodemask), MPOL_MF_MOVE | MPOL_MF_STRICT) != 0) {
		perror("mbind");
		return 2;
	}

	for (i=0; i<arg.siz; i++) {
		arg.buf1[i][0] = 0;
		arg.buf2[i][0] = 0;
		arg.buf3[i][0] = 0;
	}

/*
	if (posix_memalign(&arg.buf1, CACHE_LINE_SIZE, arg.siz*sizeof(data)) != 0) {
		perror("valloc");
		return 2;
	}
	if (posix_memalign(&arg.buf2, CACHE_LINE_SIZE, arg.siz*sizeof(data)) != 0) {
		perror("valloc");
		return 2;
	}
	if (posix_memalign(&arg.buf3, CACHE_LINE_SIZE, arg.siz*sizeof(data)) != 0) {
		perror("valloc");
		return 2;
	}
*/

/*
	arg.buf1 = malloc(arg.siz*sizeof(data));
	arg.buf2 = malloc(arg.siz*sizeof(data));
	arg.buf3 = malloc(arg.siz*sizeof(data));
*/

	LOCK_INIT(&arg.lock1);
	LOCK_INIT(&arg.lock2);
	LOCK_INIT(&arg.lock3);
	pthread_mutex_init(&arg.mutex, NULL);
	pthread_cond_init(&arg.cond, NULL);
	arg.flag = not_ready;

	pthread_attr_init(&attr);
	
	clock_gettime(CLOCK_REALTIME, &st);

	CPU_ZERO(&c);
	CPU_SET(atoi(argv[2]), &c);
	pthread_attr_setaffinity_np(&attr, sizeof(c), &c);
	pthread_create(&threads[0], &attr, producer, &arg);

	CPU_ZERO(&c);
	CPU_SET(atoi(argv[3]), &c);
	pthread_attr_setaffinity_np(&attr, sizeof(c), &c);
	pthread_create(&threads[1], &attr, consumer, &arg);

	CPU_ZERO(&c);
        CPU_SET(0, &c);
        pthread_setaffinity_np(pthread_self(), sizeof(c), &c);

	pthread_join(threads[0], NULL);
	pthread_join(threads[1], NULL);

	clock_gettime(CLOCK_REALTIME, &end);

	printf("%f\n", diff(st, end));

	LOCK_DESTROY(&arg.lock1);
	LOCK_DESTROY(&arg.lock2);
	LOCK_DESTROY(&arg.lock3);
	pthread_mutex_destroy(&arg.mutex);
	pthread_cond_destroy(&arg.cond);

	pthread_attr_destroy(&attr);

	pthread_exit(NULL);
}

