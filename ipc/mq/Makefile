all: client server

client: client.c ../common.h
	cc -g -Wall client.c ../common.c -I.. -o client -lrt

server: server.c ../common.h
	cc -g -Wall server.c ../common.c -I.. -o server -lrt

