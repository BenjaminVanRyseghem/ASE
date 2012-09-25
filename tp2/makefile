all: tp2

tp2: tp2.o
	gcc -o tp2 tp2.o -m32

tp2.o: tp2.c
	gcc -o tp2.o -c tp2.c -m32

clean:
	rm -rf *.o *.dSYM

realclean: clean
	rm -rf tp1
