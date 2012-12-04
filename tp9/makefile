CC=gcc
CFLAGS= -Iinclude -g
LDFLAGS= -Llib -lhardware
all: mmuhandler

mmuhandler.o: mmuhandler.c
	${CC} -o mmuhandler.o -c mmuhandler.c ${CFLAGS}

clean:
	rm -rf *.o *.dSYM

realclean: clean
	rm -rf mmuhandler