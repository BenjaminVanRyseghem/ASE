CC=gcc
CFLAGS= -Iinclude -g
LDFLAGS= -Llib -lhardware
all: tp6

tp6: tp6.o dump.o
	${CC} -o tp6 dump.o tp6.o ${LDFLAGS}

tp6.o: tp6.c
	${CC} -o tp6.o -c tp6.c ${CFLAGS}

dump.o: dump.c
	${CC} -o dump.o -c dump.c ${CFLAGS}

clean:
	rm -rf *.o *.dSYM

realclean: clean
	rm -rf tp6
