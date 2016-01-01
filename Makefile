src=ae.c zmalloc.c url.c lib.c evhttp.c main.c
all:
	gcc $(src) -o evhttp -g
