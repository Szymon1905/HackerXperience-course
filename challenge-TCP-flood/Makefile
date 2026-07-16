CC = gcc
CFLAGS = -Wall -Wextra -std=c2x -O3


TARGETS = TCPflood 

all: $(TARGETS)


TCPflood: TCPflood.c
	$(CC) $(CFLAGS) TCPflood.c -o TCPflood

clean:
	rm -f $(TARGETS) *.o

.PHONY: all clean
