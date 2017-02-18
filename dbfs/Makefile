prefix=/usr/local/pnfs

all: libMd2.a md2tool md3tool md3shrink md3checkdb md3diff

include ../version
#
MFLAGS=-D$(MTYPE) $(VCFLAGS)
TOOLS=../tools
MOBJ=md2lib.o md2dirs.o md2io.o md2fs.o md2tags.o md2scan.o \
     md2actions.o md2log.o md2sprint.o dbgprint.o
FSOBJ=../fs/libFs.a
FS=$(FSOBJ) $(GDBASE) 
BLIBS=$(FS) $(LEXLIB) $(NET) $(PQLIB)

$(FSOBJ): 
	cd ../fs ; make ; cd ..

.c.o:
	$(CC) $(MFLAGS) -c $<


md2tool: md2tool.o libMd2.a $(FSOBJ)
	$(CC) $(MFLAGS) -o md2tool md2tool.o libMd2.a $(BLIBS)

md3shrink: md3shrink.o libMd2.a $(FSOBJ)
	$(CC) $(MFLAGS) -o md3shrink md3shrink.o libMd2.a $(BLIBS)

md3tool: md3tool.o libMd2.a $(FSOBJ)
	$(CC) $(MFLAGS) -o md3tool md3tool.o libMd2.a $(BLIBS)

md3checkdb: md3checkdb.o libMd2.a $(FSOBJ)
	$(CC) $(MFLAGS) -o md3checkdb md3checkdb.o libMd2.a $(BLIBS)

md3diff: md3diff.o libMd2.a $(FSOBJ)
	$(CC) $(MFLAGS) -o md3diff md3diff.o libMd2.a $(BLIBS)

worldlog: worldlog.o libMd2.a $(FSOBJ)
	$(CC) $(MFLAGS) -o worldlog worldlog.o libMd2.a $(BLIBS)

libMd2.a: $(MOBJ)
	rm -f libMd2.a  ; \
        $(AR) cr libMd2.a $(MOBJ) ; \
        $(RANLIB) libMd2.a

libmd2.so: $(MOBJ)
	ld -o  libmd2.so -shared $(MOBJ) 

lex.yy.c: md2scan.l md2scan.h
	$(LEX) md2scan.l

md2scan.o: lex.yy.c md2scan.h
	$(CC) -c $(MFLAGS) lex.yy.c -o md2scan.o

clean:
	rm -f *.o md2tool md3tool md3shrink lex.yy.c *.a *.so

install:
	cp md2tool md3tool ../tools/$(OBJECTS)

deposit:
	mkdir -p $(prefix)/tools/$(OBJECTS)/
	install -m 0744 md2tool $(prefix)/tools/$(OBJECTS)/
	install -m 0744 md3tool $(prefix)/tools/$(OBJECTS)/
	@if [ -x $(prefix)/tools/md2tool ] ; then  rm $(prefix)/tools/md2tool ; fi
	@ln -s $(OBJECTS)/md2tool $(prefix)/tools/md2tool
	@if [ -x $(prefix)/tools/md3tool ] ; then  rm $(prefix)/tools/md3tool ; fi
	@ln -s $(OBJECTS)/md3tool $(prefix)/tools/md3tool
	
$(MOBJ) : md2ptypes.h md2types.h dbglue.h


md2tool md2fs.o md2actions.o : md2fs.h

md2lib.o md2log.o md2sprint.o : md2log.h
 
md2tags.o md2sprint.o md2lib.o : md2scan.h
