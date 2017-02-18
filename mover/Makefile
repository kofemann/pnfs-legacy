all: pDumpster

include ../version

MFLAGS=$(VCFLAGS)


pDumpster: pDumpster.c
	$(CC) $(MFLAGS) -D$(MTYPE) -o pDumpster pDumpster.c

clean:
	rm -f *.o pDumpster

install:
	cp pDumpster ../tools/$(OBJECTS)
