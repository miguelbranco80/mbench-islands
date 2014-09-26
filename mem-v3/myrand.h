#include <stdint.h>

typedef struct myrandstate_s {
	unsigned int m_z;
	unsigned int m_w;
} myrandstate_t;

void init_myrand(myrandstate_t *r, unsigned int seed1, unsigned int seed2);

// 0 <= u < 2^32
unsigned int get_uint(myrandstate_t *r);

// result is strictly between 0 and 1
float get_uniform(myrandstate_t *r);

