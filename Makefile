CFLAGS=-std=c11 -Wall -Wextra -Wpedantic -Werror -g -I.
CC=clang
LIB_OBJS= util.o lexer.o parser.o type.o value.o interp.o cmd.o

all: zc zi

zc: zc.o $(LIB_OBJS)
	$(CC) -o zc.exe zc.o $(LIB_OBJS)

zi: zi.o $(LIB_OBJS)
	$(CC) -o zi.exe zi.o $(LIB_OBJS)

test: zc zi
	./test.sh

clean:
	rm -f *.o *.a *.so *.dll *.dylib *.exe *.s

.PHONY: test clean