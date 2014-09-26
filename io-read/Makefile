all:	T1_simple \
	T1_queue

CC = gcc -g -Wall -Wextra

T1_simple:	T1_simple.c
	$(CC) $? -pthread -o $@

T1_queue:	queue.c queue.h T1_queue.c
	$(CC) queue.c T1_queue.c -pthread -o $@

