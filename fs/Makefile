#
include ../version
#
all: libFs.a 
#
#
CFLAGS=$(VCFLAGS) $(GDBASEINC)
#
#libFs.a:  db_gdbm-journal-easy.o db_gdbm-easy.o db_gdbm-hash.o DiffEx.o  ${POSTGRES}
libFs.a:  ${db_impl} DiffEx.o
	rm -f libFs.a ; \
        $(AR) cr libFs.a DiffEx.o $(db_impl) ; \
        $(RANLIB) libFs.a

libFs.so: db_gdbm-journal-easy.o db_gdbm-easy.o db_gdbm-hash.o DiffEx.o
	rm -f libFs.so ; ld -o libFs.so -shared DiffEx.o $(db_impl)

db_gdbm-journal-easy.o: db_gdbm-journal-easy.c dbglue.h DiffEx.h
	$(CC) -c ${CFLAGS}  db_gdbm-journal-easy.c

db_gdbm-easy.o: db_gdbm-easy.c dbglue.h
	$(CC) -c ${CFLAGS}  db_gdbm-easy.c

db_dynamic.o: db_dynamic.c dbglue.h
	$(CC) -c ${CFLAGS}  db_dynamic.c

db_gdbm-hash.o: db_gdbm-hash.c dbglue.h
	$(CC) -c ${CFLAGS}  db_gdbm-hash.c

db_oracle-hash.o: db_oracle-hash.c dbglue.h
	$(CC) -c ${CFLAGS}  db_oracle-hash.c

db_psql-hash.o: db_psql-hash.c dbglue.h
	$(CC) -c ${CFLAGS}  db_psql-hash.c

db_psql-easy.o: db_psql-easy.c dbglue.h
	$(CC) -c ${CFLAGS}  db_psql-easy.c

db_psql2-hash.o: db_psql2-hash.c dbglue.h
	$(CC) -c ${CFLAGS}  db_psql2-hash.c

db_psql2-easy.o: db_psql2-easy.c dbglue.h
	$(CC) -c ${CFLAGS}  db_psql2-easy.c

DiffEx.o: DiffEx.c DiffEx.h
	$(CC) -c ${CFLAGS} DiffEx.c

Vfss.o: Vfss.c Vfss.h
	$(CC) -c ${CFLAGS} Vfss.c
clean: 
	rm -f *.o *.a *.so

install:
	echo ""
