#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <mqueue.h>

#include "common.h"


int
main(int argc, char *argv[])
{
	if (argc != 2) {
		printf("Usage:\n%s [mq path]\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	int buffersiz, nmessages;
	read_config(&buffersiz, &nmessages);

	struct mq_attr attr;
	attr.mq_flags   = 0;
	attr.mq_maxmsg  = 2;
	attr.mq_msgsize = buffersiz;
	attr.mq_curmsgs = 0;

	mqd_t mq;
	if ((mq = mq_open(argv[1], O_RDWR, 0666, &attr)) < 0)
		error("mq_open");

	char data[buffersiz];
	unsigned long i;
	unsigned prio;

	struct timespec st, end;
	clock_gettime(CLOCK_REALTIME, &st);

	for (i = 0; i < nmessages; i++) {

		/* Send message. */

		bzero(data, buffersiz);
		sprintf(data, "Hi");
		if (mq_send(mq, data, buffersiz, 1) < 0)
			error("mq_send");

		/* Read acknowledgment. */

		if (mq_receive(mq, data, buffersiz, &prio) < 0)
			error("mq_receive");
	}

	clock_gettime(CLOCK_REALTIME, &end);
	printf("%.9f\n", ( diff(st, end) / nmessages ) );

	if (mq_close(mq) < 0)
		error("mq_close");

	return 0;
}
