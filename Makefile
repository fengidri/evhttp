CFLAGS += -fno-omit-frame-pointer -fsanitize=address -g -O1  -rdynamic
CFLAGS += -I ev -I ae
CFLAGS += -Lae -lae
CFLAGS += -Lev -levhttp

export CFLAGS

evhttp: ev/libevhttp.a ae/libae.a
	make -C ae
	make -C ev
	gcc  main.c $(CFLAGS) -o evhttp

clear:
	find -name '*.o' | xargs rm -rf
	rm evhttp
