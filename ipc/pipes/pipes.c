#define _GNU_SOURCE
#include <sched.h>

#include <errno.h>
#include <sys/types.h>
#include <time.h>
#include <strings.h>

#include "common.h"


#define MAX_CPUS	256


int
main(int argc, char* argv[])
{
	if (argc != 3) {
		printf("Usage:\n%s [server cpu id] [client cpu id]\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	
	int buffersiz, nmessages;
	read_config(&buffersiz, &nmessages);

	int srv_cpu = atoi(argv[1]);
	int cli_cpu = atoi(argv[2]);
	if (srv_cpu >= MAX_CPUS || cli_cpu >= MAX_CPUS) {
		printf("CPU number too large: redefine MAX_CPUS.\n");
		exit(EXIT_FAILURE);
	}

	int cli2srv[2];
	int srv2cli[2];
	char data[buffersiz];
	struct timespec st, end;	
	unsigned long i;
	int n;

	cpu_set_t *mask = CPU_ALLOC(MAX_CPUS);
	size_t size = CPU_ALLOC_SIZE(MAX_CPUS);
	CPU_ZERO_S(size, mask);

	pipe(cli2srv);
	pipe(srv2cli);

	if (fork() == 0) {
	
		/* Client process. */

		CPU_SET_S(cli_cpu, size, mask);
		if (sched_setaffinity(0, size, mask) < 0)
			error("sched_setaffinity");

		clock_gettime(CLOCK_REALTIME, &st);
		for (i = 0; i < nmessages; i++) {

			/* Send message. */

			bzero(data, buffersiz);
			sprintf(data, "Hi");
			if ((n = write(cli2srv[1], data, buffersiz)) < 0)
				error("write");

			/* Read acknowledgment. */

			if ((n = read(srv2cli[0], data, buffersiz)) < 0)
				error("read");
		}
		clock_gettime(CLOCK_REALTIME, &end);
		printf("%.9f\n", ( diff(st, end) / nmessages ) );

	} else {

		/* Server process. */

		CPU_SET_S(srv_cpu, size, mask);
		if (sched_setaffinity(0, size, mask) < 0)
			error("sched_setaffinity");

		for (i = 0; i < nmessages; i++) {

			/* Read message. */

			if ((n = read(cli2srv[0], data, buffersiz)) < 0)
				error("read");

			/* Send back acknowledgment. */

			bzero(data, buffersiz);
			sprintf(data, "ACK");
			if ((n = write(srv2cli[1], data, buffersiz)) < 0)
				error("write");
		}
	}

	return 0;
}
