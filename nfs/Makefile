all: pnfsd pmountd
#
include ../version
#
VSRN=dbfs
MD2=../dbfs/libMd2.a
#
DBSERV=../shmcom/shmcomlow.o ../dbserver/sclib.o
#
MD2LIBS=$(MD2) $(DBSERV) ../fs/libFs.a ../dbfs/dbgprint.o

LIB=$(MD2LIBS)  $(GDBASE) $(LEXLIB) $(RPCSOCLIB) $(PQLIB)
#
INSDIR=-I.  -I../dbfs -I../dbserver -I../shmcom -I../fs

CFLAGS= $(VCFLAGS) -D$(MTYPE) ${INSDIR}
LDFLAGS= -O

NFSOBJS= nfs_prot_svc.o nfs_prot_xdr.o pnfsd.o init.o fh.o
MNTOBJS= mount_svc.o mount_xdr.o pmountd.o fh.o

../fs/db_gdbm.o:
	cd ../fs ; make ; cd ..

../shmcom/shmcomlow.o:
	cd ../shmcom ; make ; cd ..

../dbserver/sclib.o:
	cd ../dbserver  ; make ; cd ..

../dbfs/md2liblib.a:
	cd ../dbfs ; make ; cd ..

all: pmountd pnfsd

pnfsd: ${NFSOBJS} $(MD2LIBS)
	${CC} $(LDFLAGS) -o pnfsd ${NFSOBJS} ${LIB} 

pmountd: ${MNTOBJS}  $(MD2LIBS)
	${CC} $(LDFLAGS)  -o pmountd ${MNTOBJS} ${LIB} 

clean:
	rm -rf *.o pnfsd pmountd getattr mygetwd 

install:
	cp pmountd pnfsd ../tools/$(OBJECTS)
	
deposit:
	@mkdir -p $(prefix)/tools/$(OBJECTS)/
	install -m 0744 pmountd $(prefix)/tools/$(OBJECTS)/
	install -m 0744 pnfsd $(prefix)/tools/$(OBJECTS)/
	@if [ -x $(prefix)/tools/pmountd ] ; then  rm $(prefix)/tools/pmountd ; fi
	@ln -s $(OBJECTS)/pmountd $(prefix)/tools/pmountd
	@if [ -x $(prefix)/tools/pnfsd ] ; then  rm $(prefix)/tools/pnfsd ; fi
	@ln -s $(OBJECTS)/pnfsd $(prefix)/tools/pnfsd

# include dependencies
nfs_prot_svc.o nfs_prot_xdr.o pnfsd.o fh.o init.o: nfs_prot.h

mount_svc.o mount_xdr.o pmountd.o: mount.h

fh.o pnfsd.o pmountd.o: fh.h

pnfsd.o  init.o: pnfsd.h
