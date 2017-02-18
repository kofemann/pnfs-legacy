all: pnfs3d pmount3d
#
include ../version
#
VSRN=dbfs
MD2=../dbfs/libMd2.a
#
DBSERV=../shmcom/shmcomlow.o ../dbserver/sclib.o
#
MD2LIBS=$(MD2) $(DBSERV) ../fs/libFs.a ../dbfs/dbgprint.o

LIB=$(MD2LIBS)  $(GDBASE) $(LEXLIB) $(RPCSOCLIB) 
#
INSDIR=-I.  -I../dbfs -I../dbserver -I../shmcom -I../fs

CFLAGS= $(VCFLAGS) -D$(MTYPE) ${INSDIR} #-DDEBUG
LDFLAGS= -O

NFSOBJS= nfs3_prot_svc.o nfs3_prot_xdr.o pnfs3d.o init3.o fh3.o
MNTOBJS= mount3_svc.o mount3_xdr.o pmount3d.o fh3.o

mount3_prot_svc.c: mount3_prot.x
	rpcgen -h mount3_prot.x -o mount3_prot.h
	rpcgen -m mount3_prot.x -o mount3_prot_svc.c
	rpcgen -c mount3_prot.x -o mount3_prot_xdr.c


nfs3_prot_svc.c: nfs3_prot.x
	rpcgen -h nfs3_prot.x -o nfs3_prot.h
	rpcgen -m nfs3_prot.x -o nfs3_prot_svc.c
	rpcgen -c nfs3_prot.x -o nfs3_prot_xdr.c

fh3.o: fh3.h fh3.c
	$(CC) $(CFLAGS) fh3.c $(INSDIR) -c

pmount3d: mount3.o  mount3_prot_svc.o mount3_prot_xdr.o fh3.o mount3_main.o init3.o $(MD2LIBS)
	$(CC) $(LDFLAGS) mount3.o -o pmount3d \
		mount3_main.o mount3_prot_svc.o \
		mount3_prot_xdr.o fh3.o init3.o $(LIB)

pnfs3d: nfs3.o  nfs3_prot_svc.o nfs3_prot_xdr.o fh3.o nfs3_main.o init3.o $(MD2LIBS)
	$(CC)  $(LDFLAGS) nfs3.o -o pnfs3d nfs3_main.o \
		nfs3_prot_svc.o nfs3_prot_xdr.o \
		fh3.o init3.o $(LIB)


../fs/db_gdbm.o:
	cd ../fs ; $(MAKE) ; cd ..

../shmcom/shmcomlow.o:
	cd ../shmcom ; $(MAKE) ; cd ..

../dbserver/sclib.o:
	cd ../dbserver  ; $(MAKE) ; cd ..

../dbfs/md2liblib.a:
	cd ../dbfs ; $(MAKE) ; cd ..

clean:
	rm -f *.o pnfs3d pmount3d
	rm -f  mount3_prot.h mount3_prot_svc.c mount3_prot_xdr.c
	rm -f  nfs3_prot.h nfs3_prot_svc.c nfs3_prot_xdr.c

install:
	cp pmount3d pnfs3d ../tools/$(OBJECTS)


nfs3.o: nfs3.c nfs3_prot_svc.c mount3_prot_svc.c fh3.h
mount3.o: nfs3_prot_svc.c mount3.c mount3_prot_svc.c fh3.h

nfs3_prot_svc.o: nfs3_prot_svc.c
nfs3_prot_xdr.o: nfs3_prot_xdr.c
mount3_prot_svc.o: mount3_prot_svc.c
mount3_prot_xdr.o: mount3_prot_xdr.c
fh3.o: fh3.c fh3.h
