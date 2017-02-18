prefix=/usr/local/pnfs
all: shmcom

include ../version

MFLAGS=$(VCFLAGS)


shmcom: shmcom.c shmcom.h shmcomlow.o
	$(CC) $(MFLAGS) -D$(MTYPE)  -o shmcom shmcom.c shmcomlow.o \
              ../dbfs/libMd2.a $(NET)

shmcomlow.o: shmcomlow.c shmcom.h
	$(CC) -I../dbfs $(MFLAGS) -D$(MTYPE) -c shmcomlow.c 

clean:
	rm -f *.o shmcom

install:
	cp shmcom ../tools/$(OBJECTS)

deposit:
	@mkdir -p $(prefix)/tools/$(OBJECTS)/
	install shmcom $(prefix)/tools/$(OBJECTS)/
	@if [ -x $(prefix)/tools/shmcom ] ; then  rm $(prefix)/tools/shmcom ; fi
	@ln -s $(OBJECTS)/shmcom $(prefix)/tools/shmcom
