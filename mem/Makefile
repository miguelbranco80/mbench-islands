all:	main

main: main.c config.o
	gcc main.c -O2 -Wall -Wextra -DSYNC_FETCH_ADD -o main -lpthread -lrt -lnuma config.o

config.o: config.c config.h
	gcc -O2 -Wall -Wextra -c config.c -o config.o

clean:
	rm -f config.o
	rm -f main
