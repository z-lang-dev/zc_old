CFLAGS=-std=c11 -g -static -fno-common

zc: main.o
	$(CC) $(CFLAGS) -o $@ $?

test: zc
	./test.sh

clean:
	rm -f zc *.o *~ tmp*

.PHONY: test clean
