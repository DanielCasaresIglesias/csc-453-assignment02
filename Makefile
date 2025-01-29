CC = gcc
CFLAGS = -Wall -g
LDFLAGS = -lncurses
LIBS = -L. -lPLN -lsnakes

# Source files
SRCS = lwp.c magic64.S

# Object files
OBJS = $(SRCS:.c=.o) magic64.o

# Executable files
EXEC = hungrymain snakemain numbersmain

# Build the executables
all: $(EXEC)

# Rule for building hungry snake demo
hungrymain: hungrymain.c $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS) $(LDFLAGS)

# Rule for building wandering snake demo
snakemain: snakemain.c $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS) $(LDFLAGS)

# Rule for building indented numbers demo
numbersmain: numbersmain.c $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS) $(LDFLAGS)

# Rule to build object files from source
magic64.o: magic64.S
	as -o $@ $^

# Clean up object files and executables
clean:
	rm -f $(OBJS) $(EXEC)
