CC=gcc
CFLAGS= -Iinclude -g -m32
LDFLAGS= -Llib -lhardware -m32
all: mi_kernel


mi_kernel: mi_user.o mi_kernel.o mmuhandler.o swap.o
	${CC} -o mi_kernel mi_kernel.o mi_user.o mmuhandler.o swap.o ${LDFLAGS}
	
mi_kernel.o: mi_kernel.c
	${CC} -o mi_kernel.o -c mi_kernel.c ${CFLAGS}

mi_user.o: mi_user.c
	${CC} -o mi_user.o -c mi_user.c ${CFLAGS}

mmuhandler.o: mmuhandler.c
	${CC} -o mmuhandler.o -c mmuhandler.c ${CFLAGS}

swap.o: swap.c
	${CC} -o swap.o -c swap.c ${CFLAGS}


clean:
	rm -rf *.o *.dSYM

realclean: clean
	rm -rf mmuhandler