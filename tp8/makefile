CC=gcc
CFLAGS= -Iinclude -g
LDFLAGS= -Llib -lhardware
all: vm

vm: vm.o tp8.o dump.o
	${CC} -o vm vm.o dump.o tp8.o ${LDFLAGS}

tp8.o: tp8.c
	${CC} -o tp8.o -c tp8.c ${CFLAGS}

dump.o: dump.c
	${CC} -o dump.o -c dump.c ${CFLAGS}

vm.o: vm.c
	${CC} -o vm.o -c vm.c ${CFLAGS}

clean:
	rm -rf *.o *.dSYM

realclean: clean
	rm -rf vm
