CC = gcc
CFLAGS = -Wall -Wextra -std=c2x -O3


TARGETS = DHCPstarve DHCPmine

all: $(TARGETS)


DHCPstarve: DHCPstarve.c
	$(CC) $(CFLAGS) DHCPstarve.c -o DHCPstarve

DHCPmine: DHCPmine.c
	$(CC) $(CFLAGS) DHCPmine.c -o DHCPmine

clean:
	rm -f $(TARGETS) *.o

.PHONY: all clean
