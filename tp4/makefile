CC=gcc
CC_OPTIONS=-m32 
#-Iinclude -Llib -lhardware

all: tp4

tp4: tp4.o hw.o
	${CC} -o tp4 hw.o tp4.o ${CC_OPTIONS}

tp4.o: tp4.c
	gcc -o tp4.o -c tp4.c ${CC_OPTIONS}

hw.o: hw.c
	gcc -o hw.o -c hw.c ${CC_OPTIONS}

clean:
	rm -rf *.o *.dSYM

realclean: clean
	rm -rf tp4
