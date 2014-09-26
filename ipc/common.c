#include "common.h"

int
read_config(int *msgsiz, int *nmsgs)
{
	FILE *f;

 	if ((f = fopen(CONFIG_FILE, "r")) == NULL)
		error("fopen");

	int n = fscanf(f, "%d %d", msgsiz, nmsgs);
	
	fclose(f);

	return n;
}


double
diff(struct timespec st, struct timespec end)
{
	struct timespec tmp;

	if ((end.tv_nsec - st.tv_nsec) < 0) {
		tmp.tv_sec = end.tv_sec - st.tv_sec - 1;
		tmp.tv_nsec = 1e9 + end.tv_nsec - st.tv_nsec;
	} else {
		tmp.tv_sec = end.tv_sec - st.tv_sec;
		tmp.tv_nsec = end.tv_nsec - st.tv_nsec;
	}

	return tmp.tv_sec + tmp.tv_nsec * 1e-9;
}


void
error(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

