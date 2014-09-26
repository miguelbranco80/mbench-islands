#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "common.h"


int
main(int argc, char *argv[])
{
	if (argc != 2) {
		printf("Usage:\n%s [shm path]\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	int buffersiz, nmessages;
	read_config(&buffersiz, &nmessages);

	int fd;
	if ((fd = shm_open(argv[1], O_CREAT | O_RDWR, 0666)) < 0)
		error("shm_open");

	char *shdata;
	if ((shdata = mmap(NULL, buffersiz * 2,
			   PROT_READ | PROT_WRITE, MAP_SHARED,
			   fd, 0)) == MAP_FAILED)
		error("mmap");
	
	struct timespec st, end;
	clock_gettime(CLOCK_REALTIME, &st);

	unsigned long i;
	for (i = 0; i < nmessages; i++) {

		/* Write message. */

		bzero(shdata, buffersiz);
		sprintf(shdata, "Hi");

		/* Signal that message written, flipping to 1. */

		__sync_lock_test_and_set(&shdata[buffersiz], 1);

		/* Wait for new message flag, i.e. 0. and flip to 0. */

		while (!__sync_bool_compare_and_swap(&shdata[buffersiz], 0, 0))
			;

		/* Read message. */		

		//printf("Got '%s' from server.\n", shdata);
	}

	clock_gettime(CLOCK_REALTIME, &end);
	printf("%.9f\n", ( diff(st, end) / nmessages ) );

	if (munmap(shdata, buffersiz * 2) < 0)
		error("munmap");

	return 0;
}
