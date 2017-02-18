/*
 * Copyrighted as an unpublished work.
 * (c) Copyright 1996,1997,1998 DESY Hamburg DMG-Division
 * All rights reserved.
 *
 *
 * RESTRICTED RIGHTS
 *
 * These programs are supplied under a license.  They may be used,
 * disclosed, and/or copied only as permitted under such license
 * agreement.  Any copy must contain the above copyright notice and
 * this restricted rights notice.  Use, copying, and/or disclosure
 * of the programs is strictly prohibited unless otherwise provided
 * in the license agreement.
 */
#ifndef __DIFFEX__H__
#define __DIFFEX__H__

#include <time.h>
#ifndef PROBLEM
#define PROBLEM(i)  {e=(i);goto BAD;}
#endif
typedef struct _diffEx_ {
    
    int  (*read)( struct _diffEx_ * diffEx , char * data , int * len ) ;
    int  (*write)( struct _diffEx_ * diffEx , char * data , int len ) ;
    void (*free)( struct _diffEx_ * diffEx ) ;
    void (*setMaxWriteCount)( struct _diffEx_ * diffEx , int writeCount );
    int  (*getMaxWriteCount)( struct _diffEx_ * diffEx );
    void (*setMaxWriteTime)( struct _diffEx_ * diffEx , int maxSeconds );
    int  (*getMaxWriteTime)( struct _diffEx_ * diffEx );
    int  (*isEnabled)( struct _diffEx_ * diffEx );
    int  (*sync)( struct _diffEx_ * diffEx , int flags );
    void (*setKeepOpen)( struct _diffEx_ * diffEx , int keepOpen );
    
    int flags ;
    int maxWriteCount ;
    int maxWriteTime ;
    int handle ;
    char * dbName ;
    char * backupDir ;
    char * tmpFileName ;
    char * fileName ;
    time_t creationTime ;
    int    writeCount ;
    int    openMode ;
    int    enabled ;
    int    writing ;
} DiffEx ;

DiffEx * newDiffEx( char * dbName , char * backupDir , int mode ,int * error ) ;
static void  DE_free( DiffEx * mdx ) ;
static int   DE_write( DiffEx * diffEx , char *data , int len ) ;
static int   DE_read( DiffEx * diffEx , char * data , int * len ) ;
static void  DE_setMaxWriteCount( DiffEx * diffEx , int writeCount ) ;
static int   DE_getMaxWriteCount( DiffEx * diffEx ) ;
static void  DE_setMaxWriteTime( DiffEx * diffEx ,  int maxSeconds ) ;
static int   DE_getMaxWriteTime( DiffEx * diffEx ) ;
static int   DE_isEnabled( DiffEx * diffEx ) ;
static int   DE_sync( DiffEx * diffEx , int flags ) ;
static void  DE_setKeepOpen( DiffEx * diffEx , int keepOpen ) ;
static int   DE__createFile( DiffEx * diffEx , int mode ) ;
static int   DE__openFile( DiffEx * diffEx ) ;
static int   DE__closeFile( DiffEx * diffEx ) ;
static void  DE__createNewPath( DiffEx * diffEx ) ;
static void  DE__freeMemory( DiffEx * de ) ;
static int   DE__openFileRead( DiffEx * de ) ;
#define DEX_KEEP_OPEN  (0x1)
#define DEX_READ       (0x1000) 
#define DEX_FORCE_FLUSH  (0x1)
#define DEX_CLOSE        (0x2)
#endif
