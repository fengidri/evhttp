src=ae.c zmalloc.c url.c common.c evhttp.c main.c debug.c ev.c
CFLAGS=-fno-omit-frame-pointer -fsanitize=address -g -O1  -rdynamic
all:
	gcc $(CFLAGS) $(src) -o evhttp

