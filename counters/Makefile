all:	mbench_sync mbench_spin mbench_mutex

mbench_sync: main.c config.o
	gcc main.c -O2 -Wall -Wextra -DAFFINITY -DSYNC_FETCH_ADD -o mbench_sync -lpthread -lrt -lnuma config.o
	gcc main.c -O2 -Wall -Wextra -DSYNC_FETCH_ADD -o mbench_sync_noaffinity -lpthread -lrt -lnuma config.o

mbench_spin: main.c config.o
	gcc main.c -O2 -Wall -Wextra -DAFFINITY -DPTHREAD_SPIN -o mbench_spin -lpthread -lrt -lnuma config.o
	gcc main.c -O2 -Wall -Wextra -DPTHREAD_SPIN -o mbench_spin_noaffinity -lpthread -lrt -lnuma config.o

mbench_mutex: main.c config.o
	gcc main.c -O2 -Wall -Wextra -DAFFINITY -DPTHREAD_MUTEX -o mbench_mutex -lpthread -lrt -lnuma config.o
	gcc main.c -O2 -Wall -Wextra -DPTHREAD_MUTEX -o mbench_mutex_noaffinity -lpthread -lrt -lnuma config.o

config.o: config.c config.h
	gcc -O2 -Wall -Wextra -c config.c -o config.o

clean:
	rm -f config.o
	rm -f mbench_sync
	rm -f mbench_spin
	rm -f mbench_mutex
	rm -f mbench_sync_noaffinity
	rm -f mbench_spin_noaffinity
	rm -f mbench_mutex_noaffinity

