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
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>

#include "md2log.h"
#include "md2types.h"
#include "md2scan.h"
/*
#define fixed_size_records
*/
#define record_suppression
/*
 * -------------------------------------------------------------------------------
 *            the basic open functions
 * -------------------------------------------------------------------------------
 */
int  md2Init( char * (*getenv)(const char *name) ){
   mdxInit( getenv , md2pPrintf ) ;
   return 0;
}
int   md2Create( char *dbName , int dbID )
{
MDX_FILE *db ;
mdRecord mdr ;
mdlDatum key , val ;
int rc ;

   db = mdxCreate( dbName , O_TRUNC | O_RDWR | O_CREAT  , 0600 );
   if( ! db )return MDEdbError ;
   md2PrepareRoot( &mdr ,  dbID ) ;
 
  time( (time_t*)&( mdr.head.mTime ) ) ;
  key.dptr  = (char *)&( mdr.head.ID ) ;
  key.dsize = sizeof( mdr.head.ID ) ;
  val.dptr  = (char *)&mdr ;
  val.dsize = sizeof( mdr ) ;
  rc = mdxStore( db , key , val ) ;
   
   mdxClose( db ) ;
   
   return rc ;
}
/*
int   md2CreateDB( char *dbName  )
{
MDX_FILE *db ;

   db = mdxCreate( dbName , O_TRUNC | O_RDWR | O_CREAT  , 0600 );
   if( ! db )return errno ;
   
   mdxClose( db ) ;
   
   return 0 ;
}
*/
MDL * md2OpenReadWrite( char *dbName , int *rcode, int rdel )
{
    MDL      *mdl ;
    mdRecord  mdr ;
    int       rc ;
    
    
    if( ( mdl = (MDL *)malloc(sizeof(MDL)) ) == NULL )return NULL ;
    memset( (char*)mdl ,  0 , sizeof( *mdl ) ) ;
    strcpy( mdl -> dbName , dbName ) ;
    sprintf( mdl -> dbBackupName , "trash" , dbName ) ;
    
    mdl -> backupMode = BM_NORMAL ;
    mdl -> accessMode = mdl_RDWR ;
    mdl -> id.inc     = 2 ;
    
    if( rc = mdxCheck( mdl -> dbName ) )goto problem ;
    if( ( mdl -> db = mdxOpen( mdl -> dbName , MDX_RDWR , 0700 ) ) == NULL ){
        rc = MDEdbError ;
        goto problem ;
    }
    /* File deletion registration is required */
    if (rdel) {
        mdl->dbBackup = mdxOpen(mdl->dbBackupName, MDX_RDWR, 0700); 
    } 
    md2InitLock( mdl ) ;

    mdSetRootID(mdr.head.ID);
    if(md2WriteLock( mdl ) ){ rc = MDEnoLock ; goto problem ; }
        if( rc = md2GetRecord( mdl , &mdr , 0 ) )goto abort ;
        md2GetIdRange( mdl , 0 );
    if(md2CommitLock( mdl )){ rc = MDEnoCommit ; goto problem ; }
    
    if( rcode )*rcode = 0 ;
    return mdl ;
    
abort :
        md2AbortLock( mdl ) ;
problem :
        free( (char *)mdl ) ;
        if( rcode )*rcode = rc ;
        return NULL ;
   
}
MDL * md2OpenReadOnly( char *dbName , int *rcode )
{
    MDL      *mdl ;
    mdRecord  mdr ;
    int       rc ;
    
    
    if( ( mdl = (MDL *)malloc(sizeof(MDL)) ) == NULL )return NULL ;
    memset( (char*)mdl ,  0 , sizeof( *mdl ) ) ;
    strcpy( mdl -> dbName , dbName ) ;

    mdl -> backupMode = BM_NORMAL ;
    
    mdl -> accessMode = mdl_RDONLY ;
    mdl -> id.inc     = 2 ;
    
    if( rc = mdxCheck( mdl -> dbName ) )goto problem ;
    if( ( mdl -> db = mdxOpen( mdl -> dbName , MDX_RDONLY , 0700 ) ) == NULL ){
        rc = MDEdbError ;
        goto problem ;
    }
    md2InitLock( mdl ) ;
    
    if( rcode )*rcode = 0 ;
    return mdl ;
    
abort :
        md2AbortLock( mdl ) ;
problem :
        free( (char *)mdl ) ;
        if( rcode )*rcode = rc ;
        return NULL ;
   
}
int md2Close( MDL *mdl )
{
    if( mdl ){
       if( mdl -> db != NULL )mdxClose( mdl -> db ) ;
       if( mdl -> dbBackup != NULL )mdxClose( mdl -> dbBackup ) ;
       md2CloseLock( mdl ) ;
       free( (char *)mdl ) ;
    }
    return 0 ;
}
void md2PrepareRoot( mdRecord *mdr , int db )
{

    
    mdClearRecord(mdr) ;
    mdSetRootID(  mdr -> head.ID ) ;
    mdr -> body.root.DB = db ;
    mdSetFirstID( mdr -> body.root.nextFreeID , db ) ;
    
    mdClearType( mdr -> head.type ) ;
    mdAddType( mdr -> head.type ,  mdtRoot ) ;
    
    time((time_t*) &( mdr -> head.cTime ) ) ;
    time((time_t*) &( mdr -> head.mTime ) ) ;
    return ;
}
int md2GetIdRange( MDL * mdl , int range )
{
   mdRecord mdr ;
   int rc ;
   
   if( ( range <= 0 ) || ( range > 1000 ) ){
      mdl -> id.count = 0 ;
      return -1 ;
   }
   mdSetRootID( mdr.head.ID ) ;
   if( rc = md2WriteLock( mdl ) )return MDEnoLock ;
   
       if( rc = md2GetRecord( mdl , &mdr , 0 ) )return rc ;
   
       mdl -> id.next = mdr.body.root.nextFreeID ;
       mdl -> id.count = range ;
       mdAddID( mdr.body.root.nextFreeID , 8*range) ;
 
       if( rc = md2PutRecord( mdl , &mdr , 0 ) ){
          mdl -> id.count = 0 ;
          goto abort ;
       }
       
   if( rc = md2CommitLock( mdl ) )return MDEnoCommit ;
   return 0 ;
abort :
   md2AbortLock( mdl ) ;
   return -1 ;
}

 /*
 * -------------------------------------------------------------------------------
 *            the basic io functions
 * -------------------------------------------------------------------------------
 */
int md2Flush( MDL *mdl , int flags  )
{
   if( mdl -> db )mdxFlush( mdl -> db , flags ? MDX_FORCE : 0  ) ; 
   if( mdl -> dbBackup )mdxFlush( mdl -> dbBackup , 0 ) ;
   return 0 ;
} 
int md2Ioctl( MDL *mdl , char * level , int argc , char * argv [] ,
              int * replyLen , char * reply )
{
   return mdxIoctl( mdl -> db , argc , argv , replyLen , reply ) ; 
} 
int md2DeleteRecord( MDL *mdl , md_id_t id , int cache)
{
  mdlDatum key ;
  
     key.dptr  = (char *)&id ;
     key.dsize = sizeof( id ) ;
     return mdxDelete( mdl -> db , key  ) ;
}
#ifdef md_scan_records
int md2GetFirstKey( MDL * mdl , md_id_t *id )
{
  mdlDatum key ;
  
  key = mdxFirst( mdl->db ) ;
  if( key.dptr == NULL ){
     memset((char*)id , 0 , sizeof(md_id_t));
     return -1 ;
  }
  if( key.dsize == sizeof(md_id_t) ){
     memcpy( (char*)id , key.dptr , sizeof(md_id_t) ) ;
     if( key.flags & MDX_FREE )free( (char *)key.dptr ) ;
     return 0 ;
  }else{
     md_id_t k ;
     memcpy( (char*)&k , key.dptr , sizeof(md_id_t) );
     if( key.flags & MDX_FREE )free( (char *)key.dptr ) ;
     return md2GetNextKey(mdl,k,id);
  }
}
int md2GetNextKey( MDL * mdl , md_id_t this , md_id_t *id )
{
  mdlDatum key , thisKey  ;
  memset( (char*)&thisKey , 0 , sizeof(thisKey) ) ; 
  thisKey.dptr  = (char *)&this ;
  thisKey.dsize = sizeof( this ) ;
  
  /*
   * we need to have a loop here to skip 
   * low level private keys.
   */
  while( 1 ){
     key = mdxNext( mdl->db , thisKey ) ;
     if( thisKey.flags & MDX_FREE )free( (char *)thisKey.dptr ) ;
     if( key.dptr == NULL ){
        memset((char*)id , 0 , sizeof(md_id_t));
        return -1;
     }
     if( key.dsize == sizeof(md_id_t) )break ;
     memcpy((char*)&thisKey , (char*)&key , sizeof(key)) ;
  }
  memcpy( (char*)id , key.dptr , sizeof(md_id_t) ) ;
  if( key.flags & MDX_FREE )free( (char *)key.dptr ) ;
  return 0 ;
}
#endif
#ifdef fixed_size_records
int md2PutRecord( MDL *mdl , mdRecord *mdr , int cache)
{
 mdlDatum key , val ;
 
  time( (time_t*)&( mdr -> head.mTime ) ) ;
  key.dptr  = (char *)&( mdr -> head.ID ) ;
  key.dsize = sizeof( mdr -> head.ID ) ;
  val.dptr  = (char *)mdr ;
  val.dsize = sizeof( *mdr ) ;
  return mdxStore( mdl -> db , key , val ) ;
}
int md2GetRecord( MDL *mdl , mdRecord *mdr , int cache) 
{
 mdlDatum key , val ;
 md_id_t id ;
 int rc ;
 
 if( rc = md2ReadLock( mdl ) )return MDEnoLock ;
 
 id =  mdr -> head.ID  ;
 key.dptr  = (char *)&( mdr -> head.ID ) ;
 key.dsize = sizeof( mdr -> head.ID ) ;
 
 rc = mdxFetch( mdl -> db , key, &val ) ;
 if( rc ){
   fprintf(stderr," mdxFetch %s failed %d\n",mdStringID(id),errno);
   rc = MDEdbError;
   goto commit;
 }
 if( ! val.dptr ){
    fprintf(stderr," mdxFetch %s failed %d\n",mdStringID(id),errno);
    rc = MDEnotFound ;
    goto commit ;
 }
 if( val.dsize != sizeof( *mdr ) ){
    fprintf(stderr," mdxFetch %8.8X-%8.8X wrong size %d : db=%x;size=%d\n" ,
            mdStringID( mdr -> head.ID) ,val.dsize,mdl->db,key.dsize);
     rc =  -2 ;
     goto commit ;
 }
 memcpy( (char *)mdr , val.dptr , val.dsize ) ;
 if( val.flags & MDX_FREE )free( (char *)val.dptr ) ;

commit :
 
 if( md2CommitLock( mdl ) )return MDEnoCommit ;

 return rc ;
}
#else
int md2SuppressZero( char *p , int size );

int md2PutRecord( MDL *mdl , mdRecord *mdr , int cache)
{
 mdlDatum key , val ;
 int size ;
 
  time( (time_t*)&( mdr -> head.mTime ) ) ;
  key.dptr  = (char *)&( mdr -> head.ID ) ;
  key.dsize = sizeof( mdr -> head.ID ) ;
  
  if( mdIsType( mdr  -> head.type , mdtRoot ) ){  
         size = MAX_BODY_SIZE ;      
  }else if( mdIsType( mdr -> head.type , mdtDirectory ) ){
      if( mdIsType( mdr -> head.type , mdtInode ) ){
         size = sizeof( md_dir_inode ) ;    
      }else if( mdIsType( mdr -> head.type , mdtData ) ){
         size = sizeof( md_dir_data ) ;    
      }else if( mdIsType( mdr -> head.type , mdtHash ) ){
         size = sizeof( md_dir_hash ) ;    
         size = md2SuppressZero( (char *)&mdr->body , size ) ;  
      }else{
         size = MAX_BODY_SIZE ;      
      }
  }else if( mdIsType( mdr -> head.type , mdtRegular ) ){
      if( mdIsType( mdr -> head.type , mdtInode ) ){
         size = sizeof( md_file_inode ) ; 
      }else if( mdIsType( mdr -> head.type , mdtData ) ){
         size = sizeof( md_file_data ) ;    
         size = md2SuppressZero( (char *)&mdr->body , size ) ;  
         size = md_max( size , 512 ) ;  
      }else if( mdIsType( mdr -> head.type , mdtHash ) ){
         size = sizeof( md_file_hash ) ;    
      }else{
         size = MAX_BODY_SIZE ;      
      }
  }else if( mdIsType( mdr -> head.type , mdtLink ) ){
      if( mdIsType( mdr -> head.type , mdtInode ) ){
         size = sizeof( md_file_inode ) ;    
      }else if( mdIsType( mdr -> head.type , mdtData ) ){
         size = sizeof( md_link_data ) ;    
      }else{
         size = MAX_BODY_SIZE ;      
      }
  }else if( mdIsType( mdr -> head.type , mdtTag ) ){
     size = sizeof( md_tag_inode ) ;    
  }else{
     size = MAX_BODY_SIZE ;      
  }
  
  val.dptr  = (char *)mdr ;
  val.dsize = size + sizeof( md_head ) ;
  
     return mdxStore( mdl -> db , key , val ) ;
}
int md2GetRecord( MDL *mdl , mdRecord *mdr , int cache) 
{
 mdlDatum key , val ;
 md_id_t id ;
 int rc ;
 char *p ;
 
 if( rc = md2ReadLock( mdl ) )return MDEnoLock ;
 
 id =  mdr -> head.ID  ;
 key.dptr  = (char *)&( mdr -> head.ID ) ;
 key.dsize = sizeof( mdr -> head.ID ) ;
 
    rc = mdxFetch( mdl -> db , key, &val ) ;

 if( rc ){
   fprintf(stderr," mdxFetch %s failed %d\n",mdStringID(id),errno);
   rc = MDEdbError;
   goto commit;
 }
 if( ! val.dptr ){
    fprintf(stderr," mdxFetch %s failed %d\n",mdStringID(id),errno);
    rc = MDEnotFound ;
    goto commit ;
 }
 if( val.dsize > sizeof( *mdr ) ){
    fprintf(stderr," mdxFetch %8.8X-%8.8X wrong size %d : db=%x;size=%d\n" ,
            mdStringID( mdr -> head.ID) ,val.dsize,mdl->db,key.dsize);
     rc =  -2 ;
     goto commit ;
 }
 memcpy( (char *)mdr , val.dptr , val.dsize ) ;
 if( val.flags & MDX_FREE )free( (char *)val.dptr ) ;
 
 if( val.dsize < sizeof( *mdr ) ){
    memset(  ((char*)mdr)+val.dsize , 0 , sizeof(*mdr)-val.dsize ) ;
 }

commit :
 
 if( md2CommitLock( mdl ) )return MDEnoCommit ;

 return rc ;
}
int md2SuppressZero( char *p , int size )
{
#ifdef record_suppression
 long *l ; 
  
  if( size % 4 )return size ;
  
  for( size = size / 4 - 1 , l = ( long *)p ; 
       ( size >= 0 ) && ( l[size] == 0 ) ; size-- ) ;

  return ( size + 1 ) * 4 ;
#else
  return size ;
#endif
}
#endif
