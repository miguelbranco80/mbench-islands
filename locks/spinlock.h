typedef unsigned spinlock;

void spin_lock(spinlock *lock);

void spin_unlock(spinlock *lock);

int spin_trylock(spinlock *lock);

