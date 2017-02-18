all:  JShmClient libpnfs.so ShmCom.class ShmCommand.class

include ../../version

MFLAGS=$(VCFLAGS)
FORSO=../../fs/db_gdbm.o ../../nfs/fh.o \
      ../../dbserver/sclib.o ../../dbserver/allowed.o \
      ../../shmcom/shmcomlow.o /usr/lib/libgdbm.so ShmComImpl.o JmdBinding.o
JAVA=/usr/jdk1.1.7/usr/java
GETJAVA=-I$(P_JAVAINCL) -I$(P_JAVAINCL_1)
PNFSINCL=-I../../dbfs -I../../dbserver -I../../shmcom

libpnfs.so: $(FORSO)
	ld -o libpnfs.so -G -L../../dbfs -lmd2 $(FORSO)

jpnfs_shmcom_ShmCom.h: ShmCom.class
	javah -jni jpnfs.shmcom.ShmCom
        
ShmCom.class: ShmCom.java
	javac ShmCom.java
ShmException.class: ShmException.java
	javac ShmException.java
ShmCommand.class: ShmCommand.java
	javac ShmCommand.java
             
ShmComImpl.o: jpnfs_shmcom_ShmCom.h ShmComImpl.c JmdBinding.h 
	$(CC) -c $(VCFLAGS) $(GETJAVA) $(PNFSINCL) ShmComImpl.c

JavaMain.o: JavaMain.c
	$(CC) -c $(GETJAVA) JavaMain.c

JShmClient: JShmClient.c
	$(CC) -o JShmClient JShmClient.c $(GETJAVA) -L$(P_JAVALIB) -ljava


JmdBinding.o: JmdBinding.h JmdBinding.c 
	$(CC) -c $(VCFLAGS) $(GETJAVA) $(PNFSINCL) JmdBinding.c

clean:
	rm -f *.class *.so jpnfs_shmcom_ShmCom.h *.o JShmClient

