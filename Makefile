src=ae.c zmalloc.c url.c lib.c evhttp.c main.c debug.c
all:
	gcc $(src) -o evhttp -g -rdynamic
