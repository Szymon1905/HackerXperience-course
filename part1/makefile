# Compiler and Flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c11


# Executable names
TARGETS = hello-world international-hello-world fancy-hello-world

# builds all
all: $(TARGETS)

# Target for normal Hello World
hello-world: hello-world.c hello-world.h
	$(CC) $(CFLAGS) hello-world.c -o hello-world

# Target for international Hello World
international-hello-world: international-hello-world.c international-hello-world.h
	$(CC) $(CFLAGS) international-hello-world.c -o international-hello-world

# Target for fancy Hello World
fancy-hello-world: fancy-hello-world.c
	$(CC) $(CFLAGS) fancy-hello-world.c -o fancy-hello-world


# to clean up build files
clean:
	rm -f $(TARGETS) *.o

.PHONY: all clean