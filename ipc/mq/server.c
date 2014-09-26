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

	mq_unlink(argv[1]); /* Just to make sure it is clean. */

	mqd_t mq;
	if ((mq = mq_open(argv[1], O_CREAT | O_EXCL | O_RDWR, 0666, &attr)) < 0)
		error("mq_open");

	char data[buffersiz];
	unsigned long i;
	unsigned prio;

	for (i = 0; i < nmessages; i++) {

		/* Read message. */

		if (mq_receive(mq, data, buffersiz, &prio) < 0)
			error("mq_receive");

		/* Send back acknowledgment. */

		bzero(data, buffersiz);
		sprintf(data, "ACK");
		if (mq_send(mq, data, buffersiz, 1) < 0)
			error("mq_send");

	}

	if (mq_close(mq) < 0)
		error("mq_close");

	if (mq_unlink(argv[1]) < 0)
		error("mq_unlink");

	return 0; 
}
