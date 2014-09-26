all:	sync_fetch_add pthread_spinlock pthread_mutex custom_spinlock k42 ticketlock

sync_fetch_add: main.c
	gcc main.c -O0 -DUSE_AFFINITY -DUSE_SYNC_FETCH_ADD -o sync_fetch_add -lpthread -lrt -lnuma

pthread_spinlock: main.c
	gcc main.c -O0 -DUSE_AFFINITY -DUSE_PTHREAD_SPINLOCK -o pthread_spinlock -lpthread -lrt -lnuma

pthread_mutex: main.c
	gcc main.c -O0 -DUSE_AFFINITY -DUSE_PTHREAD_MUTEX -o pthread_mutex -lpthread -lrt -lnuma

custom_spinlock: main.c spinlock.c
	gcc main.c spinlock.c -O0 -DUSE_AFFINITY -DUSE_CUSTOM_SPINLOCK -o custom_spinlock -lpthread -lrt -lnuma

k42: main.c k42.c
	gcc main.c k42.c -O0 -DUSE_AFFINITY -DUSE_K42 -o k42 -lpthread -lrt -lnuma

ticketlock: main.c ticketlock.c
	gcc main.c ticketlock.c -O0 -DUSE_AFFINITY -DUSE_TICKETLOCK -o ticketlock -lpthread -lrt -lnuma
