#include "ticketlock.h"

#define EBUSY 1

void ticket_lock(ticketlock *t)
{
	unsigned short me = atomic_xadd(&t->s.users, 1);
	
	while (t->s.ticket != me) cpu_relax();
}

void ticket_unlock(ticketlock *t)
{
	barrier();
	t->s.ticket++;
}

int ticket_trylock(ticketlock *t)
{
	unsigned short me = t->s.users;
	unsigned short menew = me + 1;
	unsigned cmp = ((unsigned) me << 16) + me;
	unsigned cmpnew = ((unsigned) menew << 16) + me;

	if (cmpxchg(&t->u, cmp, cmpnew) == cmp) return 0;
	
	return EBUSY;
}

int ticket_lockable(ticketlock *t)
{
	ticketlock u = *t;
	barrier();
	return (u.s.ticket == u.s.users);
}

