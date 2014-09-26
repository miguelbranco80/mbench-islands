#include "k42.h"

#define EBUSY 1
#define NULL (void *) 0

void k42_lock(k42lock *l)
{
	k42lock me;
	k42lock *pred, *succ;
	me.next = NULL;
	
	barrier();
	
	pred = xchg_64(&l->tail, &me);
	if (pred)
	{
		me.tail = (void *) 1;
		
		barrier();
		pred->next = &me;
		barrier();
		
		while (me.tail) cpu_relax();
	}
	
	succ = me.next;

	if (!succ)
	{
		barrier();
		l->next = NULL;
		
		if (cmpxchg(&l->tail, &me, &l->next) != &me)
		{
			while (!me.next) cpu_relax();
			
			l->next = me.next;
		}
	}
	else
	{
		l->next = succ;
	}
}


void k42_unlock(k42lock *l)
{
	k42lock *succ = l->next;
	
	barrier();
	
	if (!succ)
	{
		if (cmpxchg(&l->tail, &l->next, NULL) == (void *) &l->next) return;
		
		while (!l->next) cpu_relax();
		succ = l->next;
	}
	
	succ->tail = NULL;
}

int k42_trylock(k42lock *l)
{
	if (!cmpxchg(&l->tail, NULL, &l->next)) return 0;
	
	return EBUSY;
}

