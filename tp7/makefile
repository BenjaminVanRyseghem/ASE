CC=gcc
CFLAGS= -Iinclude -g
LDFLAGS= -Llib -lhardware
all: vm

vm: vm.o tp7.o dump.o
	${CC} -o vm vm.o dump.o tp7.o ${LDFLAGS}

tp7.o: tp7.c
	${CC} -o tp7.o -c tp7.c ${CFLAGS}

dump.o: dump.c
	${CC} -o dump.o -c dump.c ${CFLAGS}

vm.o: vm.c
	${CC} -o vm.o -c vm.c ${CFLAGS}

clean:
	rm -rf *.o *.dSYM

realclean: clean
	rm -rf vm
