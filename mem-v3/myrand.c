#include "myrand.h"

void init_myrand(myrandstate_t *r, unsigned int seed1, unsigned int seed2)
{
	r->m_z = seed1;
	r->m_w = seed2;
}

unsigned int get_uint(myrandstate_t *r)
{
	r->m_z = 36969 * (r->m_z & 65535) + (r->m_z >> 16);
	r->m_w = 18000 * (r->m_w & 65535) + (r->m_w >> 16);
	return (r->m_z << 16) + r->m_w;
}

float get_uniform(myrandstate_t *r)
{
	// 0 <= u < 2^32
	unsigned int u = get_uint(r);
	// The magic number below is 1/(2^32 + 2).
	// The result is strictly between 0 and 1.
	return (u + 1.0) * 2.328306435454494e-10;
}

