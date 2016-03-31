CFLAGS += -fno-omit-frame-pointer -g -rdynamic
CFLAGS += -I ev -I ae -I sws
CFLAGS += -Lae -lae
CFLAGS += -Lev -levhttp
CFLAGS += -Lsws -lsws

ifdef DEV
CFLAGS += -fsanitize=address -O0
endif

export CFLAGS

evhttp: main.c ae/libae.a ev/libevhttp.a sws/libsws.a
	gcc  main.c $(CFLAGS) -o evhttp

ae/libae.a:
	make -C ae

ev/libevhttp.a:
	make -C ev

sws/libsws.a:
	make -C sws

.PHONY: ae/libae.a ev/libevhttp.a sws/libsws.a

install: evhttp
	cp evhttp /usr/local/bin

clean:
	find -name '*.o' | xargs rm -rf
	rm evhttp
