CFLAGS += -I ../ae -I ../sws

libevhttp.a: url.o evhttp.o ev.o parser.o format.o
	ar cqs $@ $^

clean:
	find -name '*.o' | xargs rm -rf
	rm libevhttp.a
