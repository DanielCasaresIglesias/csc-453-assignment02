# Compiler settings
# The _POSIX_* symbols only come into play on systems that are POSIX
# but not SUS.
SUS3=-D_POSIX_SOURCE -D_POSIX_C_SOURCE=200112L -D_XOPEN_SOURCE=600 -D_DARWIN_C_SOURCE
HARDEN=-D_FORTIFY_SOURCE
CFLAGS=-Wall -g -std=c89 -pedantic $(SUS3) $(HARDEN) -m32
LDFLAGS=-LDFLAGS=-L$HOME/ncurses/lib -lncurses

ALL=liblwp.so

all:	$(ALL)

liblwp.so: lwp.o magic64.o
	$(CC) $(LDFLAGS) -shared -o $@ $^ -lncurses

lwp.o: lwp.c lwp.h
	$(CC) $(CFLAGS) -c $<

numbers: numbersmain.o liblwp.a
	$(CC) $(LDFLAGS) -o $@ $^

numbers-test: numbersmain.o libPLN.a
	$(CC) $(LDFLAGS) -o $@ $^

numbersmain.o: numbersmain.c
	$(CC) $(CFLAGS) -c $<

snakes: snakemain.o liblwp.a libsnakes.a
	$(CC) $(LDFLAGS) -lncurses -o $@ $^

snakemain.o: snakemain.c snakes.h
	$(CC) $(CFLAGS) -c $<

simpletest: simpletest.o liblwp.a
	$(CC) $(LDFLAGS) -o $@ $^

simpletest.o: simpletest.c
	$(CC) $(CFLAGS) -c $<

longlines:
	~pnico/bin/longlines.pl *.c *.h

clean:
	rm -rf core* *.o *.gch liblwp.a $(ALL)
