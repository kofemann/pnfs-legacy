
include ../version

prefix=/usr/local/pnfs

all: sclient dbserver

../dbfs/libMd2.a: 
	cd ../dbfs ; make ; cd ..

../shmcom/shmcomlow.o:
	cd ../shmcom ; make ; cd ..


MFLAGS=$(VCFLAGS) -D$(MTYPE)
MINCL=-I../shmcom -I../dbfs
MDLIBS=../shmcom/shmcomlow.o  ../dbfs/libMd2.a ../fs/libFs.a
MLIB=$(MDLIBS) $(GDBASE) $(LEXLIB) $(NET) $(PQLIB)

sclient: sclient.c sclib.o $(MDLIBS)
	$(CC) $(MFLAGS) $(MINCL) sclient.c sclib.o $(MLIB)\
            -o sclient 

sclib.o: sclib.c sclib.h
	$(CC) $(MFLAGS) -c sclib.c $(MINCL)

allowed.o: allowed.c allowed.h
	$(CC) $(MFLAGS) -c allowed.c $(MINCL)

dbserver: dbserver.c $(MDLIBS) allowed.o
	$(CC) $(MFLAGS) $(MINCL) dbserver.c  allowed.o $(MLIB) \
            -o dbserver 

clean :
	rm -f *.o dbserver sclient	


install:
	cp dbserver sclient ../tools/$(OBJECTS)

deposit:
	@mkdir -p $(prefix)/tools/$(OBJECTS)/
	install -m 0744 dbserver $(prefix)/tools/$(OBJECTS)/
	install -m 0744 sclient  $(prefix)/tools/$(OBJECTS)/
	@if [ -x $(prefix)/tools/dbserver ] ; then  rm $(prefix)/tools/dbserver ; fi
	@ln -s $(OBJECTS)/dbserver $(prefix)/tools/dbserver
	@if [ -x $(prefix)/tools/sclient ] ; then  rm $(prefix)/tools/sclient ; fi
	@ln -s $(OBJECTS)/sclient $(prefix)/tools/sclient
