
TARGETS = pnfsDump $(VALIDATION_OK)

#  Different serialisers: the file dump_ser_<name>.c must exist and
#  the file dump_output.c must be adjusted accordingly.
#
#  The list of serialisers may be adjusted by explicitly setting this
#  variable:
#
#      rm dump_output.o
#      make SERIALISERS="null xml"
#
#  By default we enable all serialisers.
SERIALISERS = simple null xml chimera syncat files verify

#  Derived information from serialisers
#
#  The list of source code files
SERIALISER_SRC = $(SERIALISERS:%=dump_ser_%.c)

#  Inform registration which serialisers we want to build
SERIALISER_CPP = $(SERIALISERS:%=-DENABLE_SERIALISER_%)


#  Validation programs.
#
#  These should be built and run.  Their return-code is 0 if
#  everything is OK.
#
VALIDATION_TARGETS = chk_lvl2_parse
VALIDATION_SRC = chk_lvl2_parse.c parse_lvl2.c
#
#  Derived values
VALIDATION_OBJ = $(VALIDATION_SRC:%.c=%.o)
VALIDATION_OK = $(VALIDATION_TARGETS:%=%.ok)


PNFS_DUMP_SRC = dump_intree.c dump_main.c dump_utils.c dump_output.c dump_tag.c dump_dummy.c dump_wrap.c dump_cache.c dump_ser_abs_file.c parse_lvl2.c md5.c $(SERIALISER_SRC) dump_compat.c dump_di_readdir.c dump_di_dbscan.c

#  External code: we pull these in explicitly (rather than as libraries)
#  to avoid pulling in the whole world!
PNFS_DUMP_EXTRA_OBJ = ../shmcom/shmcomlow.o ../dbserver/sclib.o ../dbfs/dbgprint.o


#  Include support for GenX: a small XML library.
GENX_DIR=genx
GENX_LIB_NAME=genx


#  Derived values:

ALL_GEN_OBJ = $(PNFS_DUMP_OBJ) $(VALIDATION_OBJ)

PNFS_DUMP_OBJ = $(PNFS_DUMP_SRC:%.c=%.o)

GENX_LIB_FILE=$(GENX_DIR)/$(GENX_LIB_NAME:%=lib%.a)

#
include ../version
#

CPPFLAGS = -I.  -I../dbfs -I../dbserver -I../shmcom -I$(GENX_DIR) $(SERIALISER_CPP)

CFLAGS = $(VCFLAGS) -D$(MTYPE) -DFREOPEN64=$(FREOPEN64)

all: $(TARGETS)

install: $(TARGETS)
	cp $(TARGETS) ../tools

pnfsDump: $(PNFS_DUMP_OBJ) $(PNFS_DUMP_EXTRA_OBJ) $(GENX_LIB_FILE)
	$(CC) $(LDFLAGS) -o $@ $(PNFS_DUMP_OBJ) $(LDLIBS) $(PNFS_DUMP_EXTRA_OBJ) -L$(GENX_DIR) -l$(GENX_LIB_NAME)

chk_lvl2_parse: $(VALIDATION_OBJ)
	$(CC) $(LDFLAGS) -o $@ $^

%.ok: %
	./$< && touch $@

#  We assume a built-in implicit rule like this:
#  %.o: %c
#  	$(CC) -c $< $(CPPFLAGS) $(CFLAGS)

$(GENX_LIB_FILE):
	(cd $(GENX_DIR) ; $(MAKE))


#
#  These create the external objects if they don't already exist:
#  NB. their dependencies are missing.

../shmcom/shmcomlow.o:
	cd ../shmcom ; $(MAKE) ; cd ..

../dbserver/sclib.o:
	cd ../dbserver  ; $(MAKE) ; cd ..

../dbfs/dbgprint.o:
	cd ../dbfs  ; $(MAKE) ; cd ..


#  Clean all temporary files
#  (yes, this is the wrong name for this target, but we can't use "clean")
cleantemp:
	rm -f *~ \#*\#

#  Clean all generated files: should return us to pristine condition.
#  (yes, this is the wrong name for this target, but must use "clean")
clean: cleantemp
	(cd $(GENX_DIR) ; $(MAKE) clean)
	rm -f $(TARGETS) $(VALIDATION_TARGETS) $(ALL_GEN_OBJ)


#  distclean is the correct name for what the "clean" target does.
distclean: clean
