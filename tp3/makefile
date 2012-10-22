CC=gcc
CC_OPTIONS=-m32 -g
#-Iinclude -Llib -lhardware

all: tp3

tp3: tp3.o hw.o
	${CC} -o tp3 hw.o tp3.o ${CC_OPTIONS}

tp3.o: tp3.c
	gcc -o tp3.o -c tp3.c ${CC_OPTIONS}

hw.o: hw.c
	gcc -o hw.o -c hw.c ${CC_OPTIONS}

clean:
	rm -rf *.o *.dSYM

realclean: clean
	rm -rf tp3
