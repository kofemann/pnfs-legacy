#ifndef __DUMP_WRAP_H__
#define __DUMP_WRAP_H__

#include <time.h>     // NEEDED FOR md2ptypes.h
#include <stdio.h>    // NEEDED FOR md2types.h

#include "md2types.h" // NEEDED FOR sclib.h, also includes md2ptypes.h
#include "shmcom.h"   // NEEDED FOR sclib.h
#include "sdef.h"     // NEEDED FOR sclib.h

#include "sclib.h"

/* The default delay, in milliseconds */
#define MYGET_DEFAULT_DELAY 100


void myGet_register_delay( long delay);
void myGet_register_max_retries( int new_max_retries);

int myGetRootId( SCL *scl , int db , md_id_t *id );
int myReadDirAuth( SCL *scl ,md_auth *auth , md_id_t id ,mdPermission perm ,
		   long cookie , int count , reqExtItem *extItem);
int myGetRecord( SCL *scl , md_id_t id ,  mdRecord *rec);
int myReadData( SCL *scl, md_id_t id, mdPermission perm, long offset,
		long size, char *data);

void myTimeElapsed( struct timeval *overhead_elapsed_p,
		    struct timeval *db_elapsed_p);

#endif
