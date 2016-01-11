src=ae.c zmalloc.c url.c lib.c evhttp.c main.c debug.c
CFLAGS=-fno-omit-frame-pointer -fsanitize=address -g -O1  -rdynamic
all:
	gcc $(CFLAGS) $(src) -o evhttp

