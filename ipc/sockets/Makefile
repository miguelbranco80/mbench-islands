all: tcp-client tcp-server unix-client unix-server

tcp-client: client.c ../common.h
	cc -g -Wall -DTCP client.c ../common.c -I.. -o tcp-client -lrt

tcp-server: server.c ../common.h
	cc -g -Wall -DTCP server.c ../common.c -I.. -o tcp-server -lrt

unix-client: client.c ../common.h
	cc -g -Wall client.c ../common.c -I.. -o unix-client -lrt

unix-server: server.c ../common.h
	cc -g -Wall server.c ../common.c -I.. -o unix-server -lrt

clean:
	rm -f tcp-client tcp-server unix-client unix-server
