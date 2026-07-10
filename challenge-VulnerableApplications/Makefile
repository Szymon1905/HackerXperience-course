# Compiler and Flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c11

TARGETS = main

all: $(TARGETS)

VulApp: main.c
	$(CC) $(CFLAGS) main.c -o VulApp


clean:
	rm -f $(TARGETS) *.o

.PHONY: all clean