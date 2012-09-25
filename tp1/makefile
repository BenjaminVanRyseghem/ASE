all: tp1

tp1: tp1.o
	gcc -o tp1 tp1.o -m32

tp1.o: tp1.c
	gcc -o tp1.o -c tp1.c -m32

clean:
	rm -rf *.o

realclean: clean
	rm -rf tp1