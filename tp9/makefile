CC=gcc
CFLAGS= -Iinclude -g
LDFLAGS= -Llib -lhardware
all: mmuhandler

mmuhandler: mmuhandler.o swap.o
	${CC} -o mmuhandler mmuhandler.o swap.o ${LDFLAGS}
	
mmuhandler.o: mmuhandler.c
	${CC} -o mmuhandler.o -c mmuhandler.c ${CFLAGS}

swap.o: swap.c
	${CC} -o swap.o -c swap.c ${CFLAGS}


clean:
	rm -rf *.o *.dSYM

realclean: clean
	rm -rf mmuhandler