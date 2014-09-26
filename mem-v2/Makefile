all:	affinity no_affinity

CC = gcc -O2 -g -Wall -Wextra

affinity: main.c config.o
	$(CC) main.c -DAFFINITY -o affinity -lpthread -lrt -lnuma config.o

no_affinity: main.c config.o
	$(CC) main.c -o no_affinity -lpthread -lrt -lnuma config.o

config.o: config.c config.h
	$(CC) -c config.c -o config.o

clean:
	rm -f config.o
	rm -f affinity
	rm -f no_affinity

