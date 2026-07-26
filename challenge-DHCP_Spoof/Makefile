CC = gcc
CFLAGS = -Wall -Wextra -std=c2x -O3


TARGETS = DHCPstarve 

all: $(TARGETS)


TCPflood: DHCPstarve.c
	$(CC) $(CFLAGS) DHCPstarve.c -o DHCPstarve

clean:
	rm -f $(TARGETS) *.o

.PHONY: all clean
