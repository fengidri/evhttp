CFLAGS=-g
libsws.a: file.o net.o str.o sws.o argparser.o
	ar cqs $@ $^

clean:
	find -name '*.o' | xargs rm -rf
	rm libsws.a
