#include "util.h"

typedef struct k42lock k42lock;
struct k42lock
{
        k42lock *next;
        k42lock *tail;
};

void k42_lock(k42lock *l);

void k42_unlock(k42lock *l);

int k42_trylock(k42lock *l);
