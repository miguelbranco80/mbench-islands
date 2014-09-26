#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "common.h"


int
main(int argc, char *argv[])
{
	if (argc != 2) {
		printf("Usage:\n%s [fifo path]\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	int buffersiz, nmessages;
	read_config(&buffersiz, &nmessages);

	char cli2srv[255];
	char srv2cli[255];
	sprintf(cli2srv, "%s-cli2srv", argv[1]);
	sprintf(srv2cli, "%s-srv2cli", argv[1]);
	
	int cli2srv_fd;
	if ((cli2srv_fd = open(cli2srv, O_RDWR)) < 0)
		error("open");
	int srv2cli_fd;
	if ((srv2cli_fd = open(srv2cli, O_RDWR)) < 0)
		error("open");

	char data[buffersiz];
	int n;
	unsigned long i;

	struct timespec st, end;
	clock_gettime(CLOCK_REALTIME, &st);

	for (i = 0; i < nmessages; i++) {

		/* Send message. */

		bzero(data, buffersiz);
		sprintf(data, "Hi");
		if ((n = write(cli2srv_fd, data, buffersiz)) < 0)
			error("write");

		/* Read acknowledgment. */

		if ((n = read(srv2cli_fd, data, buffersiz)) < 0)
			error("read");
	}

	clock_gettime(CLOCK_REALTIME, &end);
	printf("%.9f\n", ( diff(st, end) / nmessages ) );

	close(cli2srv_fd);
	close(srv2cli_fd);

	return 0;
}
