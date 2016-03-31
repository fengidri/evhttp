libsws.a: file.o net.o str.o sws.o
	ar cqs $@ $^

clean:
	find -name '*.o' | xargs rm -rf
	rm libsws.a
