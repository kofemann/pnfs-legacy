#
# $Id: Makefile,v 1.2 2005-09-22 09:45:02 tigran Exp $
#


include ../version

GDBM_INC =
GDBM_LIB = -lgdbm

C_OPT = -g -pedantic -Wall $(GDBASEINC) $(GDBM_IN)
LD_OPT = $(GDBM_LIB) $(GDBASE) $(FSOBJ)

FSOBJ=../fs/libFs.a

all:		dumpgdbm 

dumpgdbm.o:	dumpgdbm.c Makefile
	$(CC) $(C_OPT) -c dumpgdbm.c

dumpgdbm: $(FSOBJ) dumpgdbm.o Makefile
	$(CC) dumpgdbm.o -o dumpgdbm $(LD_OPT)

$(FSOBJ): ../fs/${db_impl:.o=.c}
	cd ../fs ; make ; cd ..

	

clean:
	@rm -f *.o dumpgdbm core

