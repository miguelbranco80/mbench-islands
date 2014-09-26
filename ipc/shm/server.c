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

	if (ftruncate(fd, buffersiz * 2) < 0)
		error("ftruncate");

	char *shdata;
	if ((shdata = mmap(NULL, buffersiz * 2,
			   PROT_READ | PROT_WRITE, MAP_SHARED,
			   fd, 0)) == MAP_FAILED)
		error("mmap");

	/* Initialize flag to 0. */

	shdata[buffersiz] = 0;

	unsigned long i;
	for (i = 0; i < nmessages; i++) {

		/* Wait for new message flag, i.e. 1. and flip to 1. */

		while (!__sync_bool_compare_and_swap(&shdata[buffersiz], 1, 1))
			;

		/* Read message. */

		/* Write message. */

		bzero(shdata, buffersiz);
		sprintf(shdata, "ACK");

		/* Signal that message written, flipping to 0. */

		__sync_lock_test_and_set(&shdata[buffersiz], 0);

	}

	if (munmap(shdata, buffersiz * 2) < 0)
		error("munmap");

	if (shm_unlink(argv[1]) < 0)
		error("shm_unlink");

	return 0; 
}
