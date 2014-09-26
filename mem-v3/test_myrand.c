#include <stdio.h>
#include <stdint.h>

#include "myrand.h"

int main()
{
	int i;
	myrandstate_t r;

        int run = 500000;
        int nslots = 10;
	printf("\nGenerating %d random numbers in %d slots.\n", run, nslots);

	init_myrand(&r, 123, 456);

        int slots[nslots];
        for (i = 0; i < nslots; i++)
		slots[i] = 0;

        for (i = 0; i < run; i++) {
                uint32_t v = (uint32_t) (get_uniform(&r) * nslots);
                slots[v] += 1;
        }

	printf("\nSlots:\n======\n");

        for (i = 0; i < nslots; i++)
		printf("[%d]\t%d\n", i, slots[i]);
}

