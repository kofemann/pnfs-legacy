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
 * -------------------------------------------------------------------------------
 *            the high level directory scanning functions
 * -------------------------------------------------------------------------------
 *     3         2         1         0       
 *    10987654321098765432109876543210
 *    ppppppppppppppppppppphhhhhhhhhhe
 */
#define mdFromCookie(c,e,h,p)  {(e)=!((c)&1);\
                                (h)=(((c)>>1)&0x3FF);\
                                (p)=((c)>>11);}
#define mdToCookie(c,e,h,p)   {(c)=((e)?0:1)|\
                                   (((h)&0x3FF)<<1) |\
                                   (((p)&0x1FFFFF)<<11);}
                                   
int  md2DirOpen( MDL * mdl , md_id_t id , md_cookie *cky )
{
           long   proloque , position , hash ;
  md_dir_cookie   cookie ;
            int   rc ;
  
  memset( (char*)&cookie , 0 , sizeof( md_dir_cookie ) ) ;
  mdFromCookie( *cky , proloque , hash, position ) ;
  if( proloque ){
     cookie.position = 0 ;
     cookie.hash     = 0 ;
     if( rc = md2_dir_open( mdl , id , &cookie ) )return rc ;
  }else{
     cookie.position = position ;
     cookie.hash     = hash ;
     if( rc = md2_dir_open( mdl , id , &cookie ) )return rc ;
  }
  return 0 ;
}                           
int  md2DirNext( MDL * mdl , md_cookie *cky , md_dir_item *item )
{
           long   proloque , position , hash ;
  md_dir_cookie   cookie ;
            int   rc ;
        md_id_t   root_id ;
  rc = 0 ;
  memset( (char*)&cookie , 0 , sizeof( md_dir_cookie ) ) ;
  mdFromCookie( *cky , proloque , hash, position ) ;
  if( proloque ){
     memset( (char*)item , 0 , sizeof( *item ) ) ;
     if( position == 0 ){
       mdSetRootID(root_id) ;
       if( mdIsEqualID( root_id , mdl -> dir.dir.head.parentID ) ){
         strcpy( item -> name , ".." ) ;
         item -> ID = mdl -> dir.dir.head.ID ;
       }else{
         strcpy( item -> name , ".." ) ;
         item -> ID = mdl -> dir.dir.head.parentID ;
       }
       position++ ;
     }else if( position == 1 ){
       strcpy( item -> name , "." ) ;
       item -> ID = mdl -> dir.dir.head.ID ;
       proloque = 0 ;
       position = 0 ;
     }
     hash = 0 ;
  }else{
     cookie.position = position ;
     cookie.hash     = hash ;
     rc = md2_dir_next( mdl , item , &cookie )  ;
     position =   cookie.position  ;
     hash     =   cookie.hash     ;
  }
  mdToCookie( *cky , proloque , hash, position) ;
  return rc ;
}                           
int  md2DirClose( MDL * mdl )
{
  return md2_dir_close( mdl ) ;

}                           
/*
 * -------------------------------------------------------------------------------
 *            the basic directory scanning functions
 * -------------------------------------------------------------------------------
 */
int    md2_dir_open( MDL *mdl , md_id_t id , md_dir_cookie *cookie ) 
{
  int  rc , i , j , k , m ;
  md_id_t   blockID ;
  
   if( mdl -> dir.valid )md2_dir_close( mdl ) ;
   if( rc = md2ReadLock( mdl ) )return MDEnoLock ;
   mdl -> dir.valid   = 1 ;
   mdl -> dir.eof     = 0 ;
   
   mdl->dir.dir.head.ID = id ;
   if( rc = md2GetRecord( mdl , &mdl->dir.dir , 0 ) )goto abort ;
   if( ( ! mdIsType( mdl->dir.dir.head.type , mdtDirectory ) ) || 
       ( ! mdIsType( mdl->dir.dir.head.type , mdtInode     ) )    )
       { rc = MDEnotDirectory ; goto abort ; }
       
   mdl->dir.current.hash      =  cookie -> hash ;
   mdl->dir.current.position  =  cookie -> position ;
   mdl->dir.current.hashBlock =  mdl -> dir.current.hash /
                                 mdl -> dir.dir.body.dirInode.hashInfo.entriesPerRow ;
   if( mdl->dir.current.hashBlock >= mdl->dir.dir.body.dirInode.hashInfo.rows ){
      mdl->dir.eof = 1 ;
      return 0 ;
   }
   for( i = mdl->dir.current.hashBlock ;
        i < mdl->dir.dir.body.dirInode.hashInfo.rows ; i++ ){
        
        mdl->dir.hash.head.ID = mdl->dir.dir.body.dirInode.hashHandle[i] ;
        if( mdIsNullID( mdl->dir.hash.head.ID ) ){ rc = -1 ; goto abort ; }
    
        if( rc = md2GetRecord( mdl , &mdl->dir.hash , 0 ) )goto abort ;
        mdl->dir.current.hashRel =  mdl -> dir.current.hash %
                                    mdl -> dir.dir.body.dirInode.hashInfo.entriesPerRow ;
        if( mdl->dir.current.hashRel >= mdl->dir.hash.body.dirHash.hashHead.entries ){
           mdl->dir.eof = 1 ;
           return 0 ;
        }
        for( j = mdl->dir.current.hashRel ; 
             j < mdl->dir.hash.body.dirHash.hashHead.entries ; j++ ){
             
            blockID = mdl->dir.hash.body.dirHash.hashPointer[j];
            if( mdIsNullID( blockID ) ){
               mdl->dir.current.position = 0 ;
               continue ;
            }
            mdl->dir.current.posBlock = mdl->dir.current.position / DIR_ITEMS ;
            for( k = 0 ; 
                 ! mdIsNullID( blockID ) ; 
                 k++ , blockID = mdl->dir.block.head.nextID        ){
                 
               mdl->dir.block.head.ID = blockID ;
               if( rc = md2GetRecord( mdl , &mdl->dir.block , 0 ) )goto abort ;
               if( mdl->dir.current.posBlock <= k ){
                   for( m = (mdl->dir.current.position % DIR_ITEMS) ;
                        m < DIR_ITEMS ; m++                         ){
                      
                        if( ! mdIsNullID( mdl->dir.block.body.dirData.dirItem[m].ID ) )
                             goto found ;
                        mdl->dir.current.position = 0 ;
                   }
               }
               
            }
        }
   }
   mdl->dir.eof = 1 ;
   return 0 ;

found :   
   mdl->dir.current.hashBlock = i ; 
   mdl->dir.current.hashRel   = j ;
   mdl->dir.current.posBlock  = k ;
   mdl->dir.current.posRel    = m ;
   mdl->dir.current.hash      = mdl->dir.current.hashBlock *
                                mdl -> dir.dir.body.dirInode.hashInfo.entriesPerRow +
                                mdl->dir.current.hashRel ;
   mdl->dir.current.position  = mdl->dir.current.posBlock * DIR_ITEMS +
                                mdl->dir.current.posRel ;
   cookie -> hash     = mdl->dir.current.hash ;
   cookie -> position = mdl->dir.current.position ;
                           
   return 0 ;
   
abort :
   md2_dir_close( mdl ) ;
   return rc ;
}
int    md2_dir_next( MDL *mdl , md_dir_item *item , md_dir_cookie *cookie ) 
{
  int      rc , *iPtr , *jPtr , *kPtr , *mPtr , hold , found ;
  md_id_t  blockID ;
  
   if( ! mdl -> dir.valid )return -1 ;
   if( mdl->dir.eof )return 1 ;

   iPtr = &mdl->dir.current.hashBlock ;
   jPtr = &mdl->dir.current.hashRel ;
   kPtr = &mdl->dir.current.posBlock  ;
   mPtr = &mdl->dir.current.posRel  ;
   hold  = 1 ;
   found = 0 ;
   for( ; *iPtr < mdl->dir.dir.body.dirInode.hashInfo.rows ; (*iPtr)++ ){
        /*printf( " hashBlock : %d\n" , *iPtr ) ;*/
        if( ! hold ){
           mdl->dir.hash.head.ID = mdl->dir.dir.body.dirInode.hashHandle[*iPtr] ;
           if( mdIsNullID( mdl->dir.hash.head.ID ) ){ rc = -1 ; goto abort ; }    
           if( rc = md2GetRecord( mdl , &mdl->dir.hash , 0 ) )goto abort ;
        }                           
        if(!hold)(*jPtr)=0;
        for( ; *jPtr < mdl->dir.hash.body.dirHash.hashHead.entries ; (*jPtr)++ ){
            /*printf( " hashRel : %d\n" , *jPtr ) ;*/
            
            blockID = mdl->dir.hash.body.dirHash.hashPointer[*jPtr];
            if( mdIsNullID( blockID ) )continue ; 
        
            if(!hold)(*kPtr)=0;
            for( ; ! mdIsNullID( blockID ) ;  (*kPtr)++ , blockID = mdl->dir.block.head.nextID){
               /* printf( " posBlock : %d\n" , *kPtr ) ;*/
               if( ! hold ){ 
                   mdl->dir.block.head.ID = blockID ;
                   if( rc = md2GetRecord( mdl , &mdl->dir.block , 0 ) )goto abort ;
               }
               if(!hold)(*mPtr)=0;
               for(  ; (*mPtr) < DIR_ITEMS ; (*mPtr)++ ){
                    /*printf( " posRel  : %d\n" , *mPtr ) ;*/
                    hold = 0 ;
                    if( ! mdIsNullID( mdl->dir.block.body.dirData.dirItem[*mPtr].ID ) ){
                        if(found)goto ok ;
                        *item = mdl->dir.block.body.dirData.dirItem[*mPtr]  ;
                        found = 1 ;
                    }
               }
               
            }
        }
   }
   mdl->dir.eof = 1 ;

ok:
   mdl->dir.current.hash      = mdl->dir.current.hashBlock *
                                mdl -> dir.dir.body.dirInode.hashInfo.entriesPerRow +
                                mdl->dir.current.hashRel ;
   mdl->dir.current.position  = mdl->dir.current.posBlock * DIR_ITEMS +
                                mdl->dir.current.posRel ;
   cookie -> hash     = mdl->dir.current.hash ;
   cookie -> position = mdl->dir.current.position ;
   return found ? 0 : 1  ;
abort :
   (void)md2_dir_close( mdl ) ;
   return rc ;
}
int    md2_dir_close( MDL *mdl ) 
{
   if( mdl -> dir.valid ){
   if(md2CommitLock( mdl ))return MDEnoCommit ;
     mdl -> dir.valid = 0 ;
   }
   return 0 ;

}
