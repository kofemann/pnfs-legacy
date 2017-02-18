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
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <gdbm.h>
#include "dbglue.h"

int  mdxInit( char * (*getenv)( const char * name ), int (*prt)(int , char *,...) ){
   return 0 ;
}
mdlDatum mdxFirst( MDX_FILE mdx )
{
   datum v ;
   mdlDatum val ;
   
   v = gdbm_firstkey( (GDBM_FILE) mdx ) ;
   
   val.dsize = v.dsize ;
   val.dptr  = v.dptr ;
   if( v.dptr )val.flags = MDX_FREE ;
   
   return val ;
}
mdlDatum mdxNext( MDX_FILE mdx , mdlDatum key )
{
   datum k , v ;
   mdlDatum val ;
   
   k.dptr  = key.dptr ;
   k.dsize = key.dsize ;
   
   v = gdbm_nextkey( (GDBM_FILE) mdx , k ) ;
   
   val.dsize = v.dsize ;
   val.dptr  = v.dptr ;
   if( v.dptr )val.flags = MDX_FREE ;
   
   return val ;
}
int mdxScan( MDX_FILE mdx )
{
int rc , i;
unsigned char tmp[8] ;
datum key , nextkey ;

        key = gdbm_firstkey ( mdx );
        while ( key.dptr ) {
           nextkey = gdbm_nextkey ( mdx, key );
           memcpy( tmp , key.dptr , 8 ) ;
           for(i=0;i<8;i++)fprintf(stderr,"%2.2X",tmp[i]);
           fprintf(stderr,"\n");
           key = nextkey;
        };
  
  return 0 ;

}
MDX_FILE mdxOpen( char *name , int flags ,int mode )
{

 int flag ;
 GDBM_FILE x ;
 
 flag = 0 ;
 if( flags & MDX_RDONLY )flag |= GDBM_READER ;
 if( flags & MDX_RDWR )flag |= GDBM_WRITER ;
 flag |= GDBM_FAST ;
 
 x = gdbm_open( name , 8*1024 , flag , mode , NULL ) ;
 
 return (MDX_FILE) x ;
 
}
MDX_FILE mdxCreate( char *name , int flags ,int mode )
{
 int flag ;
 GDBM_FILE x ;
 
 flag = 0 ;
 flag |= GDBM_WRCREAT ;
 
 x = gdbm_open( name , 8*1024 , flag , mode , NULL ) ;
 
 return (MDX_FILE) x ;

}
int mdxCheck( char *name )
{
 GDBM_FILE x ;
 
 
 x = gdbm_open( name , 8*1024 , GDBM_READER , 0 , NULL ) ;
 if( x )gdbm_close( x ) ;
 return x ? 0 : -1 ;

}
int mdxFetch( MDX_FILE mdx , mdlDatum key, mdlDatum *val )
{
   datum k , v ;
   
   k.dptr  = key.dptr ;
   k.dsize = key.dsize ;
   
   v = gdbm_fetch( (GDBM_FILE) mdx , k ) ;
   
   val->dsize = v.dsize ;
   val->dptr  = v.dptr ;
   val->flags = ( v.dptr ? MDX_FREE : 0) ;
   
   return 0 ;
}
int mdxStore( MDX_FILE mdx , mdlDatum key , mdlDatum val ) 
{
 datum k , v ;
   
 k.dsize = key.dsize ;
 k.dptr  = key.dptr ;
 v.dsize = val.dsize ;
 v.dptr  = val.dptr ;
 
 return gdbm_store( (GDBM_FILE) mdx , k , v , GDBM_REPLACE ) ;

}
int mdxIoctl( MDX_FILE mdxf , int argc , char * argv [] , int * replyLen , char *reply ){
   return -1 ;
}
int mdxDelete( MDX_FILE mdx , mdlDatum key  )
{
 datum k  ;
   
 k.dsize = key.dsize ;
 k.dptr  = key.dptr ;
 return gdbm_delete( (GDBM_FILE) mdx , k ) ;

}
int mdxClose( MDX_FILE mdx )
{
   gdbm_close( (GDBM_FILE) mdx ) ;
   return 0 ;
}
int mdxFlush( MDX_FILE mdx , int force ) 
{
   gdbm_sync( (GDBM_FILE) mdx );
   return 0 ;
}
int mdxReadLock( MDX_FILE mdx )
{
   return 0 ;
} 
int mdxWriteLock( MDX_FILE mdx )
{
   return 0 ;
} 
int mdxCommitLock( MDX_FILE mdx )
{
   return 0 ;
} 
int mdxAbortLock( MDX_FILE mdx )
{
   return 0 ;
} 
