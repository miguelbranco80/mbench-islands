all:	memxchg

memxchg: main.c dateutil.c
	gcc -g main.c dateutil.c -O0 -o memxchg -lnuma -lrt

clean:
	rm -f memxchg
