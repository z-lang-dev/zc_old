CFLAGS=-std=c11 -Wall -Wextra -Wpedantic -Werror -g -I.
CC=clang

all: zc zi

zc: zc.o lexer.o parser.o
	$(CC) -o zc.exe zc.o lexer.o parser.o

zi: zi.o lexer.o parser.o
	$(CC) -o zi.exe zi.o lexer.o parser.o

test: zc zi
	./test.sh

clean:
	rm -f *.o *.a *.so *.dll *.dylib *.exe *.s

.PHONY: test clean