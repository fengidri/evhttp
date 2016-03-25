CFLAGS += -fno-omit-frame-pointer -g -O1  -rdynamic
CFLAGS += -I ev -I ae
CFLAGS += -Lae -lae
CFLAGS += -Lev -levhttp

#CFLAGS += -fsanitize=address

export CFLAGS

all:
	make -C ae
	make -C ev
	gcc  main.c $(CFLAGS) -o evhttp

install: evhttp
	cp evhttp /usr/local/bin

clear:
	find -name '*.o' | xargs rm -rf
	rm evhttp
