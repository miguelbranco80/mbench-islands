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
	
	if (mkfifo(cli2srv, 0666) < 0)
		error("mkfifo");
	if (mkfifo(srv2cli, 0666) < 0)
		error("mkfifo");

	int cli2srv_fd;
	if ((cli2srv_fd = open(cli2srv, O_RDWR)) < 0)
		error("open");
	int srv2cli_fd;
	if ((srv2cli_fd = open(srv2cli, O_RDWR)) < 0)
		error("open");

	char data[buffersiz];
	int n;
	unsigned long i;
	for (i = 0; i < nmessages; i++) {

		/* Read message. */

		if ((n = read(cli2srv_fd, data, buffersiz)) < 0)
			error("read");

		/* Send back acknowledgment. */

		bzero(data, buffersiz);
		sprintf(data, "ACK");
		if ((n = write(srv2cli_fd, data, buffersiz)) < 0)
			error("write");

	}

	close(cli2srv_fd);
	close(srv2cli_fd);

	remove(cli2srv);
	remove(srv2cli);

	return 0; 
}
