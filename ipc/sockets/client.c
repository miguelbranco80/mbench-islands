#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netdb.h> 

#include "common.h"


int
main(int argc, char *argv[])
{
#if defined(TCP)
	if (argc != 3) {
		printf("Usage:\n%s [hostname] [port]\n", argv[0]);
		exit(EXIT_FAILURE);
	}
#else
	if (argc != 2) {
		printf("Usage:\n%s [socket path]\n", argv[0]);
		exit(EXIT_FAILURE);
	}
#endif

	int buffersiz, nmessages;
	read_config(&buffersiz, &nmessages);

	int sockfd;

#if defined(TCP)
	int portno = atoi(argv[2]);
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		error("socket");
#else
	if ((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
		error("socket");
#endif

#if defined(TCP) 
	struct sockaddr_in serv_addr;
#else
	struct sockaddr_un serv_addr;
#endif

#if defined(TCP)
	struct hostent *server;
	if ((server = gethostbyname(argv[1])) == NULL)
		error("gethostbyname: invalid hostname");
#endif

	bzero(&serv_addr, sizeof(serv_addr));

	int servlen;
#if defined(TCP)
	serv_addr.sin_family = AF_INET;
	bcopy(server->h_addr, &serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port = htons(portno);
	servlen = sizeof(serv_addr);
#else
	serv_addr.sun_family = AF_UNIX;
	strcpy(serv_addr.sun_path, argv[1]);
	servlen = strlen(serv_addr.sun_path) + sizeof(serv_addr.sun_family);
#endif

	if (connect(sockfd, (struct sockaddr *) &serv_addr, servlen) < 0) 
		error("connect");

	char data[buffersiz];
	int n;
	unsigned long i;

	struct timespec st, end;
	clock_gettime(CLOCK_REALTIME, &st);

	for (i = 0; i < nmessages; i++) {

		/* Send message. */

		bzero(data, buffersiz);
		sprintf(data, "Hi");
		if ((n = write(sockfd, data, buffersiz)) < 0)
			error("write");

		/* Read acknowledgment. */

		if ((n = read(sockfd, data, buffersiz)) < 0)
			error("read");
	}

	clock_gettime(CLOCK_REALTIME, &end);
	printf("%.9f\n", ( diff(st, end) / nmessages ) );

	close(sockfd);

	return 0;
}
