CFLAGS=-std=c11 -Wall -Wextra -Wpedantic -Werror -g -I.
CC=clang

all: zc zi

zc: zc.o
	$(CC) -o zc.exe zc.o

zi: zi.o
	$(CC) -o zi.exe zi.o

test: zc zi
	./test.sh

clean:
	rm -f *.o *.a *.so *.dll *.dylib *.exe *.s

.PHONY: test clean