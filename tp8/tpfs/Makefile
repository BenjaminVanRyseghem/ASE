# $Id: Makefile,v 1.2 2004/10/12 09:06:17 marquet Exp $
##############################################################################

ROOTDIR=/home/enseign/ASE

CC	= gcc
CFLAGS	= -Wall -ansi -pedantic 
CFLAGS  += -g
LIBDIR  = $(ROOTDIR)/lib
INCDIR  = -I$(ROOTDIR)/include
LIBS    = -L$(LIBDIR) -lhardware

###------------------------------
### Main targets 
###------------------------------------------------------------
BINARIES= mkhd 
OBJECTS	= $(addsuffix .o,\
	  mkhd)

all: $(BINARIES) $(OBJECTS)


###------------------------------
### Main rules 
###------------------------------------------------------------
mkhd.o: mkhd.c
	$(CC) $(CFLAGS) -c mkhd.c $(INCDIR)

mkhd: mkhd.o
	$(CC) $(CFLAGS) -o mkhd mkhd.o $(LIBS)

###------------------------------
### Misc.
###------------------------------------------------------------
.PHONY: clean realclean depend
clean:
	$(RM) *.o $(BINARIES)
realclean: clean 
	$(RM) vdiskA.bin vdiskB.bin
