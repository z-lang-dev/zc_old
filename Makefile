CFLAGS=-std=c11 -Wall -Wextra -Wpedantic -Werror -g -I.
CC=clang

all: zc

zc: zc.o
	$(CC) $(CFLAGS) -o zc.exe zc.o

clean:
	rm -f *.o *.a *.so *.dll *.dylib *.exe *.s

.PHONY: clean