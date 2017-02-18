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
#ifndef __DBGLUE__H__
#define __DBGLUE__H__

#define MDX_FREE   1
#define MDX_RDONLY    (1)
#define MDX_RDWR      (2)
#define MDX_FORCE     (0x1)
#define MDX_CREATE    (0x40) 
#define MDX_OPEN      (0x80)
typedef struct {
     char *dptr;
     int dsize;
     int flags ;
} mdlDatum;

typedef void *MDX_FILE ;

mdlDatum mdxFirst( MDX_FILE mdx );
mdlDatum mdxNext( MDX_FILE mdx , mdlDatum key );

int      mdxInit( char * (*getenv)(const char * ) , int (*prt)(int , char *,...)) ;
MDX_FILE mdxOpen( char *name , int flags ,int mode ) ;
MDX_FILE mdxCreate( char *name , int flags ,int mode ) ;
int mdxIoctl( MDX_FILE mdxf , int argc , char * argv [] , int * replyLen , char *reply );
int mdxCheck( char *name ) ;
int mdxFetch( MDX_FILE mdx , mdlDatum key , mdlDatum *val ) ;
int mdxStore( MDX_FILE mdx , mdlDatum key , mdlDatum val ) ;
int mdxClose( MDX_FILE mdx ) ;
int mdxFlush( MDX_FILE mdx , int flags ) ;
int mdxDelete( MDX_FILE mdx , mdlDatum key  ) ;
int mdxReadLock( MDX_FILE mdx ) ;
int mdxWriteLock( MDX_FILE mdx ) ;
int mdxCommitLock( MDX_FILE mdx ) ;
int mdxAbortLock( MDX_FILE mdx ) ;

#endif
