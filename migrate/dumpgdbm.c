/*
 * $Id: dumpgdbm.c,v 1.6 2005-09-23 07:00:39 tigran Exp $
 */

#include  <stdio.h>
#include  <stdlib.h>
#include  <unistd.h>
#include  <string.h>
#include  <ctype.h>
#include  <gdbm.h>
#include "../dbfs/dbglue.h"


static int dump_gdbm_file(char *name)
{
    GDBM_FILE file;
    datum entry, datas;
    mdlDatum key, val;
    MDX_FILE *conn;
    long long int entryCount = 0;
    int rc = -1;

    file = gdbm_open(name, 8*1024, GDBM_READER, 0444, NULL);

    if (file == NULL) {
		fprintf(stderr, "can't open %s (%s)\n", name, gdbm_strerror ( gdbm_errno ) );	
		return rc;
    }



	conn = mdxCreate(name, 0, 0);


	if( conn == NULL ) {
		    gdbm_close(file);
		    return rc;
	}

    entry = gdbm_firstkey(file);
    if (entry.dptr == NULL) {
	fprintf(stderr, "database %s is empty\n", name);
	return rc;
    }


	/* start transaction */

	if ( mdxWriteLock(conn) != 0 ) {
	    fprintf(stderr, "BEGIN command failed\n");
		mdxClose(conn);		
	}else {

	    do {
			datas = gdbm_fetch(file, entry);
	
			key.dptr = entry.dptr;
			key.dsize = entry.dsize;
			
			val.dptr = datas.dptr;
			val.dsize = datas.dsize;
			
			rc = mdxStore(conn, key, val);
			++entryCount;
	
			free(val.dptr);
			free(entry.dptr);
			
			entry = gdbm_nextkey(file, entry);
	    } while ( (rc ==0) && (entry.dptr != NULL) );

		rc = mdxCommitLock(conn);
		if(	rc != 0 ) {
		    fprintf(stderr, "COMMIT command failed\n");			
		}else{
		    printf("Total: %lld entries.\n", entryCount);
		}

	}
	
    gdbm_close(file);    
    
    return rc;
}


int main(int argc, char *argv[])
{
    return dump_gdbm_file(argv[optind]);
}
