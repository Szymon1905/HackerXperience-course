# Compiler and Flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c11

TARGETS = attack

all: $(TARGETS)

attack: attack.c
	$(CC) $(CFLAGS) attack.c -o attack


clean:
	rm -f $(TARGETS) *.o

.PHONY: all clean