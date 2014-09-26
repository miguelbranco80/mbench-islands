#include "spinlock.h"
#include "util.h"

#define EBUSY 1

void spin_lock(spinlock *lock)
{
	while (1)
	{
		if (!xchg_32(lock, EBUSY)) return;
	
		while (*lock) cpu_relax();
	}
}

void spin_unlock(spinlock *lock)
{
	barrier();
	*lock = 0;
}

int spin_trylock(spinlock *lock)
{
	return xchg_32(lock, EBUSY);
}

