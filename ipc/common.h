#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define CONFIG_FILE	"settings.cfg"

int	read_config(int *msgsiz, int *nmsgs);

double	diff(struct timespec st, struct timespec end);

void	error(const char *msg);

