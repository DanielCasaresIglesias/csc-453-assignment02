# Compiler settings
CC = gcc
CFLAGS = -Wall -g -fPIC  # -fPIC for position-independent code
LDFLAGS = -shared  # Shared library flag

# Source files for the library
SRCS = lwp.c magic64.S

# Object files generated from the source files
OBJS = $(SRCS:.c=.o) magic64.o

# Library name
LIB_NAME = liblwp.so

# Build the library
all: $(LIB_NAME)

# Rule to build the shared library
$(LIB_NAME): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(LIB_NAME) $(LDFLAGS)

# Rule to compile magic64.S into object file using gcc
magic64.o: magic64.S
	$(CC) -o magic64.o -c magic64.S

# Clean up generated files
clean:
	rm -f $(OBJS) $(LIB_NAME)

# Install the shared library (optional, modify the path if necessary)
install: $(LIB_NAME)
	cp $(LIB_NAME) /usr/local/lib/

# Rule to test the build (optional, for convenience)
test: $(LIB_NAME)
	@echo "Library $(LIB_NAME) built successfully."
