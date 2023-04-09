CFLAGS=-std=c11 -g -static
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

zc: $(OBJS)
	$(CC) -o zc $(OBJS) $(LDFLAGS)

$(OBJS): zc.h

test: zc
	./test.sh

clean:
	rm -f zc *.o *~ tmp*

.PHONY: test clean
