# Compiler settings
CC = gcc
CFLAGS = -Wall -g -fPIC  # -fPIC for position-independent code
LDFLAGS = -shared  # Shared library flag
LIBS = -lncurses  # Link with ncurses (no need for libsnakes or libPLN if they are not separate)
INCLUDES = -I.  # Include current directory

# Source files for the library
SRCS = magic64.S lwp.c

# Object files generated from the source files
OBJS = $(SRCS:.c=.o) $(SRCS:.S=.o)

# Library name
LIB_NAME = liblwp.so

# Build the library
all: $(LIB_NAME)

# Rule to build the shared library
$(LIB_NAME): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(LIB_NAME) $(LDFLAGS) $(LIBS)

# Rule to compile magic64.S into object file using gcc
magic64.o: magic64.S
	$(CC) $(CFLAGS) $(INCLUDES) -c magic64.S -o magic64.o

# Rule to compile lwp.c into object file
lwp.o: lwp.c
	$(CC) $(CFLAGS) $(INCLUDES) -c lwp.c -o lwp.o

# Clean up generated files
clean:
	rm -f $(OBJS) $(LIB_NAME)

# Install the shared library (optional, modify the path if necessary)
install: $(LIB_NAME)
	cp $(LIB_NAME) /usr/local/lib/

# Rule to test the build (optional, for convenience)
test: $(LIB_NAME)
	@echo "Library $(LIB_NAME) built successfully."
