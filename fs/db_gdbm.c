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
#include <string.h>
#include <gdbm.h>
#include "DiffEx.h"
#include "dbglue.h"

typedef struct _mdxbh_ {

    GDBM_FILE   regular ;
    char      * regularName ;
    char      * databaseName ;
    DiffEx    * journal ;

} MDX ;

static int _mdxOpenJournal( MDX * mdx , int argc , char * argv [] ) ;
static int _mdxCloseJournal( MDX * mdx , int argc , char * argv [] ) ;
static int _mdxSetJournal( MDX * mdx , int argc , char * argv [] ) ;

#define MDX_BACKUP  (0x1)
#define MDX_RECOVER (0x2)


MDX * newMDX( char * name ){
   int i = 0 , n = 0 ;
   MDX * mdx = NULL ;
   mdx = (MDX *)malloc(sizeof(MDX));
   if( mdx == NULL )return NULL ;
   mdx->regular     = NULL ;
   mdx->regularName = strdup(name);
   mdx->journal     = NULL ;
   /*
    * fiddle around with the directories and names
    */
   n = strlen( name ) ;
   for( i = n - 1 ; ( i >=0 ) && ( name[i] != '/' ) ; i-- ) ;
   if( (i+1) == n ){ free(mdx) ; return NULL ; }

   mdx -> databaseName = strdup( name + i + 1 ) ;
    
   return mdx ;
}
MDX_FILE mdxCreate( char *name , int flags , int mode )
{
 int flag ;
 MDX * mdx = newMDX(name) ; 
 
 if( mdx == NULL )return NULL ;
 flag = 0 ;
 flag |= GDBM_WRCREAT ;
 
 mdx -> regular = gdbm_open( name , 8*1024 , flag , mode , NULL ) ;
 
 return (MDX_FILE) mdx ;

}
MDX_FILE mdxOpen( char *name , int flags ,int mode )
{

 int flag ;
 MDX * mdx = newMDX(name) ; 
 if( mdx == NULL )return NULL ;
 
 flag = 0 ;
 if( flags & MDX_RDONLY )flag |= GDBM_READER ;
 if( flags & MDX_RDWR )flag |= GDBM_WRITER ;
 flag |= GDBM_FAST ;
 
 mdx -> regular = gdbm_open( name , 8*1024 , flag , mode , NULL ) ;
 
 return (MDX_FILE) mdx ;
 
}
int mdxIoctl( MDX_FILE mdxf , char * func , int argc , char * argv [] ){
   MDX * mdx = (MDX *)mdxf ;
   if( func == NULL )return -110 ;
   if( ! strcmp( func , "open-journal" ) ){
      if( argv == NULL )return -112 ;
      return _mdxOpenJournal( mdx , argc , argv ) ;    
   }else if( ! strcmp( func , "set-journal" ) ){
      if( argv == NULL )return -112 ;
      return _mdxSetJournal( mdx , argc , argv ) ;    
   }else if( ! strcmp( func , "close-journal" ) ){
      if( argv == NULL )return -112 ;
      return _mdxCloseJournal( mdx , argc , argv ) ;    
   }else{
      return -111 ;
   }
}
static int _mdxOpenJournal( MDX * mdx , int argc , char * argv [] ){

   int mode   = 0600 ;
   int result = 0 ;
   if( argc < 1 )return -112 ;
   
   if( mdx -> journal != NULL )
      ( mdx -> journal ) -> free( mdx -> journal ) ;

   mdx -> journal = NULL ;

   if( argc > 1 )sscanf( argv[1] , "%d" , &mode ) ;

   mdx -> journal = newDiffEx( mdx->databaseName , argv[0] , mode , &result ) ;
   
   return mdx -> journal == NULL ? result : 0 ;
}
static int _mdxCloseJournal( MDX * mdx , int argc , char * argv [] ){

   
   if( mdx -> journal == NULL )return -120 ;
   mdx -> journal -> free( mdx -> journal ) ;

   mdx -> journal = NULL ;
   
   return 0 ;
}
static int _mdxSetJournal( MDX * mdx , int argc , char * argv [] ){
   int result = 0 ;
   if( argc < 2 )return -112 ;
   
   if( mdx -> journal == NULL )return -120 ;
   if( ! strcmp( argv[0] , "count" ) ){
      sscanf( argv[1] , "%d" , &result ) ;
      mdx -> journal -> setMaxWriteCount( mdx -> journal , result ) ;
   }else if( ! strcmp( argv[0] , "time" ) ){
      sscanf( argv[1] , "%d" , &result ) ;
      mdx -> journal -> setMaxWriteTime( mdx -> journal , result ) ;
   }else{
      return -114 ;
   }
   
   return 0 ;
}
static int _mdxMergeJournal( MDX * mdx , int argc , char * argv [] ){
   int result = 0 ;
   if( argc < 1 )return -112 ;
   
   if( mdx -> journal == NULL )return -120 ;
   if( ! strcmp( argv[0] , "count" ) ){
      sscanf( argv[1] , "%d" , &result ) ;
      mdx -> journal -> setMaxWriteCount( mdx -> journal , result ) ;
   }else if( ! strcmp( argv[0] , "time" ) ){
      sscanf( argv[1] , "%d" , &result ) ;
      mdx -> journal -> setMaxWriteTime( mdx -> journal , result ) ;
   }else{
      return -114 ;
   }
   
   return 0 ;
}
int mdxCheck( char *name )
{
 GDBM_FILE x ;
 
 
 x = gdbm_open( name , 8*1024 , GDBM_READER , 0 , NULL ) ;
 if( x )gdbm_close( x ) ;
 return x ? 0 : -1 ;

}
mdlDatum mdxFetch( MDX_FILE mdxf , mdlDatum key )
{
   datum k , v ;
   mdlDatum val ;
   MDX * mdx = (MDX*)mdxf ;
   
   k.dptr  = key.dptr ;
   k.dsize = key.dsize ;
   
   v = gdbm_fetch( mdx->regular , k ) ;
   
   val.dsize = v.dsize ;
   val.dptr  = v.dptr ;
   if( v.dptr )val.flags = MDX_FREE ;
   
   return val ;
}
static char __buffer[8*1024] ;
int mdxStore( MDX_FILE mdxf , mdlDatum key , mdlDatum val ) 
{
 datum k , v ;
 MDX * mdx = (MDX*)mdxf ;
   
 k.dsize = key.dsize ;
 k.dptr  = key.dptr ;
 v.dsize = val.dsize ;
 v.dptr  = val.dptr ;
 
 if( mdx -> journal != NULL ){
    int pos = 0 ;
    memcpy( __buffer + pos , (char*)&key.dsize , sizeof( key.dsize ) ) ;
    pos += sizeof( key.dsize ) ;
    memcpy( __buffer + pos , key.dptr , key.dsize ) ;
    pos += key.dsize ;
    memcpy( __buffer + pos , (char*)&val.dsize , sizeof( val.dsize ) ) ;
    pos += sizeof( val.dsize ) ;
    memcpy( __buffer + pos , val.dptr , val.dsize ) ;
    pos += val.dsize ;
    mdx -> journal -> write( mdx -> journal , __buffer , pos ) ;
 }
 return gdbm_store( mdx->regular , k , v , GDBM_REPLACE ) ;

}
int mdxDelete( MDX_FILE mdxf , mdlDatum key  )
{
 datum k  ;
 MDX * mdx = (MDX*)mdxf ;
   
 k.dsize = key.dsize ;
 k.dptr  = key.dptr ;
 if( mdx -> journal != NULL ){
    int pos = 0 ;
    mdlDatum val ;
    memcpy( __buffer + pos , (char*)&key.dsize , sizeof( key.dsize ) ) ;
    pos += sizeof( key.dsize ) ;
    memcpy( __buffer + pos , key.dptr , key.dsize ) ;
    pos += key.dsize ;
    val.dsize = 0 ;
    memcpy( __buffer + pos , (char*)&val.dsize , sizeof( val.dsize ) ) ;
    pos += sizeof( val.dsize ) ;
    mdx -> journal -> write( mdx -> journal , __buffer , pos ) ;
 }
 return gdbm_delete( mdx->regular , k ) ;

}
int mdxClose( MDX_FILE mdxf )
{
   MDX * mdx = (MDX *)mdxf ;
   if( mdx -> journal != NULL ){
      mdx -> journal -> free( mdx -> journal ) ;
      mdx -> journal = NULL ;
   }
   gdbm_close( mdx->regular ) ;
   return 0 ;
}
int mdxFlush( MDX_FILE mdxf ) 
{
   MDX * mdx = (MDX *)mdxf ;
   if( mdx -> journal != NULL ){
      mdx -> journal -> sync( mdx -> journal ) ;
   }
   gdbm_sync( mdx->regular );
   return 0 ;
}
int mdxReadLock( MDX_FILE mdxf )
{
   return 0 ;
} 
int mdxWriteLock( MDX_FILE mdxf )
{
   return 0 ;
} 
int mdxCommitLock( MDX_FILE mdxf )
{
   return 0 ;
} 
int mdxAbortLock( MDX_FILE mdxf )
{
   return 0 ;
} 
mdlDatum mdxFirst( MDX_FILE mdxf )
{
   datum v ;
   mdlDatum val ;
   
   v = gdbm_firstkey( ((MDX *)mdxf)->regular ) ;
   
   val.dsize = v.dsize ;
   val.dptr  = v.dptr ;
   if( v.dptr )val.flags = MDX_FREE ;
   
   return val ;
}
mdlDatum mdxNext( MDX_FILE mdxf , mdlDatum key )
{
   datum k , v ;
   mdlDatum val ;
   
   k.dptr  = key.dptr ;
   k.dsize = key.dsize ;
   
   v = gdbm_nextkey( ((MDX *)mdxf)->regular , k ) ;
   
   val.dsize = v.dsize ;
   val.dptr  = v.dptr ;
   if( v.dptr )val.flags = MDX_FREE ;
   
   return val ;
}
int mdxScan( MDX_FILE mdxf )
{
int rc , i;
unsigned char tmp[8] ;
datum key , nextkey ;

        key = gdbm_firstkey( ((MDX *)mdxf)->regular );
        while ( key.dptr ) {
           nextkey = gdbm_nextkey ( ((MDX *)mdxf)->regular, key );
           memcpy( tmp , key.dptr , 8 ) ;
           for(i=0;i<8;i++)fprintf(stderr,"%2.2X",tmp[i]);
           fprintf(stderr,"\n");
           key = nextkey;
        };
  
  return 0 ;

}
