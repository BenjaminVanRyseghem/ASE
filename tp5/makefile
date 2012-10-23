CC=gcc-4.2
CFLAGS= -Iinclude -g
LDFLAGS= -Llib -lhardware
all: tp5

tp5: tp5.o dump.o
	${CC} -o tp5 dump.o tp5.o ${LDFLAGS}

tp5.o: tp5.c
	${CC} -o tp5.o -c tp5.c ${CFLAGS}

dump.o: dump.c
	${CC} -o dump.o -c dump.c ${CFLAGS}

clean:
	rm -rf *.o *.dSYM

realclean: clean
	rm -rf tp5
