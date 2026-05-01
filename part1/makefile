# Compiler and Flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c11
# Define a directory for binaries
BIN_DIR = bin

# Executable names
TARGETS = hello-world international-hello-world fancy-hello-world

# builds all
all: $(TARGETS)

# Target for normal Hello World
hello-world: hello-world.c hello-world.h
	mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) hello-world.c -o $(BIN_DIR)/hello-world

# Target for international Hello World
international-hello-world: international-hello-world.c international-hello-world.h
	mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) international-hello-world.c -o $(BIN_DIR)/international-hello-world

# Target for fancy Hello World
fancy-hello-world: fancy-hello-world.c
	mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) fancy-hello-world.c -o $(BIN_DIR)/fancy-hello-world


# to clean up build files
clean:
	rm -f $(TARGETS) *.o

.PHONY: all clean