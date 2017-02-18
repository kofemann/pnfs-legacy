CFLAGS=-g -O3
O=genx.o charProps.o 

test:	tgx
	./tgx > /dev/null

tgx:	tgx.o libgenx.a
	cc $(CFLAGS) -o tgx tgx.o libgenx.a

libgenx.a:	$O
	ar rc libgenx.a $O
	ranlib libgenx.a

tarball:	clean
	cd .. && tar --exclude '.*' -c -z -f genx.tgz genx && mv genx.tgz genx 

clean:	
	rm -f genx.tgz *.o *.a tgx *~
	cd docs && make clean

cpp:
	g++ -c -Wall -pedantic genx.c charProps.c
	rm genx.o charProps.o

genx.o:	genx.c genx.h
charProps.o:	charProps.c genx.h

