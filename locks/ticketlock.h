#include "util.h"

typedef union ticketlock ticketlock;

union ticketlock
{
	unsigned u;
	struct
	{
		unsigned short ticket;
		unsigned short users;
	} s;
};

void ticket_lock(ticketlock *t);

void ticket_unlock(ticketlock *t);

int ticket_trylock(ticketlock *t);

int ticket_lockable(ticketlock *t);
