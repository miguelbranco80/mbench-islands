#include <string.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>

#include "common.h"


int
main(int argc, char *argv[])
{
	if (argc != 2) {
#if defined(TCP)
		printf("Usage:\n%s [port]\n", argv[0]);
#else
		printf("Usage:\n%s [socket path]\n", argv[0]);
#endif
		exit(EXIT_FAILURE);
	}
	
	int buffersiz, nmessages;
	read_config(&buffersiz, &nmessages);

	int sockfd;

#if defined(TCP) 
	int portno = atoi(argv[1]);
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		error("socket");
#else
	if ((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
		error("socket");
#endif

#if defined(TCP) 
	struct sockaddr_in serv_addr, cli_addr;
#else
	struct sockaddr_un serv_addr, cli_addr;
#endif

	bzero(&serv_addr, sizeof(serv_addr));
	
	int servlen;
#if defined(TCP)
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);
	servlen = sizeof(serv_addr);
#else
	serv_addr.sun_family = AF_UNIX;
	strcpy(serv_addr.sun_path, argv[1]);
	servlen = strlen(serv_addr.sun_path) + sizeof(serv_addr.sun_family);
#endif

	if (bind(sockfd, (struct sockaddr *) &serv_addr, servlen) < 0)
	      error("bind");

	listen(sockfd, 5);

	char data[buffersiz];
	int n;
	socklen_t clilen = sizeof(cli_addr);

	/* Wait for connection. */

	int nsockfd;
	if ((nsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, 
	 		      &clilen)) < 0)
		error("accept");

	unsigned long i;
	for (i = 0; i < nmessages; i++) {

		/* Read message. */

		if ((n = read(nsockfd, data, buffersiz)) < 0)
			error("read");

		/* Send back acknowledgment. */

		bzero(data, buffersiz);
		sprintf(data, "ACK");
		if ((n = write(nsockfd, data, buffersiz)) < 0)
			error("write");

	}

	sleep(1); /* Just give a bit of time so that client closes first. */

	close(nsockfd);
	close(sockfd);

	return 0; 
}
