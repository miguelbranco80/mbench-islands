UNAME := $(shell uname -s)

all:	affinity no_affinity

CC = gcc -O2 -g -Wall -Wextra

affinity: main.c config.o myrand.o
ifeq ($(UNAME),SunOS)
	$(CC) -DAFFINITY -o $@ -lpthread $^
endif
ifeq ($(UNAME),Linux)
	$(CC) -DAFFINITY -o $@ -lpthread -lrt -lnuma $^
endif

no_affinity: main.c config.o myrand.o
ifeq ($(UNAME),SunOS)
	$(CC) -o $@ -lpthread $^
endif
ifeq ($(UNAME),Linux)
	$(CC) -o $@ -lpthread -lrt -lnuma $^
endif

config.o: config.c config.h
	$(CC) -c config.c -o $@

myrand.o: myrand.c myrand.h
	$(CC) -c myrand.c -o $@

test_myrand: test_myrand.c myrand.o
	$(CC) -o $@ $^

clean:
	rm -f config.o
	rm -f myrand.o
	rm -f affinity
	rm -f no_affinity

