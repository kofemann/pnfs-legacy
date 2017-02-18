#include  <stdio.h>
#include  <stdlib.h>
#include  <unistd.h>
#include  <string.h>
#include  <ctype.h>
#include <fcntl.h>
#include  <gdbm.h>



typedef struct {
	char* key;
	char* val;
	int keyLen;
	int valLen;
} gdbmRecord;




int inject( GDBM_FILE db, gdbmRecord * record ) 
{

	datum k,v;

	k.dptr = record->key;
	k.dsize = record->keyLen;
	
	v.dptr = record->val;
	v.dsize = record->valLen;


	return gdbm_store( db , k , v , GDBM_REPLACE ) ;
}

int readRecord( int fd, gdbmRecord* record )
{

   int n;
   int reclen;

   n = read(fd, &reclen, sizeof(int) ); 

   if( n <= 0 ) return -1;


   n = read(fd, &record->keyLen, sizeof(int) ); 
    if( n <= 0 ) return -1;

   n = read(fd, record->key, record->keyLen);
    if( n <= 0 ) return -1;

   n = read(fd, &record->valLen, sizeof(int) ); 
    if( n <= 0 ) return -1;

   n = read(fd, record->val, record->valLen);
    if( n <= 0 ) return -1;

   return 0;
}

void main(int argc, char *argv[])
{


    int fd;
    int reclen;
    int n;
    gdbmRecord record;
    
    GDBM_FILE new_db;

    if( argc != 3 ) {
         fprintf(stderr, "Usage: %s <dump> <db>", argv[0]);
         exit(1);
    }


    fd = open(argv[1], O_RDONLY);
    if( fd < 0 ) {
         perror("open binary dump");
         exit(2);
    } 

    new_db = gdbm_open(argv[2], 8*1024, GDBM_NEWDB, 0644 , NULL);
    if( new_db == NULL ) {
        perror(" open new db");
	close(fd);
        exit(3);
    }


    record.key = malloc(1024);
    record.val = malloc(4096);

    while( readRecord(fd,  &record ) == 0 ) {

	inject(new_db, &record);
         
    }


   close(fd);
   gdbm_close(new_db);
}
