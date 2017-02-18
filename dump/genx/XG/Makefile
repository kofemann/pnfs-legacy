# $Id: Makefile,v 1.2 2004/03/10 22:19:49 dvd Exp $

CC=cc
CFLAGS=-g -O -I/usr/local/include -I..
LD=cc
LDFLAGS=-g -L/usr/local/lib -L..
C=xg.c lib.c gig.c
O=lib.o
LIB=-lexpat -lgenx

.SUFFIXES: .c .o .a

.c.o:
	$(CC) $(CFLAGS) -c $<

all: xg
	
xg: xg.o $(O)
	$(LD) $(LDFLAGS) -o xg xg.o $(O) $(LIB)
	
clean:
	-rm xg *.o

depend: $(C)
	makedepend -Y $(C)
# DO NOT DELETE

xg.o: xg.h
lib.o: xg.h
gig.o: xg.h
