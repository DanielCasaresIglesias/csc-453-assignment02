CC = gcc
CFLAGS = -Wall -g
LDFLAGS = -lncurses
LIBS = -L. -lPLN -lsnakes

# Source files
SRCS = lwp.c ~pn-cs453/Given/Asgn2/src/magic64.S

# Object files
OBJS = $(SRCS:.c=.o) magic64.o

# Executable files
EXEC = hungrymain snakemain numbersmain

# Build the executables
all: $(EXEC)

# Rule for building hungry snake demo
hungrymain: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(DEMOS_PATH)/hungrymain.c $^ $(LIBS) $(LDFLAGS)

# Rule for building wandering snake demo
snakemain: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(DEMOS_PATH)/snakemain.c $^ $(LIBS) $(LDFLAGS)

# Rule for building indented numbers demo
numbersmain: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(DEMOS_PATH)/numbersmain.c $^ $(LIBS) $(LDFLAGS)

# Rule to build object files from source
magic64.o: ~pn-cs453/Given/Asgn2/src/magic64.S
	as -o $@ $^

# Clean up object files and executables
clean:
	rm -f $(OBJS) $(EXEC)
