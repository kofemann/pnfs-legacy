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

/*               level:      0    1    2    3    4    5    6    7    */
static int _levelMask[8] = { 0 , -1  , 0  , 0  , 0  , 0 , -1  , 0  } ;

int md2FakeFileUnixAttr( md_unix *attr , md_id_t id , int size );
int md2FindDirectoryEntryByID( MDL * mdl , md_id_t childID , md_dir_item *item );
int md2RegisterDeletetion(MDL * mdl, md_id_t dirid, char * name, md_id_t id);
int md2DeleteChain( MDL * mdl , md_id_t idIn );

int md2SetLevelMask( int level , int value ){

   if( ( level < 0 ) || ( level > 7 ) )return -1 ;
   _levelMask[level] = value ;
   return 0 ;
}
int md2GetRoot( MDL *mdl , int db , md_id_t *fsMountID ) 
{
 mdRecord root ;
 int rc ;
 
 if(md2ReadLock( mdl ) )return MDEnoLock ;
 
     mdSetRootID(root.head.ID);
     if( (rc = md2GetRecord( mdl , &root , 0 )) )goto abort ;
     
 if(md2CommitLock( mdl ))return MDEnoCommit ;
 *fsMountID = root.head.nextID ;
 return 0 ;
  
abort :

 md2AbortLock( mdl ) ;
 return rc ;  
}
int md2GetRootDir( MDL *mdl , md_id_t *fsMountID ) 
{
 mdRecord root ;
 int rc ;
 
 if(md2ReadLock( mdl ) )return MDEnoLock ;
 
     mdSetRootID(root.head.ID);
     if( (rc = md2GetRecord( mdl , &root , 0 )) )goto abort ;
     
 if(md2CommitLock( mdl ))return MDEnoCommit ;
 *fsMountID = root.head.nextID ;
 return 0 ;
  
abort :

 md2AbortLock( mdl ) ;
 return rc ;  
}
int md2GetDbId( MDL * mdl , long *dbID )
{
 int rc ;
 mdRecord root ;
 
 if(md2ReadLock( mdl ) )return MDEnoLock ;
 
    mdSetRootID(root.head.ID) ;
    if( (rc = md2GetRecord( mdl , &root , 0 )) )goto abort ;
    if( dbID )*dbID = root.body.root.DB ;
 
 if(md2CommitLock( mdl ))return MDEnoCommit ;
 return 0 ;
abort :

 md2AbortLock( mdl ) ;
 return rc ;  

}
int md2GetRootStatistics( MDL * mdl , md_statistics *st )
{
 int rc ;
 mdRecord root ;
 
 if(md2ReadLock( mdl ) )return MDEnoLock ;
 
    mdSetRootID(root.head.ID) ;
    if( (rc = md2GetRecord( mdl , &root , 0 )) )goto abort ;
    if( st )*st = root.body.root.statistics ;
 
 if(md2CommitLock( mdl ))return MDEnoCommit ;
 return 0 ;
abort :

 md2AbortLock( mdl ) ;
 return rc ;  

}
int md2SetRootStatistics( MDL * mdl , md_statistics *st )
{
 int rc ;
 mdRecord root ;
 
 if(md2WriteLock( mdl ) )return MDEnoLock ;
 
    mdSetRootID(root.head.ID) ;
    if( (rc = md2GetRecord( mdl , &root , 0 )) )goto abort ;
    if( st )root.body.root.statistics = *st   ;
    if( (rc = md2PutRecord( mdl , &root , 0 )) )goto abort ;
 
 if(md2CommitLock( mdl ))return MDEnoCommit ;
 return 0 ;
abort :

 md2AbortLock( mdl ) ;
 return rc ;  

}
int md2GetRootConfig( MDL * mdl , md_id_t *configID )
{
 int rc ;
 mdRecord root ;
 
 if(md2ReadLock( mdl ) )return MDEnoLock ;
 
    mdSetRootID(root.head.ID) ;
    if( (rc = md2GetRecord( mdl , &root , 0 )) )goto abort ;
    if( configID )*configID = root.body.root.configID ;
 
 if(md2CommitLock( mdl ))return MDEnoCommit ;
 return 0 ;
abort :

 md2AbortLock( mdl ) ;
 return rc ;  

}
int md2SetRootConfig( MDL * mdl , md_id_t *configID )
{
 int rc ;
 mdRecord root ;
 
 if(md2WriteLock( mdl ) )return MDEnoLock ;
 
    mdSetRootID(root.head.ID) ;
    if( (rc = md2GetRecord( mdl , &root , 0 )) )goto abort ;
    if( configID )root.body.root.configID = *configID   ;
    if( (rc = md2PutRecord( mdl , &root , 0 )) )goto abort ;
 
 if(md2CommitLock( mdl ))return MDEnoCommit ;
 return 0 ;
abort :

 md2AbortLock( mdl ) ;
 return rc ;  

}
int md2ChangeParent( MDL * mdl , md_id_t id , md_id_t parent  )
{
 int      rc  ;
 mdRecord inode ;
 
 if(md2ReadLock( mdl ) )return MDEnoLock ;
    inode.head.ID = id ;
    if( (rc = md2GetRecord( mdl , &inode , 0 )) )goto abort ;
    if( ! mdIsType( inode.head.type , mdtInode )  ){
       rc = MDEnotFound ;
       goto abort ;
    }
    inode.head.parentID = parent ;
    if( (rc = md2PutRecord( mdl , &inode , 0 )) )goto abort ;
    
 if(md2CommitLock( mdl ))return MDEnoCommit ;
 return 0 ;
 
abort :

 md2AbortLock( mdl ) ;
 return rc ;  

} 

int md2CountDirectoryEntries( MDL * mdl , md_id_t dirID )
{
 int      rc , i , j , k , count ;
 mdRecord dir , hash , data ;
 md_id_t  id ;
 
 count = 0 ;
 if(md2ReadLock( mdl ) )return MDEnoLock ;
    dir.head.ID = dirID ;
    if( (rc = md2GetRecord( mdl , &dir , 0 )) )goto abort ;
    if( ! ( mdIsType( dir.head.type , mdtDirectory ) &&
            mdIsType( dir.head.type , mdtInode )        ) ){
       rc = MDEnotDirectory ;
       goto abort ;
    }
    for( i = 0 ; i < dir.body.dirInode.hashInfo.rows ; i++ ){
        hash.head.ID = dir.body.dirInode.hashHandle[i] ;
        if( (rc = md2GetRecord( mdl , &hash , 0 )) )goto abort ;  
        for( j = 0 ; j < hash.body.dirHash.hashHead.entries ; j++ ){
           if( mdIsNullID( hash.body.dirHash.hashPointer[j] ) )continue ;
           for( id = hash.body.dirHash.hashPointer[j] ;
                ! mdIsNullID(id) ; 
                id = data.head.nextID  ){
                
                data.head.ID = id ;
                if( (rc = md2GetRecord( mdl , &data , 0 )) )goto abort ;
                for( k = 0 ; k < data.body.dirData.dirHead.entries ; k++ )
                 if( ! mdIsNullID( data.body.dirData.dirItem[k].ID ) )
                   count++ ;
                
           }
        }
    }

 if(md2CommitLock( mdl ))return MDEnoCommit ;
 return count ;
 
abort :

 md2AbortLock( mdl ) ;
 return rc ;  

} 
int md2PrintHashEntries( MDL * mdl , md_id_t dirID , FILE *f )
{
 int      rc , i , j , k , count ,  hashPosition ;
 mdRecord dir , hash , data ;
 md_id_t  id ;
 
 count = 0 ;
 hashPosition = -1 ;
 if(md2ReadLock( mdl ) )return MDEnoLock ;
    dir.head.ID = dirID ;
    if( (rc = md2GetRecord( mdl , &dir , 0 )) )goto abort ;
    if( ! ( mdIsType( dir.head.type , mdtDirectory ) &&
            mdIsType( dir.head.type , mdtInode )        ) ){
       rc = MDEnotDirectory ;
       goto abort ;
    }
    for( i = 0 ; i < dir.body.dirInode.hashInfo.rows ; i++ ){
        hash.head.ID = dir.body.dirInode.hashHandle[i] ;
        if( (rc = md2GetRecord( mdl , &hash , 0 )) )goto abort ;  
        for( j = 0 ; j < hash.body.dirHash.hashHead.entries ; j++ ){
               hashPosition ++ ;
           if( mdIsNullID( hash.body.dirHash.hashPointer[j] ) ){
             fprintf(f," %d %d\n" , hashPosition , 0 ) ;
             continue ;
           }
           count = 0 ;
           for( id = hash.body.dirHash.hashPointer[j] ;
                ! mdIsNullID(id) ; 
                id = data.head.nextID  ){
                
                data.head.ID = id ;
                if( (rc = md2GetRecord( mdl , &data , 0 )) )goto abort ;
                for( k = 0 ; k < data.body.dirData.dirHead.entries ; k++ )
                 if( ! mdIsNullID( data.body.dirData.dirItem[k].ID ) )
                   count++ ;
                
           }
           fprintf(f," %d %d\n" , hashPosition , count ) ;
        }
    }

 if(md2CommitLock( mdl ))return MDEnoCommit ;
 return count ;
 
abort :

 md2AbortLock( mdl ) ;
 return rc ;  

}
int md2MakeNewDirectory( MDL * mdl , md_id_t dir , md_id_t *id )
{
 mdRecord  Mdr , *mdr , Hash , *hash , parentDir ;
 int       rc , rest , i ;
  
 mdr  = &Mdr ;
 hash = &Hash ;
 
 if(md2WriteLock( mdl ) )return MDEnoLock ;
   
   
 if( (rc  =  md2GetNewRecord( mdl , mdr )) )goto abort ;
   
   mdAddType( mdr->head.type , mdtInode ) ;
   mdAddType( mdr->head.type , mdtDirectory ) ;
   
   mdr->head.parentID = dir ;

#ifdef MD_FLAGS_INHERIT   
   parentDir.head.ID = dir ;
   if( (rc  =  md2GetRecord( mdl , &parentDir , 0 )) )goto abort ;
   mdr->body.dirInode.attr.flags = parentDir.body.dirInode.attr.flags;
#endif
   
   mdr->body.dirInode.attr.unixAttr.mst_dev     = mdDeviceID( mdr->head.ID) ;
   mdr->body.dirInode.attr.unixAttr.mst_ino     = mdInodeID( mdr->head.ID) ;
   mdr->body.dirInode.attr.unixAttr.mst_rdev    = 100 ;
   time((time_t*)&mdr->body.dirInode.attr.unixAttr.mst_atime)  ;
   time((time_t*)&mdr->body.dirInode.attr.unixAttr.mst_mtime) ;
   time((time_t*)&mdr->body.dirInode.attr.unixAttr.mst_ctime) ;
   mdr->body.dirInode.attr.unixAttr.mst_mode    = 040777 ;
   mdr->body.dirInode.attr.unixAttr.mst_nlink   = 1 ;
   mdr->body.dirInode.attr.unixAttr.mst_uid     = 0 ;
   mdr->body.dirInode.attr.unixAttr.mst_gid     = 0 ;
   mdr->body.dirInode.attr.unixAttr.mst_size    = 512 ;
   mdr->body.dirInode.attr.unixAttr.mst_sizeHigh    = 0 ;
   mdr->body.dirInode.attr.unixAttr.mst_blksize = 512 ;
   mdr->body.dirInode.attr.unixAttr.mst_blocks  =  1 ;

   mdr->body.dirInode.hashInfo.function = 0 ;
   mdr->body.dirInode.hashInfo.size     = MD_HASH_SIZE ;
   mdr->body.dirInode.hashInfo.entriesPerRow = HASH_POINTERS ;
   mdr->body.dirInode.hashInfo.rows     = 
      MD_HASH_SIZE/HASH_POINTERS + (MD_HASH_SIZE%HASH_POINTERS ? 1 : 0 );

   for( i = 0 , rest = mdr->body.dirInode.hashInfo.size ;
        i < mdr->body.dirInode.hashInfo.rows ;
        i++ , rest -= mdr->body.dirInode.hashInfo.entriesPerRow   ){

     if( (rc  =  md2GetNewRecord( mdl , hash )) )goto abort ;
       mdAddType( hash->head.type , mdtHash ) ;
       mdAddType( hash->head.type , mdtDirectory ) ;
       hash->head.parentID     = dir ;
       hash->head.baseID       = mdr->head.ID ;
       hash->body.dirHash.hashHead.entries  = HASH_POINTERS>rest?rest:HASH_POINTERS ;
       mdr-> body.dirInode.hashHandle[i]    = hash->head.ID ;
       if( (rc = md2PutRecord( mdl , hash , 0 )) )goto abort ;
   }
   
   if( (rc = md2PutRecord( mdl , mdr , 0 )) )goto abort ;
   
 if(md2CommitLock( mdl ))return MDEnoCommit ;
 if( id )*id = mdr -> head.ID ;
 return 0 ;
 
abort :

 md2AbortLock( mdl ) ;
 return rc ;  
}
int md2ForceSizePerm( MDL *mdl , md_id_t id , md_permission perm  , md_long size ) 
{
 int rc ;
 md_record base ;
 int mode ;
 
 if( !  mdpIsNoIO( perm ) ){ rc = MDEnotAllowed ; goto abort ; }
 mode = mdpGetLevel( perm ) ;
 if( ( mode < 0 ) || ( mode > 7 ) )return -1 ;
 if(md2WriteLock( mdl ) )return MDEnoLock ;
   base.head.ID = id ;
   if( (rc  =  md2GetRecord( mdl , &base , 0 )) )goto abort ;
   if( mdIsType( base.head.type , mdtForceIO ) ){ rc = MDEnotAllowed ; goto abort ; }
   if( ( ! mdIsType( base.head.type , mdtInode )   ) ||
       ( ! mdIsType( base.head.type , mdtRegular ) )     ){
     rc = MDEnotFound ;
     goto abort ;
   }
   /*
   if( size > 0x20000000 ){ rc = MDEquota ; goto abort ; } ;
   */
   base.body.fileInode.attr[mode].unixAttr.mst_size = size ;
   base.body.fileInode.attr[mode].unixAttr.mst_sizeHigh = 0 ;
   if( (rc  =  md2PutRecord( mdl , &base , 0 )) )goto abort ;

 if(md2CommitLock( mdl ))return MDEnoCommit ;
 return 0 ;
 
abort :

 md2AbortLock( mdl ) ;
 return rc ;  

}
int md2ForceSize( MDL *mdl , md_id_t id , int mode , md_long size ) 
{
 int rc ;
 md_record base ;
 
 if( ( mode < 0 ) || ( mode > 7 ) )return -1 ;
 if(md2WriteLock( mdl ) )return MDEnoLock ;
   base.head.ID = id ;
   if( (rc  =  md2GetRecord( mdl , &base , 0 )) )goto abort ;
   if( ( ! mdIsType( base.head.type , mdtInode )   ) ||
       ( ! mdIsType( base.head.type , mdtRegular ) )     ){
     rc = MDEnotFound ;
     goto abort ;
   }
   /*
   if( size > 0x20000000 ){ rc = MDEquota ; goto abort ; } ;
   */
   base.body.fileInode.attr[mode].unixAttr.mst_size = size ;
   if( (rc  =  md2PutRecord( mdl , &base , 0 )) )goto abort ;

 if(md2CommitLock( mdl ))return MDEnoCommit ;
 return 0 ;
 
abort :

 md2AbortLock( mdl ) ;
 return rc ;  

}
void md2SetMostAttr(  md_unix *uattr , md_unix *attr )
{
      if( uattr->mst_mode != md_no_mode ){
      
               
         attr->mst_mode = 
           ( attr->mst_mode & ~0777 ) |
           ( uattr->mst_mode & 0777 ) ;
           
      }
		 
      if( uattr->mst_uid != md_no_uid )
         attr->mst_uid = uattr->mst_uid ;
	   
      if( uattr->mst_gid != md_no_gid )
         attr->mst_gid = uattr->mst_gid ;
	   
      if( uattr->mst_atime != md_no_time )
         attr->mst_atime = uattr->mst_atime ;
	   
      if( uattr->mst_mtime != md_no_time )
         attr->mst_mtime = uattr->mst_mtime ;

      return ;
}
int md2ModifyUnixAttr( MDL * mdl , md_id_t id , int mode , md_unix *uattr)
{
 int rc , truncate , setall , i;
 md_record base ;
 md_unix *attr ;
 
 truncate = 0 ;
 setall   = mode & mdpAllLevels ;
 mode     = mode & 7 ;
#ifdef map_special_ids
 mdClearSpecialID(id);
#else
 if( mdIsSpecialID( id ) )return 0 ;
#endif
 
 
 if(md2WriteLock( mdl ) )return MDEnoLock ;
   base.head.ID = id ;
   if( (rc  =  md2GetRecord( mdl , &base , 0 )) )goto abort ;
   if( ! mdIsType( base.head.type , mdtInode ) ){
     rc = MDEnotFound ;
     goto abort ;
   }
   if( mdIsType( base.head.type , mdtTag ) ){
      /*if( rc = md2GetTopTag( mdl , base.head.ID , &base ) )goto abort ;*/
      /*
       * My current understanding of tags cracks the chain as
       * soon as an inhereted tag is changed. The data and the
       * attributes are copied from the master and
       * the changes are done on the local copy. A remove
       * should undo everything.
       */
     if( (rc = md2TagUnChainLow( mdl , &base )) )goto abort ;
      md2SetMostAttr(uattr,&base.body.tagInode.attr.unixAttr );
      /*
       * set size doesn't belong to setAttribute because it's
       * an active operation in case of regulare files.
       * In our case it's just setting the size.
       */
      if( uattr->mst_size != md_no_size )
         base.body.tagInode.attr.unixAttr.mst_size =
         base.body.tagInode.attr.entries = uattr->mst_size ;
   }else{
      if( mdIsType( base.head.type , mdtDirectory ) ){
          attr = &base.body.dirInode.attr.unixAttr ; 
          md2SetMostAttr(uattr,&base.body.dirInode.attr.unixAttr );
      }else if( mdIsType( base.head.type , mdtRegular|mdtLink ) ){
          attr = &base.body.fileInode.attr[mode].unixAttr ;
          if( setall ){
              for( i = 0 ; i < 8 ; i++ )
              md2SetMostAttr(uattr,&base.body.fileInode.attr[i].unixAttr );
          }else{
              md2SetMostAttr(uattr,&base.body.fileInode.attr[mode].unixAttr );
              /* igitt  */
              if( mode == 0 ){
                int level ;
                for( level = 1 ; level < 8 ; level ++ )
                   if( _levelMask[level] < 0 )
                       md2SetMostAttr(&base.body.fileInode.attr[0].unixAttr,
                                      &base.body.fileInode.attr[level].unixAttr );
              }
          }
      }else{
          rc = MDEnotFound ;
          goto abort ;
      }

         
      if( uattr->mst_size != md_no_size ){
         attr->mst_size = uattr->mst_size ;
         truncate = 1 ;
      }
      if( uattr->mst_sizeHigh != md_no_size ){
         attr->mst_sizeHigh = uattr->mst_sizeHigh ;
         truncate = 1 ;
      }

   }
   if( (rc  =  md2PutRecord( mdl , &base , 0 )) )goto abort ;
   
   if( truncate &&  mdIsType( base.head.type , mdtRegular ) ){
     if( (rc = md2TruncateZero( mdl , id , mode )) )goto abort ;
   }

 if(md2CommitLock( mdl ))return MDEnoCommit ;
 return 0 ;
 
abort :

 md2AbortLock( mdl ) ;
 return rc ;  


}
void md2SetMostAttrs(  md_unix *uattr , md_unix *attr )
{
      if( uattr->mst_mode != md_no_mode ){
      
               
         attr->mst_mode = 
           ( attr->mst_mode & ~0777 ) |
           ( uattr->mst_mode & 0777 ) ;
           
      }
		 
      if( uattr->mst_uid != md_no_uid )
         attr->mst_uid = uattr->mst_uid ;
	   
      if( uattr->mst_gid != md_no_gid )
         attr->mst_gid = uattr->mst_gid ;
	   
      if( uattr->mst_atime != md_no_time )
         attr->mst_atime = uattr->mst_atime ;
	   
      if( uattr->mst_mtime != md_no_time )
         attr->mst_mtime = uattr->mst_mtime ;
         
      if( uattr->mst_ctime != md_no_time )
         attr->mst_ctime = uattr->mst_ctime ;

      return ;
}
int md2ModifyUnixAttrs( MDL * mdl , md_id_t id , int mode , md_unix *uattr)
{
 int rc , setall;
 md_record base ;
 md_unix *attr ;
 
 setall   = mode & mdpAllLevels ;
 mode     = mode & 7 ;
#ifdef map_special_ids
 mdClearSpecialID(id);
#else
 if( mdIsSpecialID( id ) )return 0 ;
#endif

 if(md2WriteLock( mdl ) )return MDEnoLock ;
   base.head.ID = id ;
   if( (rc  =  md2GetRecord( mdl , &base , 0 )) )goto abort ;
   if( ! mdIsType( base.head.type , mdtInode ) ){
     rc = MDEnotFound ;
     goto abort ;
   }
   if( mdIsType( base.head.type , mdtInode ) ){
      if( mdIsType( base.head.type , mdtDirectory ) ){
      
          attr = &base.body.dirInode.attr.unixAttr ; 
          md2SetMostAttrs(uattr,&base.body.dirInode.attr.unixAttr );
          
      }else if( mdIsType( base.head.type , mdtRegular ) ){
      
          attr = &base.body.fileInode.attr[mode].unixAttr ;
          md2SetMostAttrs(uattr,&base.body.fileInode.attr[mode].unixAttr );
          
      }else{
          rc = MDEnotFound ;
          goto abort ;
      }
   }else{
      rc = MDEnotFound ;
      goto abort ;
   }
   if( (rc  =  md2PutRecord( mdl , &base , 0 )) )goto abort ;

 if(md2CommitLock( mdl ))return MDEnoCommit ;
 return 0 ;
 
abort :

 md2AbortLock( mdl ) ;
 return rc ;  


}
int md2ModUnixAttr( MDL * mdl , md_id_t id , int mode , md_unix *uattr)
{
 int rc ;
 md_attr attr ;
 
 if( ( mode < 0 ) || ( mode > 7 ) )return -1 ;
 if(md2WriteLock( mdl ) )return MDEnoLock ;
    rc = md2GetAttribute( mdl , id , mode , &attr);
    if( rc ) goto abort ;

    if( uattr->mst_mode != md_no_mode )
       attr.unixAttr.mst_mode = 
         ( attr.unixAttr.mst_mode & ~0777 ) |
         ( uattr->mst_mode & 0777 ) ;
		 
    if( uattr->mst_uid != md_no_uid )
       attr.unixAttr.mst_uid = uattr->mst_uid ;
	   
    if( uattr->mst_gid != md_no_gid )
       attr.unixAttr.mst_gid = uattr->mst_gid ;
	   
    if( uattr->mst_atime != md_no_time )
       attr.unixAttr.mst_atime = uattr->mst_atime ;
	   
    if( uattr->mst_mtime != md_no_time )
       attr.unixAttr.mst_mtime = uattr->mst_mtime ;
   
    if( uattr->mst_size != md_no_size ){
       attr.unixAttr.mst_size = uattr->mst_size ;
    }
	
    if( uattr->mst_sizeHigh != md_no_size ){
       attr.unixAttr.mst_sizeHigh = uattr->mst_sizeHigh ;
    }

    rc = md2SetAttribute( mdl , id , mode , &attr ) ;
    if( rc ) goto abort ;
 if(md2CommitLock( mdl ))return MDEnoCommit ;
 return 0 ;
 
abort :

 md2AbortLock( mdl ) ;
 return rc ;  
}
int md2GetUnixAttr( MDL * mdl , md_id_t id , int mode , md_unix *uattr)
{
 int rc ;
 md_attr attr ;
 md_permission perm ;
 
 if( ( mode < 0 ) || ( mode > 7 ) )return -1 ;
 mdpSetLevel(perm,mode);
 if(md2ReadLock( mdl ) )return MDEnoLock ;
    rc = md2GetExtAttributePerm( mdl , id , perm , &attr , NULL );
    if( rc ) goto abort ;
    memcpy( (char *)uattr ,
            (char *)&attr.unixAttr ,
            sizeof( md_unix ) ) ;
 if(md2CommitLock( mdl ))return MDEnoCommit ;
 return 0 ;
 
abort :

 md2AbortLock( mdl ) ;
 return rc ;  
}
int md2GetAttribute( MDL * mdl , md_id_t id , int mode , md_attr *attr)
{
 md_permission perm ;
   
 mdpSetLevel(perm,mode);
 return md2GetExtAttributePerm( mdl , id , perm , attr , NULL ) ;  
}
int md2GetExtAttribute( MDL * mdl , md_id_t id , int mode , md_attr *attr , mdRecord *rec )
{
   md_permission perm ;
   
   mdpSetLevel(perm,mode);
   
   return md2GetExtAttributePerm( mdl , id , perm ,attr , rec );
}
int md2GetExtAttributePerm( MDL * mdl , md_id_t id , md_permission perm ,
                            md_attr *attr , mdRecord *rec )
{
 md_record base ;
 int mode , special , rc ;

 memset( (char *)attr , 0 , sizeof(md_attr) );
 
 mode = mdGetLevel(perm);
 
 if( ( mode < 0 ) || ( mode > 7 ) )return -1 ;
 
 special = 0 ;
 if(md2ReadLock( mdl ) )return MDEnoLock ;
   if( mdIsSpecialID( id ) ){
      special = mdpGetSpecial( perm ) ;
      mdClearSpecialID(id);
   }
   if( special == MDO_REQ_CONST ){
       md2File * x =  md2xPrintConst( );      
       (void)md2FakeFileUnixAttr( &attr->unixAttr ,id , md2sfGetSize( x ) );     
       attr->unixAttr.mst_ino |= 0x7 ;
       goto ok ;
   }else if( special == MDO_REQ_GET ){
       (void)md2FakeFileUnixAttr( &attr->unixAttr ,id ,
             4*sizeof(md_id_t)+2*sizeof(md_permission)+ 25 );     
       attr->unixAttr.mst_ino |= 0x7 ;
       goto ok ;
   }else if( special == MDO_REQ_LIST ){
       (void)md2FakeFileUnixAttr( &attr->unixAttr ,id , 40 );     
       attr->unixAttr.mst_ino |= 0x7 ;
       goto ok ;
   }else if( special == MDO_REQ_NAMEOF ){
       (void)md2FakeFileUnixAttr( &attr->unixAttr ,id , 256 );     
       attr->unixAttr.mst_ino |= 0x7 ;
       goto ok ;
   }else if( special == MDO_REQ_GET_COUNTERS ){
       (void)md2FakeFileUnixAttr( &attr->unixAttr ,id , 1024 );     
       attr->unixAttr.mst_ino |= 0x7 ;
       goto ok ;
   }else if( special == MDO_REQ_GET_DATABASE ){
       (void)md2FakeFileUnixAttr( &attr->unixAttr ,id , 128 );     
       attr->unixAttr.mst_ino |= 0x7 ;
       goto ok ;
   }else if( special == MDO_REQ_NAME ){
       md_dir_item item ;
       if( (rc = md2FindDirectoryEntryByID( mdl , id ,  &item )) )goto abort;
       (void)md2FakeFileUnixAttr( &attr->unixAttr ,id , strlen( item.name )+1 );     
       attr->unixAttr.mst_ino |= 0x7 ;
       goto ok ;
   }else if( ( special == MDO_REQ_ID ) || ( special == MDO_REQ_PARENT ) ){
       (void)md2FakeFileUnixAttr(&attr->unixAttr,id,sizeof(id)*2+1);             
       attr->unixAttr.mst_ino |= 0x7 ;
       goto ok ;    
   }

   base.head.ID = id ;
   if( (rc  =  md2GetRecord( mdl , &base , 0 )) )goto abort ;
   if( special == MDO_REQ_LSTAGS ){
       md2File * x =  md2xPrintTags(  mdl , &base );      
       (void)md2FakeFileUnixAttr( &attr->unixAttr , base.head.ID , md2sfGetSize( x ) );     
       attr->unixAttr.mst_ino |= 0x7 ;
       goto ok ;
   }else if( special == MDO_REQ_LSXTAGS ){
       md2File * x =  md2xPrintXTags(  mdl , &base );      
       (void)md2FakeFileUnixAttr( &attr->unixAttr , base.head.ID , md2sfGetSize( x ) );     
       attr->unixAttr.mst_ino |= 0x7 ;
       goto ok ;
   }else if( special == MDO_REQ_SHOWID ){
       md2File * x =  md2xPrintRecord(  &base );      
       (void)md2FakeFileUnixAttr( &attr->unixAttr , base.head.ID , md2sfGetSize( x ) );     
       attr->unixAttr.mst_ino |= 0x7 ;
       goto ok ;
   }else if( special == MDO_REQ_GETATTR ){
       md2File * x =  md2xPrintAttributes(  &base );      
       (void)md2FakeFileUnixAttr( &attr->unixAttr , base.head.ID , md2sfGetSize( x ) );     
       attr->unixAttr.mst_ino |= 0x7 ;
       goto ok ;
   }
#ifdef deny_unknown_special
   /* 
      for all unknown specials ( e.g. (set) and (io) ) we
      return the attributes of the corresponding
      object Inode.
    */
   else if( special ){
       rc = MDEnotFound ;
       goto abort ;
   }
#endif
   if( ! mdIsType( base.head.type , mdtInode ) ){
     rc = MDEnotFound ;
     goto abort ;
   }
   if( mdIsType( base.head.type , mdtDirectory ) ){
      memcpy( (char *)attr , 
              (char *)&base.body.dirInode.attr ,
              sizeof( md_attr ) ) ;
      attr->unixAttr.mst_ino = mdInodeLevelID(id,0) ;
   }else if( mdIsType( base.head.type , mdtRegular|mdtLink ) ){
      memcpy( (char *)attr , 
              (char *)&base.body.fileInode.attr[mode] ,
              sizeof( md_attr ) ) ;
      attr->unixAttr.mst_ino = mdInodeLevelID(id,mode) ;
   }else if( mdIsType( base.head.type , mdtTag ) ){
      memset( (char*)attr , 0 , sizeof(md_attr) ) ;
      if( (rc = md2GetTagUnixAttr( mdl , id  , &attr->unixAttr )) )goto abort ;
   }else{
      rc = MDEnotFound ;
      goto abort ;
   }
   
ok:

 if(rec)memcpy( (char *)rec , (char *)&base , sizeof( mdRecord ) ) ;
 if(md2CommitLock( mdl ))return MDEnoCommit ;
 return 0 ;
 
abort :

 md2AbortLock( mdl ) ;
 return rc ;  

}
int md2FakeFileUnixAttrInode( md_unix *attr , int inode , int dev , int size )
{
       memset( (char *)attr , 0 , sizeof( *attr ) ) ;
       attr->mst_dev     = dev ;
       attr->mst_ino     = inode ;
       attr->mst_rdev    = 100 ;
       time((time_t*)& attr->mst_atime)  ;
       time((time_t*)& attr->mst_mtime) ;
       time((time_t*)& attr->mst_ctime) ;
       attr->mst_mode    = 0100444 ;
       attr->mst_nlink   = 1 ;
       attr->mst_uid     = 0 ;
       attr->mst_gid     = 0 ;
       attr->mst_size    = size ;
	   attr->mst_sizeHigh    = 0 ;
       attr->mst_blksize = 512 ;
       attr->mst_blocks  =  0 ;
       return 0 ;
}
int md2FakeFileUnixAttr( md_unix *attr , md_id_t id , int size )
{
       memset( (char *)attr , 0 , sizeof( *attr ) ) ;
       attr->mst_dev     = mdDeviceID( id ) ;
       attr->mst_ino     = mdInodeID( id ) ;
       attr->mst_rdev    = 100 ;
       time((time_t*)& attr->mst_atime)  ;
       time((time_t*)& attr->mst_mtime) ;
       time((time_t*)& attr->mst_ctime) ;
       attr->mst_mode    = 0100444 ;
       attr->mst_nlink   = 1 ;
       attr->mst_uid     = 0 ;
       attr->mst_gid     = 0 ;
       attr->mst_size    = size ;
	   attr->mst_sizeHigh    = 0 ;
       attr->mst_blksize = 512 ;
       attr->mst_blocks  =  0 ;
       return 0 ;
}
int md2ModFlags( MDL * mdl , md_id_t id , md_long * flags , md_long mask )
{
 int rc ;
 md_record base ;
 int i ;
 if(md2WriteLock( mdl ) )return MDEnoLock ;
   base.head.ID = id ;
   if( ((rc  =  md2GetRecord( mdl , &base , 0 ))) )goto abort ;
   
   if( ! ( mdIsType( base.head.type , mdtInode ) &&
           ( mdIsType( base.head.type , mdtRegular ) ||
             mdIsType( base.head.type , mdtDirectory )   )  ) ){
     rc = MDEnotFound ;
     goto abort ;
   }
   if( mdIsType( base.head.type , mdtDirectory ) ){
       *flags = base.body.dirInode.attr.flags =
       ( base.body.dirInode.attr.flags & ~ mask ) | ( (*flags) & mask ) ;
   }else if( mdIsType( base.head.type , mdtRegular ) ){
       for(i=0;i<8;i++)
       *flags = base.body.fileInode.attr[i].flags =
       ( base.body.fileInode.attr[i].flags & ~ mask ) | ( (*flags) & mask ) ;
   }
   if( (rc  =  md2PutRecord( mdl , &base , 0 )) )goto abort ;
   
 if(md2CommitLock( mdl ))return MDEnoCommit ;
 return 0 ;
 
abort :

 md2AbortLock( mdl ) ;
 return rc ;  

}
int md2SetAttribute( MDL * mdl , md_id_t id , int mode , md_attr *attr)
{
 int rc ;
 md_record base ;
 if( ( mode < 0 ) || ( mode > 7 ) )return -1 ;
 if(md2WriteLock( mdl ) )return MDEnoLock ;
   base.head.ID = id ;
   if( (rc  =  md2GetRecord( mdl , &base , 0 )) )goto abort ;
   if( ! mdIsType( base.head.type , mdtInode ) ){
     rc = MDEnotFound ;
     goto abort ;
   }
   if( mdIsType( base.head.type , mdtDirectory ) ){
      memcpy( (char *)&base.body.dirInode.attr , 
              (char *)attr ,
              sizeof( md_attr ) ) ;
   }else if( mdIsType( base.head.type , mdtTag ) ){
      if( attr->unixAttr.mst_size != md_no_size )
         base.body.tagInode.attr.entries = attr->unixAttr.mst_size ;
   }else if( mdIsType( base.head.type , mdtRegular ) ){
      memcpy( (char *)&base.body.fileInode.attr[mode] , 
              (char *)attr ,
              sizeof( md_attr ) ) ;
   }else{
     rc = MDEnotFound ;
     goto abort ;
   }
   if( (rc  =  md2PutRecord( mdl , &base , 0 )) )goto abort ;
 if(md2CommitLock( mdl ))return MDEnoCommit ;
 return 0 ;
 
abort :

 md2AbortLock( mdl ) ;
 return rc ;  

}
int md2ModifyLinkCount( MDL * mdl , md_id_t id , int * diff )
{
 int rc , i ;
 md_record base ;
 
 if(md2WriteLock( mdl ) )return MDEnoLock ;
     base.head.ID = id ;
     if( (rc  =  md2GetRecord( mdl , &base , 0 )) )goto abort ;
     if( ! mdIsType( base.head.type , mdtInode ) ){
         rc = MDEnotFound ;
         goto abort ;
     }
     if(  mdIsType( base.head.type , mdtRegular ) ){
         base.body.fileInode.attr[0].unixAttr.mst_nlink += *diff ;
         *diff = base.body.fileInode.attr[0].unixAttr.mst_nlink ;
         for( i = 1 ; i < 8 ; i++ )
            base.body.fileInode.attr[0].unixAttr.mst_nlink = *diff ;
     }else{
         rc = MDEnotFound ;
         goto abort ;
     }
     if( (rc  =  md2PutRecord( mdl , &base , 0 )) )goto abort ;
 if(md2CommitLock( mdl ))return MDEnoCommit ;
 return 0 ;
 
abort :

 md2AbortLock( mdl ) ;
 return rc ;  

}
int md2WriteData( MDL * mdl , md_id_t id , int mode , char *b , long off , long size )
{
 int      rc ;
 mdRecord base ;

 if( ( mode < 0 ) || ( mode > 7 ) )return -1 ;
 if(md2WriteLock( mdl ) )return MDEnoLock ;
   base.head.ID = id ;
   if( (rc  =  md2GetRecord( mdl , &base , 0 )) )goto abort ;
   
   if( mdIsType( base.head.type , mdtTag ) ){
     if( (rc = md2WriteTagLow( mdl , &base , b , off , size )) )goto abort ;
   }else{
     if( (rc = md2WriteDataLow( mdl , &base , mode , b , off , size )) )goto abort ;
   }
   
   if( (rc  =  md2PutRecord( mdl , &base , 0 )) )goto abort ;
   
 if(md2CommitLock( mdl ))return MDEnoCommit ;
 return 0 ;
 
abort :

 md2AbortLock( mdl ) ;
 return rc ;  

}
int md2WriteDataPerm( MDL * mdl , md_id_t id , md_permission perm ,  char *b , long off , long size )
{
 int      rc;
 mdRecord base ;

 if(md2WriteLock( mdl ) )return MDEnoLock ;


   base.head.ID = id ;
   fprintf(stderr," md2WriteDataPerm : get record : %s\n",mdStringID(base.head.ID));
   if( (rc  =  md2GetRecord( mdl , &base , 0 )) )goto abort ;
   
   if( mdIsType( base.head.type , mdtTag ) ){
     if( (rc = md2WriteTagLow( mdl , &base , b , off , size )) )goto abort ;
  }else{
      if( mdpIsNoIO( perm ) && 
          ( ! mdIsType( base.head.type , mdtForceIO ) ) &&
          ( mdpGetLevel(perm) == 0 )                        ){
         rc = MDEnoIO ;
         goto abort ;
      }else{
	 if( (rc = md2WriteDataLow( mdl , &base , mdpGetLevel(perm) ,
				    b , off , size )) )goto abort ;
      }
   }
   
   if( (rc  =  md2PutRecord( mdl , &base , 0 )) )goto abort ;
   
 if(md2CommitLock( mdl ))return MDEnoCommit ;
 return 0 ;
 
abort :

 md2AbortLock( mdl ) ;
 return rc ;  

}
#define MD2_MAX_META   (4*1024)

int md2WriteDataLow( MDL * mdl , mdRecord *base , int mode , char *b ,
                     long off , long size )
{
 mdRecord rec ;
 int rc ;
   if( mode ){
       if( ( off + 4 ) > MD2_MAX_META )return MDEnoSpace ;
       if( ( off + size ) > MD2_MAX_META )size = MD2_MAX_META - off ;
   }
   time((time_t*)&base->body.fileInode.attr[mode].unixAttr.mst_mtime) ;

   if( mdIsNullID( base->body.fileInode.attr[mode].chain ) ){
       if( ( off + size ) <= DATA_UNITS ){
          return md2WriteDataLowSmall( mdl , base , mode , b , off , size ) ;
       }else{
          return md2WriteDataLowLarge( mdl , base , mode , b , off , size ) ;
       }
   }else{
      rec.head.ID = base->body.fileInode.attr[mode].chain ;
      if( (rc  =  md2GetRecord( mdl , &rec , 0 )) )return rc ;
      if( mdIsType( rec.head.type , mdtHash ) ){
          return md2WriteDataLowLarge( mdl , base , mode , b , off , size ) ;
      }else{
          if( ( off + size ) <= DATA_UNITS ){
             return md2WriteDataLowSmall( mdl , base , mode , b , off , size ) ;
          }else{
	    if( (rc = md2WriteDataLowUpgrade( mdl , base , mode , &rec )) )return rc ;
             return md2WriteDataLowLarge( mdl , base , mode , b , off , size ) ;
          }
      }
      
   }
}
int md2WriteDataLowUpgrade( MDL * mdl , mdRecord *base , int mode , mdRecord *block )
{
  mdRecord hash ;
  int rc ;
  
  if( (rc  =  md2GetNewRecord( mdl , &hash )) )return rc ;
  mdAddType( hash.head.type , mdtRegular ) ;
  mdAddType( hash.head.type , mdtHash ) ;
  base->body.fileInode.attr[mode].chain   = hash.head.ID ;
  hash.head.parentID = base->head.ID ;
  hash.body.fileHash.hashHead.entries =  base->body.fileInode.fileInfo.blocksPerHash ;
  
  block->head.parentID = base->head.ID ;
  block->head.baseID   = hash.head.ID ;
  hash.body.fileHash.hashPointer[0] = block->head.ID ;
  if( (rc  =  md2PutRecord( mdl , &hash , 0 )) )return rc  ;
  if( (rc  =  md2PutRecord( mdl , block , 0 )) )return rc  ;

  return 0 ;
}
int md2WriteDataLowSmall( MDL * mdl , mdRecord *base , int mode , char *b ,
                          long off , long size )
{
 mdRecord block ;
 int rc ;

 if( mdIsNullID( base->body.fileInode.attr[mode].chain ) ){
   if( (rc  =  md2GetNewRecord( mdl , &block )) )return rc ;
   mdAddType( block.head.type , mdtRegular ) ;
   mdAddType( block.head.type , mdtData ) ;
   block.head.parentID = base->head.ID ;
   block.head.baseID   = base->head.ID  ;
   base->body.fileInode.attr[mode].chain = block.head.ID ;
 }else{
   block.head.ID = base->body.fileInode.attr[mode].chain ;
   if( (rc  =  md2GetRecord( mdl , &block , 0 )) )return rc ;
 }  
   memcpy( &block.body.fileData.data[off] , b , size ) ;
   base->body.fileInode.attr[mode].entries =  
      md_max( base->body.fileInode.attr[mode].entries , ( off + size ) );
   base->body.fileInode.attr[mode].unixAttr.mst_size = 
      base->body.fileInode.attr[mode].entries ;
   if( (rc  =  md2PutRecord( mdl , &block , 0 )) )return rc  ;

   return 0 ;
}
int md2WriteDataLowLarge( MDL * mdl , mdRecord *base , int mode , char *b , long off , long size )
{
 mdRecord last , hash , block  ;
 int      rc ;
 long     whichHash , whichBlock , bytesPerHash , whichOff ;
 long     rest , whichRest , thisHash , pos , cpy ;
 md_id_t  *thisIdPtr ;

   bytesPerHash = base->body.fileInode.fileInfo.bytesPerBlock *
                  base->body.fileInode.fileInfo.blocksPerHash    ;
   whichHash  = off / bytesPerHash ;
   rest       = off - ( whichHash * bytesPerHash ) ;
   whichBlock = rest / base->body.fileInode.fileInfo.bytesPerBlock ;
   whichOff   = rest - ( whichBlock * base->body.fileInode.fileInfo.bytesPerBlock ) ;
   whichRest  = base->body.fileInode.fileInfo.bytesPerBlock - whichOff ;
   /*printf( " Hash %d Block %d Off %d Rest %d \n" ,
             whichHash,whichBlock,whichOff,whichRest ) ;*/
   /*
    * find the hash block
    */
   thisIdPtr  = &base->body.fileInode.attr[mode].chain ;
   mdSetNullID( hash.head.ID ) ;
   rest = size ;
   for( thisHash = 0 ; rest > 0 ; thisHash++ ){
       mdCopyRecord( &last , &hash ) ;
       if( mdIsNullID( *thisIdPtr ) ){
	 if( (rc  =  md2GetNewRecord( mdl , &hash )) )goto abort ;
           mdAddType( hash.head.type , mdtRegular ) ;
           mdAddType( hash.head.type , mdtHash ) ;
            *thisIdPtr         = hash.head.ID ;
            hash.head.parentID = base->head.ID ;
            hash.body.fileHash.hashHead.entries = 
                   base->body.fileInode.fileInfo.blocksPerHash ;
            if( ! mdIsNullID( last.head.ID ) ){
	      if( (rc  =  md2PutRecord( mdl , &last , 0 )) )goto abort ;
            }
       }else{
            hash.head.ID = *thisIdPtr ;
            if( (rc  =  md2GetRecord( mdl , &hash , 0 )) )goto abort ;
       }
       thisIdPtr = &last.head.nextID ;
       if( thisHash == whichHash ){
           for( pos = whichBlock ; rest > 0 ; pos++){
             if( pos >= base->body.fileInode.fileInfo.blocksPerHash ){
                 whichHash++ ;
                 whichBlock = 0 ;
                 break ;
             }
             if( mdIsNullID( hash.body.fileHash.hashPointer[pos] ) ){
               if( (rc  =  md2GetNewRecord( mdl , &block )) )goto abort ;
               mdAddType( block.head.type , mdtRegular ) ;
               mdAddType( block.head.type , mdtData ) ;
               block.head.parentID = base->head.ID ;
               block.head.baseID   = hash.head.ID ;
               hash.body.fileHash.hashPointer[pos] = block.head.ID ;
               if( (rc  =  md2PutRecord( mdl , &hash , 0 )) )goto abort ;
             }else{
               block.head.ID = hash.body.fileHash.hashPointer[pos] ;
               if( (rc  =  md2GetRecord( mdl , &block , 0 )) )goto abort ;
             }
             cpy  = md_min( rest , whichRest ) ;
             memcpy( &block.body.fileData.data[whichOff] , b , cpy ) ;
             /*printf( " offset %d copy %d\n" , whichOff , cpy ) ;*/
             rest  -= cpy ;
             b     += cpy ;
             if( (rc  =  md2PutRecord( mdl , &block , 0 )) )goto abort ;
             whichOff   = 0 ;
             whichRest  = base->body.fileInode.fileInfo.bytesPerBlock ;
           }
       }
   }
   base->body.fileInode.attr[mode].entries =  
      md_max( base->body.fileInode.attr[mode].entries , ( off + size ) );
   base->body.fileInode.attr[mode].unixAttr.mst_size = 
      base->body.fileInode.attr[mode].entries ;
   
 return 0 ;
 
abort :

 return rc ;  

}
int md2ReadData( MDL * mdl , md_id_t id , int mode , char *b , long off , long size )
{
 int      rc ;
 mdRecord base ;

 if( ( mode < 0 ) || ( mode > 7 ) )return -1 ;

 if(md2ReadLock( mdl ) )return MDEnoLock ;
   base.head.ID = id ;
   if( (rc  =  md2GetRecord( mdl , &base , 0 )) )goto abort ;
   
   if( mdIsType( base.head.type , mdtTag ) ){
       rc = md2ReadTagLow( mdl , &base , b , off , size ) ;
       if( rc < 0 )goto abort ; 
   }else{
       rc = md2ReadDataLow( mdl , &base , mode , b , off , size ) ;
       if( rc < 0 )goto abort ; 
   }

 if(md2CommitLock( mdl ))return MDEnoCommit ;
 return rc ;
 
abort :

 md2AbortLock( mdl ) ;
 return rc ;  

}
int md2ReadDataPerm( MDL * mdl , md_id_t id , md_permission perm ,
                     md_id_t mountID , char *b , long off , long size )
{
 int      rc , special ;
 mdRecord base ;
 int len ;
 
 if(md2ReadLock( mdl ) )return MDEnoLock ;
   special = 0 ;
   if( mdIsSpecialID( id ) ){
      mdClearSpecialID(id);
      special = mdpGetSpecial( perm ) ;
      if( special == MDO_REQ_ID ){
          len = 2*sizeof(md_id_t)+1 ;
          if( size < len ){ rc = MDEnotFound ; goto abort ; }
          sprintf( b , "%s\n" , mdStringID(id) ) ;
          rc = strlen( b ) ;
          goto ok ;
      }else if( special == MDO_REQ_GET ){
          /*
               result :
               ( important for osmcp to stay exactly with this sequence )
                 dirID=<dirId>\n
                 dirPerm=<dirPerm>\n
                 mountID=<mountID>\n
           */
           char * cursor ;
          len = 4*sizeof(md_id_t)+2*sizeof(md_permission)+25 ;
          if( size < len ){ rc = MDEnotFound ; goto abort ; }
          cursor = b ;
          sprintf( cursor , "dirID=%s\n" , mdStringID(id) ) ;
          cursor += strlen( cursor ) ;
          sprintf( cursor , "dirPerm=%s\n" , mdStringPermission(perm) ) ;
          cursor +=strlen( cursor ) ;
          sprintf( cursor , "mountID=%s\n" , mdStringID(mountID) ) ;
          rc = strlen( b ) ;
          goto ok ;
      }else if( special == MDO_REQ_PARENT ){
          base.head.ID = id ;
          if( (rc  =  md2GetRecord( mdl , &base , 0 )) )goto abort ;
          len = 2*sizeof(md_id_t)+1 ;
          if( size < len ){ rc = MDEnotFound ; goto abort ; }
          sprintf( b , "%s\n" , mdStringID(base.head.parentID) ) ;
          rc = strlen( b ) ;
          goto ok ;
      }else if( ( special == MDO_REQ_SHOWID  ) ||
                ( special == MDO_REQ_GETATTR ) ||
                ( special == MDO_REQ_LSXTAGS ) ||
                ( special == MDO_REQ_LSTAGS  )     )  {
        md2File * x ;  int len ; char *buf ;
        base.head.ID = id ;
        if( (rc  =  md2GetRecord( mdl , &base , 0 )) )goto abort ;
        x =  ( special == MDO_REQ_SHOWID )  ? md2xPrintRecord(  &base ) :
             ( special == MDO_REQ_LSTAGS )  ? md2xPrintTags( mdl ,  &base ) :
             ( special == MDO_REQ_LSXTAGS ) ? md2xPrintXTags( mdl ,  &base ) :
             ( special == MDO_REQ_GETATTR ) ? md2xPrintAttributes( &base ) :
              NULL ;
        if( x == NULL ){ rc = MDEnotFound ; goto abort ; }
        buf = md2sfGetBuffer(x) ;
        len = md2sfGetSize(x) - off ;
        if( len <= 0 ){ rc = 0 ; goto ok ; }
        len = len < size ? len : size ;
        memcpy( b , &buf[off] , len ) ;
        rc = len ;
        goto ok ;
      }else if(   special ==  MDO_REQ_LIST )  {
          sprintf( b , "%s\n" ,mdStringPermission(perm) ) ;
          rc = strlen( b ) ;
          goto ok ;
      }else if(   special ==  MDO_REQ_NAME )  {
         md_dir_item item ;
	 if( (rc = md2FindDirectoryEntryByID( mdl , id ,  &item )) )goto abort;
          len = strlen( item.name ) ;
          if( size < len ){ rc = MDEnotFound ; goto abort ; }
          sprintf( b , "%s\n" ,item.name ) ;
          rc = strlen( b ) ;
          goto ok ;
      }else if(   special ==  MDO_REQ_CONST )  {
        md2File * x ;  int len ; char *buf ;
        x =  md2xPrintConst( ) ;
        if( x == NULL ){ rc = MDEnotFound ; goto abort ; }
        buf = md2sfGetBuffer(x) ;
        len = md2sfGetSize(x) - off ;
        if( len <= 0 ){ rc = 0 ; goto ok ; }
        len = len < size ? len : size ;
        memcpy( b , &buf[off] , len ) ;
        rc = len ;
        goto ok ;
      }else{
          rc = MDEnotFound ;
          goto abort ;
      }
   }


   base.head.ID = id ;
   if( (rc  =  md2GetRecord( mdl , &base , 0 )) )goto abort ;
   
   if( mdIsType( base.head.type , mdtTag ) ){
       rc = md2ReadTagLow( mdl , &base , b , off , size ) ;
       if( rc < 0 )goto abort ; 
   }else{
       if( mdpIsNoIO( perm ) && 
           ( ! mdIsType( base.head.type , mdtForceIO ) ) &&
           ( mdpGetLevel(perm) == 0 )                                               ){
           rc = MDEnoIO ; 
       }else{
           rc = md2ReadDataLow( mdl , &base , mdpGetLevel(perm) , b , off , size ) ;
           if( rc < 0 )goto abort ; 
       }
   }
ok:
 if(md2CommitLock( mdl ))return MDEnoCommit ;
 return rc ;
 
abort :

 md2AbortLock( mdl ) ;
 return rc ;  

}
int md2ReadDataLow( MDL * mdl , mdRecord *base , int mode , char *b , long off , long size )
{

 mdRecord rec ;
 int rc ;
 
 if( mdIsNullID( base->body.fileInode.attr[mode].chain ) ){ 
   return 0 ; 
 }else{
      rec.head.ID =  base->body.fileInode.attr[mode].chain ;
      if( (rc  =  md2GetRecord( mdl , &rec , 0 )) )return rc ;
      if( mdIsType( rec.head.type , mdtHash ) ){
          return md2ReadDataLowLarge( mdl , base , mode , b , off , size ) ;
      }else{
          return md2ReadDataLowSmall( mdl , base , &rec , mode , b , off , size ) ;
      }
      
 
 
 }
}
#define _READ_READ_SIZE
#ifdef _READ_READ_SIZE
int md2ReadDataLowSmall( MDL * mdl , mdRecord *base , mdRecord *block ,
                         int mode , char *b , long off , long size )
{
  long rsize , rend ;

  rend = md_min( DATA_UNITS , base -> body.fileInode.attr[mode].entries ) ;
 
  if( off < DATA_UNITS ){
    rsize = md_min( size , rend - off ) ;
    memcpy( b , &block->body.fileData.data[off] , rsize ) ;
  }else
    rsize = 0 ;
  
  return rsize ;
}
#else
int md2ReadDataLowSmall( MDL * mdl , mdRecord *base , mdRecord *block ,
                         int mode , char *b , long off , long size )
{
  long pos ;
  
  if( off < DATA_UNITS ){
    pos = md_min( size , DATA_UNITS - off ) ;
    memcpy( b , &block->body.fileData.data[off] , md_min( size ,pos ) ) ;
  }else
    pos = 0 ;
  
  if( ( off + size ) > DATA_UNITS )memset( &b[pos] , 0 , ( off+size) - DATA_UNITS ) ;
  
  return size ;
}
#endif
int md2ReadDataLowLarge( MDL * mdl , mdRecord *base , int mode , char *b , long off , long size )
{
 mdRecord hash , block  ;
 int      rc , genRC ;
 long     whichHash , whichBlock , bytesPerHash , whichOff ;
 long     rest , whichRest , thisHash , pos , cpy ;
 md_id_t  thisId ;

   genRC = 0 ;

   bytesPerHash = base->body.fileInode.fileInfo.bytesPerBlock *
                  base->body.fileInode.fileInfo.blocksPerHash    ;
   whichHash  = off / bytesPerHash ;
   rest       = off - ( whichHash * bytesPerHash ) ;
   whichBlock = rest / base->body.fileInode.fileInfo.bytesPerBlock ;
   whichOff   = rest - ( whichBlock * base->body.fileInode.fileInfo.bytesPerBlock ) ;
   whichRest  = base->body.fileInode.fileInfo.bytesPerBlock - whichOff ;
   /*
    * find the hash block
    */
   thisId  = base->body.fileInode.attr[mode].chain ;
   mdSetNullID( hash.head.ID ) ;
   rest  = md_min( off+size , base->body.fileInode.attr[mode].entries ) -off ;
   if( rest <= 0 ){ genRC = 0 ; goto ok ; }
   genRC = rest ;
   for( thisHash = 0 ; rest > 0 ; thisHash++ ){
       if( mdIsNullID( thisId ) ){
            memset( b , 0 , rest ) ;
            break ;
       }else{
            hash.head.ID = thisId ;
            if( (rc  =  md2GetRecord( mdl , &hash , 0 )) )goto abort ;
       }
       thisId = hash.head.nextID ;
       if( thisHash == whichHash ){
           for( pos = whichBlock ; rest > 0 ; pos++){
             if( pos >= base->body.fileInode.fileInfo.blocksPerHash ){
                 whichHash++ ;
                 whichBlock = 0 ;
                 whichOff   = 0 ;
                 whichRest  = base->body.fileInode.fileInfo.bytesPerBlock ;
                 break ;
             }
             cpy  = md_min( rest , whichRest ) ;
             if( mdIsNullID( hash.body.fileHash.hashPointer[pos] ) ){
                memset( b , 0 , cpy ) ;
             }else{
                block.head.ID = hash.body.fileHash.hashPointer[pos] ;
                if( (rc  =  md2GetRecord( mdl , &block , 0 )) )goto abort ;
                memcpy( b , &block.body.fileData.data[whichOff] , cpy ) ;
                whichOff   = 0 ;
                whichRest  = base->body.fileInode.fileInfo.bytesPerBlock ;

             }
             rest  -= cpy ;
             b     += cpy ;
           }
       }
   }
ok:
 return genRC ;
 
abort :

 return rc ;  

}
int md2RemoveFile( MDL * mdl , md_id_t dirID , md_permission perm, char *name )
{
 int           rc  ;
 md_id_t       resID ;
 md_dir_item   item ;
 md_object    *object ;
 
 
 if(md2WriteLock( mdl ) )return MDEnoLock ;
   rc =  0 ;
   if( ( object = md2IsObject( name ) ) != NULL ){
      /*
       *  the special syntax part
       */
      int oType = md2ObjectType( object ) ;
      
      if( oType == MDO_TAG ){
         mdRecord tag ;
         if( (rc = md2FindTag( mdl , dirID ,
                               object -> argv[0] , &tag )) )goto abort ;
         if( (rc = md2RemoveTag( mdl , tag.head.ID )) )goto abort ;
      }else if( oType == MDO_REQ_SET ){
         if( object->argc < 2 ){ rc = MDEnotFound ; goto abort ; }
         if( ! strcmp( object->argv[1] , "size" ) ){
           mdRecord    mdr ;
           md_dir_item item ;
           /*if( object -> argc < 3 ){ rc = MDEnotFound ; goto abort ; }*/
           if( ( mdGetLevel(perm) != 0 ) /* ||
               ( ! mdpIsNoIO(perm)     )    || */ ){
             rc = MDEnotFound ; goto abort ;  
           }
           if( (rc = mdFindDirectoryEntry(mdl ,dirID ,
                                          object->argv[0] , &item )))goto abort ;
           mdr.head.ID = item.ID ;
           if( (rc  =  md2GetRecord( mdl , &mdr , 0 )) )goto abort ;
           if( ( ! mdIsType( mdr.head.type , mdtInode )   ) ||
               ( ! mdIsType( mdr.head.type , mdtRegular ) ) ||
               (   mdIsType( mdr.head.type , mdtForceIO ) ) ){
                rc = MDEnotFound ;
                goto abort ;
           }
           if( mdr.body.fileInode.attr[0].unixAttr.mst_size == 0 ){
                rc = MDEnotFound ;
                goto abort ;
           }else{
              mdr.body.fileInode.attr[0].unixAttr.mst_size = 0 ;
              if( (rc  =  md2PutRecord( mdl , &mdr , 0 )) )goto abort ;
           }
         }else if( ! strcmp( object->argv[1] , "io" ) ){
           mdRecord    mdr ;
           md_dir_item item ;
           if( ( mdGetLevel(perm) != 0 )   ){
             rc = MDEnotFound ; goto abort ;  
           }
           if( (rc = mdFindDirectoryEntry(mdl ,dirID ,
					  object->argv[0] , &item )))goto abort ;
           mdr.head.ID = item.ID ;
           if( (rc  =  md2GetRecord( mdl , &mdr , 0 )) )goto abort ;
           if( ( ! mdIsType( mdr.head.type , mdtInode )   ) ||
               ( ! mdIsType( mdr.head.type , mdtRegular ) ) ||
               ( ! mdIsType( mdr.head.type , mdtForceIO ) ) ){
                rc = MDEnotFound ;
                goto abort ;
           }
           mdRmType( mdr.head.type , mdtForceIO );
           if( (rc  =  md2PutRecord( mdl , &mdr , 0 )) )goto abort ;
         }
      }
   }else{
      /*
       * the normal namespace part
       */
      rc = md2ExtLookup( mdl , dirID , name ,&resID , &item );
      if(rc)goto abort ;
      
      if( dirID.db != resID.db ){ rc = MDEdbXsearch ; goto abort ; }
      rc = md2ExtRemoveFromDirectory( mdl , dirID , name , &resID,&item );
      if( rc )goto abort ;
      if( (rc = md2DeleteFile( mdl , resID )) )goto abort ;
      /* Here we register the file deletion if required */
      if (mdl->dbBackup!=NULL)
    	  md2RegisterDeletetion(mdl, dirID, name, resID);
   }
   
 if(md2CommitLock( mdl ))return MDEnoCommit ;
 return rc ;
 
abort :

 md2AbortLock( mdl ) ;
 return rc ;  

}
int md2TruncateZero( MDL * mdl , md_id_t id , int level )
{
 mdRecord inode;
 int      rc;
 
 if(md2WriteLock( mdl ) )return MDEnoLock ;
 
   if( mdIsSpecialID( id ) )return MDEnotFound ;
 
   inode.head.ID = id ;
   if( (rc = md2GetRecord( mdl , &inode , 0 )) )goto abort ;
   if( ! ( mdIsType( inode.head.type , mdtInode ) ) ){rc = MDEnotRegular ; goto abort ;}
     if(  mdIsType( inode.head.type , mdtRegular )  ){
   
        if( ! mdIsNullID( inode.body.fileInode.attr[level].chain ) ){
          if( (rc = md2DeleteChain( mdl , inode.body.fileInode.attr[level].chain )) )goto abort ;
          mdSetNullID( inode.body.fileInode.attr[level].chain ) ;
	  inode.body.fileInode.attr[level].unixAttr.mst_size = 0 ;
	  inode.body.fileInode.attr[level].entries           = 0 ;

     
          if( (rc = md2PutRecord( mdl , &inode , 0 )) )goto abort ;
        }
   }else if(  mdIsType( inode.head.type , mdtTag )  ){
       if( (rc = md2TagUnChain( mdl , &inode )) )goto abort ;
   }else{
     rc = MDEnotRegular ;
     goto abort ;
   }
 if(md2CommitLock( mdl ))return MDEnoCommit ;
 return 0 ;
 
abort :

 md2AbortLock( mdl ) ;
 return rc ;  

}
int md2DeleteChain( MDL * mdl , md_id_t idIn )
{
 int rc , i , h ;
 mdRecord hash ;
 md_id_t  id ;
 
  if( mdIsNullID( idIn ) )return 0 ;
  hash.head.ID = idIn ;
  if( (rc = md2GetRecord( mdl , &hash , 0 )) )return rc ;
  
  if( mdIsType( hash.head.type , mdtHash ) ){

      for(  i = 0 ;
            ! mdIsNullID( hash.head.ID ) ;
            hash.head.ID = hash.head.nextID , i++ ){
      
         if( i && (  rc = md2GetRecord( mdl , &hash , 0 ) ) )return rc ;
         
         for( h = 0 ; h < hash.body.fileHash.hashHead.entries ; h++ )
           if( ! mdIsNullID( id = hash.body.fileHash.hashPointer[h] ) )
             if( (rc = md2DeleteRecord( mdl , id , 0 )) )return rc ;
             
         if( (rc = md2DeleteRecord( mdl , hash.head.ID , 0 )) )return rc ;
         
      }
  }else{  
     if( (rc = md2DeleteRecord( mdl , hash.head.ID , 0 )) )return rc ;
  }
  return 0 ;      
}
int md2DeleteFile( MDL * mdl , md_id_t id )
{
 mdRecord inode;
 md_id_t link ;
 int      i , rc;
 
 if(md2WriteLock( mdl ) )return MDEnoLock ;
   inode.head.ID = id ;
   if( (rc = md2GetRecord( mdl , &inode , 0 )) )goto abort ;
   if( ! ( mdIsType( inode.head.type , mdtInode ) &&
           ( mdIsType( inode.head.type , mdtRegular ) ||
             mdIsType( inode.head.type , mdtLink    )     ) ) 
      ){
      rc = MDEnotRegular ;
      goto abort ;         
   }
   if( mdIsType( inode.head.type , mdtRegular ) ){
     for( i = 0 ; i < 8 ; i++ ){
   
      if( mdIsNullID( inode.body.fileInode.attr[i].chain ) )continue ;
      if( (rc = md2DeleteChain( mdl , inode.body.fileInode.attr[i].chain )) )goto abort ;

     }
   }else if( mdIsType( inode.head.type , mdtLink ) ){
     for( i = 0 ; i < 8 ; i++ ){   
      if( mdIsNullID( link = inode.body.fileInode.attr[i].chain ) )continue ;
      if( (rc = md2DeleteRecord( mdl , link , 0 )) )goto abort ;
     }
   }else{
      rc = MDEnotRegular ;
      goto abort ;         
   }
   if( (rc = md2DeleteRecord( mdl , inode.head.ID , 0 )) )goto abort ;
  
 if(md2CommitLock( mdl ))return MDEnoCommit ;
 return 0 ;
 
abort :

 md2AbortLock( mdl ) ;
 return rc ;  

}

int md2RegisterDeletetion(MDL * mdl, md_id_t dirid, char * name, md_id_t id)
{
	mdRecord mdr;
	mdlDatum key, val;
	int size;

	mdr.head.ID = id;
	mdr.head.parentID = dirid;
	mdr.head.cTime = time(0);
	
	strcpy(mdr.body.raw, name);
	size = strlen(name);
	
	key.dptr = (char *)&(mdr.head.ID) ;
	key.dsize = sizeof(mdr.head.ID);
	  
	val.dptr = (char *)&mdr ;
	val.dsize = size + sizeof(md_head);

	return mdxRegisterDeletetion(mdl->dbBackup, key, val);
}

int md2MakeNewFile( MDL * mdl , md_id_t dir , md_id_t *id )
{
 mdRecord  Mdr , *mdr , parentDir ;
 int       rc ,i ;
  
 mdr  = &Mdr ;
  
 if(md2WriteLock( mdl ) )return MDEnoLock ;
  
 if( (rc  =  md2GetNewRecord( mdl , mdr )) )goto abort ;
   
   mdAddType( mdr->head.type , mdtInode ) ;
   mdAddType( mdr->head.type , mdtRegular ) ;
   
   mdr->head.parentID = dir ;

#ifdef MD_FLAGS_INHERIT   
   parentDir.head.ID = dir ;
   if( (rc  =  md2GetRecord( mdl , &parentDir , 0 )) )goto abort ;
   mdr->body.fileInode.attr[0].flags = parentDir.body.dirInode.attr.flags;
#endif
   
   mdr->body.fileInode.attr[0].unixAttr.mst_dev     = mdDeviceID( mdr->head.ID) ;
   mdr->body.fileInode.attr[0].unixAttr.mst_ino     = mdInodeID( mdr->head.ID) ;
   mdr->body.fileInode.attr[0].unixAttr.mst_rdev    = 100 ;
   time((time_t*)&mdr->body.fileInode.attr[0].unixAttr.mst_atime)  ;
   time((time_t*)&mdr->body.fileInode.attr[0].unixAttr.mst_mtime) ;
   time((time_t*)&mdr->body.fileInode.attr[0].unixAttr.mst_ctime) ;
   mdr->body.fileInode.attr[0].unixAttr.mst_mode    = 0100600 ;
   mdr->body.fileInode.attr[0].unixAttr.mst_nlink   = 1 ;
   mdr->body.fileInode.attr[0].unixAttr.mst_uid     = 0 ;
   mdr->body.fileInode.attr[0].unixAttr.mst_gid     = 0 ;
   mdr->body.fileInode.attr[0].unixAttr.mst_size    = 0 ;
   mdr->body.fileInode.attr[0].unixAttr.mst_sizeHigh    = 0 ;
   mdr->body.fileInode.attr[0].unixAttr.mst_blksize = 512 ;
   mdr->body.fileInode.attr[0].unixAttr.mst_blocks  =  0 ;

   for( i  = 1 ; i < 8 ; i++ ){
     memcpy( &mdr->body.fileInode.attr[i] , 
             &mdr->body.fileInode.attr[0] , 
             sizeof( mdr->body.fileInode.attr[0] )  ) ;
     mdr->body.fileInode.attr[i].unixAttr.mst_ino += i ;
     mdr->body.fileInode.attr[i].flags = mdr->body.fileInode.attr[0].flags;	 
   }
             
   mdr->body.fileInode.fileInfo.bytesPerBlock = DATA_UNITS ;
   mdr->body.fileInode.fileInfo.blocksPerHash = DATA_POINTERS ;
   
   if( (rc = md2PutRecord( mdl , mdr , 0 )) )goto abort ;
   
 if(md2CommitLock( mdl ))return MDEnoCommit ;
 if( id )*id = mdr -> head.ID ;
 return 0 ;
 
abort :

 md2AbortLock( mdl ) ;
 return rc ;  
}
int md2MakeNewLink( MDL * mdl , md_id_t dir , md_id_t *id )
{
 mdRecord  Mdr , *mdr;
 int       rc ,i ;
  
 mdr  = &Mdr ;
  
 if(md2WriteLock( mdl ) )return MDEnoLock ;
  
 if( (rc  =  md2GetNewRecord( mdl , mdr )) )goto abort ;
   
   mdAddType( mdr->head.type , mdtInode | mdtLink ) ;
   
   mdr->head.parentID = dir ;
   
   mdr->body.fileInode.attr[0].unixAttr.mst_dev     = mdDeviceID( mdr->head.ID) ;
   mdr->body.fileInode.attr[0].unixAttr.mst_ino     = mdInodeID( mdr->head.ID) ;
   mdr->body.fileInode.attr[0].unixAttr.mst_rdev    = 100 ;
   time((time_t*)&mdr->body.fileInode.attr[0].unixAttr.mst_atime)  ;
   time((time_t*)&mdr->body.fileInode.attr[0].unixAttr.mst_mtime) ;
   time((time_t*)&mdr->body.fileInode.attr[0].unixAttr.mst_ctime) ;
   mdr->body.fileInode.attr[0].unixAttr.mst_mode    = 0120777 ;
   mdr->body.fileInode.attr[0].unixAttr.mst_nlink   = 1 ;
   mdr->body.fileInode.attr[0].unixAttr.mst_uid     = 0 ;
   mdr->body.fileInode.attr[0].unixAttr.mst_gid     = 0 ;
   mdr->body.fileInode.attr[0].unixAttr.mst_size    = 0 ;
   mdr->body.fileInode.attr[0].unixAttr.mst_sizeHigh    = 0 ;
   mdr->body.fileInode.attr[0].unixAttr.mst_blksize = 512 ;
   mdr->body.fileInode.attr[0].unixAttr.mst_blocks  =  0 ;

   for( i  = 0 ; i < 8 ; i++ ){
     memcpy( &mdr->body.fileInode.attr[i] , 
             &mdr->body.fileInode.attr[0] , 
             sizeof( mdr->body.fileInode.attr[0] )  ) ;
     mdr->body.fileInode.attr[i].unixAttr.mst_ino += i ;	 
   }
   
   if( (rc = md2PutRecord( mdl , mdr , 0 )) )goto abort ;
   
 if(md2CommitLock( mdl ))return MDEnoCommit ;
 if( id )*id = mdr -> head.ID ;
 return 0 ;
 
abort :

 md2AbortLock( mdl ) ;
 return rc ;  
}
int md2HushFunc1( char *name , int size )
{
   int i , sum ;
   
   for( i = 0 , sum = 0 ; name[i] != '\0' ; i++ )sum += name[i] ;
   if( ! i )return -1 ;
   return sum % size ;
}
int md2HushFunc0( char *name , int size )
{
   int i , sum ;
   static int prim[] = {2,3,5,7,11,13,17,19,23,29,31} ;
   
   for( i = 0 , sum = 0 ; name[i] != '\0' ; i++ )
       sum += (name[i]*prim[i%(sizeof(prim)/sizeof(prim[0]))]) ;
   if( ! i )return -1 ;
   return sum % size ;
}
int md2PrintDirectory( MDL * mdl , md_id_t dirID , int mode , FILE *f )
{
 int      rc , i , j , k , m ;
 mdRecord dir , hash , data , ent ;
 md_id_t  id ;
 char *strattr ;
 
 if(md2ReadLock( mdl ) )return MDEnoLock ;
    dir.head.ID = dirID ;
    if( (rc = md2GetRecord( mdl , &dir , 0 )) )goto abort ;
    if( ! ( mdIsType( dir.head.type , mdtDirectory ) &&
            mdIsType( dir.head.type , mdtInode )        ) ){
       rc = MDEnotDirectory ;
       goto abort ;
    }
    for( i = 0 ; i < dir.body.dirInode.hashInfo.rows ; i++ ){
        hash.head.ID = dir.body.dirInode.hashHandle[i] ;
        if( (rc = md2GetRecord( mdl , &hash , 0 )) )goto abort ;  
        for( j = 0 ; j < hash.body.dirHash.hashHead.entries ; j++ ){
           if( mdIsNullID( hash.body.dirHash.hashPointer[j] ) )continue ;
           for( id = hash.body.dirHash.hashPointer[j] ;
                ! mdIsNullID(id) ; 
                id = data.head.nextID  ){
                
                data.head.ID = id ;
                if( (rc = md2GetRecord( mdl , &data , 0 )) )goto abort ;
                for( k = 0 ; k < data.body.dirData.dirHead.entries ; k++ ){
                 if( ! mdIsNullID( data.body.dirData.dirItem[k].ID ) )
                   if( mode ){
                        ent.head.ID = data.body.dirData.dirItem[k].ID ;
                        if( (rc = md2GetRecord( mdl , &ent , 0 )) ){
                           fprintf(f,"%s -- NO ATTRIBUTES --  %s\n",
                                   mdStringID( data.body.dirData.dirItem[k].ID ) ,
                                   data.body.dirData.dirItem[k].name               ) ;
                        }else{
                           if( mdIsType( ent.head.type , mdtDirectory ) ){
                              fprintf(f,"%s ", mdStringID( data.body.dirData.dirItem[k].ID ) ) ;
                              fprintf(f,"%s ", mdStringPermission( data.body.dirData.dirItem[k].perm ) ) ;
                              strattr = md2PrintUnixAttrS( &ent.body.dirInode.attr.unixAttr );
                              fprintf(f,"%s %s\n", strattr , data.body.dirData.dirItem[k].name ) ;
                           }else{
                              for(m=0;m<8;m++){
                                fprintf(f,"%s ", mdStringID( data.body.dirData.dirItem[k].ID ) ) ;
                                fprintf(f,"%s ", mdStringPermission( data.body.dirData.dirItem[k].perm ) ) ;
                                strattr = md2PrintUnixAttrS( &ent.body.fileInode.attr[m].unixAttr );
                                fprintf(f,"%s (%d)%s\n",strattr,m,data.body.dirData.dirItem[k].name ) ;
                              }
                           }
                        }
                   }else{
                       mdFhandle m ;
                       m.id         = data.body.dirData.dirItem[k].ID ;
                       m.permission = data.body.dirData.dirItem[k].perm ;
                       fprintf(f,"%s %8.8lX %s\n",
                               mdStringFhandle( m ) ,
							   data.body.dirData.dirItem[k].expire , 
                               data.body.dirData.dirItem[k].name               ) ;
                   }
                }
           }
        }
    }
 if(md2CommitLock( mdl ))return MDEnoCommit ;
 return 0 ;
 
abort :

 md2AbortLock( mdl ) ;
 return rc ;  

}
int md2MakeDirectory( MDL * mdl , md_id_t dirID , char *name , md_id_t *resID )
{
 md_id_t  newID , rootID ;
 mdRecord rootRec ;
 int      rc ;
 
 if(md2WriteLock( mdl ) )return MDEnoLock ;
 if( (rc = md2MakeNewDirectory( mdl , dirID , &newID ))  )goto abort ;
 
 if( (rc = md2DuplicateTags( mdl , dirID , newID )) )goto abort ;
 mdSetRootID( rootID ) ;
    if( mdIsEqualID( rootID , dirID ) ){
       rootRec.head.ID = rootID ;
       if( (rc = md2GetRecord( mdl , &rootRec , 0 )) )goto abort ;
       rootRec.head.nextID = newID ;
       if( (rc = md2PutRecord( mdl , &rootRec , 0 )) )goto abort ;
    }else{
      if( (rc = md2AddToDirectory( mdl , dirID , name , newID )) )goto abort ;
    }
   if( resID )*resID = newID ;
 if(md2CommitLock( mdl ))return MDEnoCommit ;
 return 0 ;
 
abort :

 md2AbortLock( mdl ) ;
 return rc ;  

}
int md2MakeFile( MDL * mdl , md_id_t dirID , char *name , md_id_t *resID )
{
 md_id_t  newID ;
 int      rc ;
 char      tagName[MD_MAX_NAME_LENGTH] ;
 
 if(md2WriteLock( mdl ) )return MDEnoLock ;
    if( md2GetTagName( name , tagName ) ){
      if( (rc = md2MakeNewFile( mdl , dirID , &newID ))  )goto abort ;
 
      if( (rc = md2AddToDirectory( mdl , dirID , name , newID )) )goto abort ;
    }else{
       mdRecord tag;
       if( (rc = md2AddNewTag( mdl , dirID , tagName , &tag )) )goto abort ;
       newID = tag.head.ID ;
    }
    if( resID )*resID = newID ;
 if(md2CommitLock( mdl ))return MDEnoCommit ;
 return 0 ;
 
abort :

 md2AbortLock( mdl ) ;
 return rc ;  

}
int md2UpdateDirTime( MDL * mdl , md_id_t dirID ){
 int rc = 0 ;
 mdRecord dir ;
 if(md2WriteLock( mdl ) )return MDEnoLock ;
 
    dir.head.ID = dirID ;
    if( (rc = md2GetRecord( mdl , &dir , 0 )) )goto abort ;
    time( (time_t*)&dir.body.dirInode.attr.unixAttr.mst_atime ) ;
    dir.body.dirInode.attr.unixAttr.mst_mtime =
      dir.body.dirInode.attr.unixAttr.mst_atime ;
    if( (rc = md2PutRecord( mdl , &dir , 0 )) )goto abort ;
 
 if(md2CommitLock( mdl ))return MDEnoCommit ;
 return 0 ;
abort :

 md2AbortLock( mdl ) ;
 return rc ;  

}
int md2MakeFilePerm( MDL * mdl  , md_id_t dirID  , md_permission  perm ,
                     char *name , md_id_t *resID , md_permission *resPerm)
{
 md_id_t   newID ;
 int       rc , typ ;
 mdRecord  tag;
 md_object  *object ;
 md_permission defPerm ;
 char *c ;
 
  resPerm = resPerm == NULL ? &defPerm : resPerm ;
 *resPerm = perm ;
 
 if(md2WriteLock( mdl ) )return MDEnoLock ;

 if( (object = md2IsObject( name  )) ){
      /*
      * this is the part where we handle the spooky namespace
      *
      md2pPrintf(md2pMaxLevel," md2MakeFilePerm : %s is object %d\n" ,
                              name , md2ObjectType( object ) ) ;
      */
      if( ( typ = md2ObjectType( object ) ) == MDO_TAG ){
	if( (rc = md2AddNewTag( mdl , dirID , md2ObjectName( object ) , &tag )) )goto abort ;
          newID = tag.head.ID ;
      }else if( typ ==  MDO_REQ_SET ){
         /* 
             create  .(set)(<filename>)(<commmand>)( ...) 
          */
         md_dir_item  item;
         mdRecord     mdr ;
         unsigned long long int size ;
         int          level = mdGetLevel(perm);
         if( object -> argc < 2 ){ rc = MDEnotFound ; goto abort ; }
         /*
         * operations are only allowed on existing objects
         */
         if( (rc = mdFindDirectoryEntry(mdl ,dirID ,object->argv[0] , &item )))goto abort ;
         mdr.head.ID = item.ID ;
         if( (rc = md2GetRecord( mdl , &mdr , 0 )) )goto abort ;
         if( ! strcmp( object -> argv[1] , "size" ) ){
            if( object -> argc < 3 ){ rc = MDEnotFound ; goto abort ; }
            if( ( level == 0 ) &&
             /* ( mdpIsNoIO(perm) ) &&  */
                ( ! mdIsType( mdr.head.type , mdtForceIO ) ) ){
             
                   sscanf( object->argv[2] , "%lld" , &size );
                   mdr.body.fileInode.attr[0].unixAttr.mst_size = size ;
		   mdr.body.fileInode.attr[0].unixAttr.mst_sizeHigh = size >> 32 ;
		   if( (rc = md2PutRecord( mdl , &mdr , 0 )) )goto abort;
                   
                   newID = item.ID ;
                   mdSetSpecialID( newID );
                   mdpSetSpecial( *resPerm , MDO_REQ_SET_SIZE );
                   
             }else{
                   rc = MDEnotFound ;
                   goto abort ;
             }
             /*
                update the modification time of the
                directory to force a getattr.
                We ignore the return code.
             */
             (void)md2UpdateDirTime( mdl , dirID );

         }else if( ! strcmp( object->argv[1] , "io" ) ){
             if( ( mdr.body.fileInode.attr[0].entries != 0 ) &&
                 ( mdIsNullID( mdr.body.fileInode.attr[0].chain ) ) ) {
                rc = MDEnotEmpty ;
                goto abort ;
             }
             mdAddType( mdr.head.type , mdtForceIO );
             /*mdRmType( mdr.head.type , mdtForceIO );*/
             if( (rc = md2PutRecord( mdl , &mdr , 0 )) )goto abort ;
             newID = item.ID ;
             mdSetSpecialID( newID );
             mdpSetSpecial( *resPerm , MDO_REQ_SET_SIZE );
             (void)md2UpdateDirTime( mdl , dirID );
         }else{
                rc = MDEnotFound ;
                goto abort ;
         }   
      }else if( typ ==  MDO_REQ_FORCEIO ){
         c = md2ObjectName(object) ;
         if( object -> argc == 1 ){
            md2ScanId( c , &newID ) ;
            tag.head.ID = newID ;
            if( (rc = md2GetRecord( mdl , &tag , 0 )) )goto abort ;
            mdAddType( tag.head.type , mdtForceIO ) ;  
            if( (rc = md2PutRecord( mdl , &tag , 0 )) )goto abort ;
         }else{
            rc = MDEnotFound ;
            goto abort ;
         }
      }else if( typ ==  MDO_REQ_FORCEIO2 ){
         md_dir_item thisItem ;
         
         c = md2ObjectName(object) ;
         if( (rc = mdFindDirectoryEntry(mdl ,dirID ,c ,&thisItem )))goto abort ;
         newID = tag.head.ID = thisItem.ID ;
         if( (rc = md2GetRecord( mdl , &tag , 0 )) )goto abort ;
         mdAddType( tag.head.type , mdtForceIO ) ;  
         if( (rc = md2PutRecord( mdl , &tag , 0 )) )goto abort ;
      }else{
         rc = MDEnotFound ;
         goto abort ;
      }
    }else{
       /*
        * regular name space
        */ 

#ifdef no_level0_create  
       if( mdpIsNoIO( perm ) ){
          rc = MDEnotAllowed ;
          goto abort ;
       }else
#endif	 
       {  
	 if( (rc = md2MakeNewFile( mdl , dirID , &newID ))  )goto abort ; 
	 if( (rc = md2AddToDirectory( mdl , dirID , name , newID )) )goto abort ;
       }
    }


    if( resID )*resID = newID ;
 if(md2CommitLock( mdl ))return MDEnoCommit ;
 return 0 ;
 
abort :

 md2AbortLock( mdl ) ;
 return rc ;  

}
int md2MakeLink( MDL * mdl , md_id_t dirID , char *name ,
                 int level , char *path ,  md_id_t *resID )
{
 md_id_t  newID ;
 int      rc , rc2 ;
 
 if(md2WriteLock( mdl ) )return MDEnoLock ;
    rc = md2Lookup( mdl , dirID , name , &newID ) ;
    if( rc == MDEnotFound  ){
      if( (rc2 = md2MakeNewLink( mdl , dirID ,  &newID )) ){
          rc = rc2 ;
          goto abort ;
       }
      if( (rc2 = md2AddToDirectory( mdl , dirID , name , newID )) ){ 
          rc = rc2 ; 
          goto abort ; 
       }    
    }else if( rc ){
       goto abort ;
    }
    if( (rc = md2WriteLink( mdl , newID , level , path )) ){
       goto abort ;
    }
    if( resID )*resID = newID ;
 if(md2CommitLock( mdl ))return MDEnoCommit ;
 return 0 ;
 
abort :

 md2AbortLock( mdl ) ;
 return rc ;  

}
int md2ReadLink( MDL * mdl , md_id_t linkID , int level , char *path )
{
 mdRecord inode , data ;
 int rc ;
 md_id_t id ;
 
 if(md2ReadLock( mdl ) )return MDEnoLock ;
 
   inode.head.ID = linkID ;
   if( (rc = md2GetRecord( mdl , &inode , 0 )) )goto abort;
   if( mdIsNullID( id = inode.body.fileInode.attr[level].chain ) )
      return MDEnotFound ;
   data.head.ID = id ;
   if( (rc = md2GetRecord( mdl , &data , 0 )) )goto abort ;
   strcpy( path ,  data.body.linkData.data ) ;
   
 
 if(md2CommitLock( mdl ))return MDEnoCommit ;
 return 0 ;
 
abort :

 md2AbortLock( mdl ) ;
 return rc ;  


}
int md2WriteLink( MDL * mdl , md_id_t linkID , int level , char *path )
{
 int rc , len ;
 md_id_t id ;
 mdRecord inode , data ;
 
 if(md2WriteLock( mdl ) )return MDEnoLock ;
 
   inode.head.ID = linkID ;
   if( (rc = md2GetRecord( mdl , &inode , 0 )) )goto abort;
   if( mdIsNullID( id = inode.body.fileInode.attr[level].chain ) ){
     if( (rc = md2GetNewRecord( mdl , &data )) )goto abort ;
     mdAddType( data.head.type , mdtLink | mdtData) ;
     inode.body.fileInode.attr[level].chain = data.head.ID ;
   }else{
       data.head.ID = id ;
       if( (rc = md2GetRecord( mdl , &data , 0 )) )goto abort ;
   }
   len = strlen( path ) ;
   if( ( len + 4 ) > DATA_UNITS ){ rc = MDEnoSpace ; goto abort ; }
   strcpy( data.body.linkData.data , path ) ;
   inode.body.fileInode.attr[level].entries            = len ;
   inode.body.fileInode.attr[level].unixAttr.mst_size  = len ;
   inode.body.fileInode.attr[level].unixAttr.mst_sizeHigh  = 0 ;
   if( (rc = md2PutRecord( mdl , &data  , 0 )) )goto abort ;
   if( (rc = md2PutRecord( mdl , &inode , 0 )) )goto abort ;
   
 if(md2CommitLock( mdl ))return MDEnoCommit ;
 return 0 ;
 
abort :

 md2AbortLock( mdl ) ;
 return rc ;  

}

int md2ModPermPtr( MDL *mdl, md_dir_item *item , void *data )
{
  item -> perm = *( (md_permission *)data ) ;
  return 0 ;
}
int md2SetEntryOnlyPtr( MDL *mdl, md_dir_item *item , void *data )
{
  mdpSetEntryOnly( item -> perm ) ;
  return 0 ;
}
int md2ModExpirePtr( MDL *mdl, md_dir_item *item , void *data )
{
  item -> expire = *( (md_time_t *)data ) ;
  return 0 ;
}
int md2ModWhatever( MDL * mdl , md_id_t dirID , char *name , modDirItemFunc func , void * data )
{
 mdRecord  dir , hash , mdr ;
 int       rc , column , fnd , i ; 
 md_id_t   xid ;
 
 if(md2ReadLock( mdl ) )return MDEnoLock ;
    dir.head.ID = dirID ;
    rc = md2GetDirectoryInfos( mdl , &dir , &hash , name );
    if( rc < 0 )goto abort ;
    column = rc ;
    if( mdIsNullID( hash.body.dirHash.hashPointer[column] ) ){
       rc = MDEnotFound ;
       goto abort ;
    }
    mdr.head.ID = hash.body.dirHash.hashPointer[column] ;
    for( fnd = 0 , xid = mdr.head.ID ; ! mdIsNullID( xid ) ; xid = mdr.head.nextID ){
       mdr.head.ID = xid ;
       if( (rc = md2GetRecord( mdl , &mdr , 0 )) )goto abort ;
       for( i = 0 ; i < mdr.body.dirData.dirHead.entries ; i++ ){
         if( ( ! mdIsNullID( mdr.body.dirData.dirItem[i].ID      ) ) &&
             ( ! strcmp( mdr.body.dirData.dirItem[i].name , name ) )   ){
           fnd = 1 ;
	   (*func)( mdl , &mdr.body.dirData.dirItem[i] , data ) ;
	   if( (rc = md2PutRecord( mdl , &mdr , 0 )) )goto abort ;
           break ;
         }
       }
       if( fnd )break ;
    }
    if( ! fnd ){
       rc = MDEnotFound ;
       goto abort ;
    }
 if(md2CommitLock( mdl ))return MDEnoCommit ;
 return 0 ;
 
abort :

 md2AbortLock( mdl ) ;
 return rc ;  

}
#ifdef modPermissionFunction
int md2ModPermission( MDL * mdl , md_id_t dirID , char *name , md_permission perm )
{
 mdRecord  dir , hash , mdr ;
 int       rc , column , fnd , i ;
 md_id_t   xid ;
 
 if(md2ReadLock( mdl ) )return MDEnoLock ;
    dir.head.ID = dirID ;
    rc = md2GetDirectoryInfos( mdl , &dir , &hash , name );
    if( rc < 0 )goto abort ;
    column = rc ;
    if( mdIsNullID( hash.body.dirHash.hashPointer[column] ) ){
       rc = MDEnotFound ;
       goto abort ;
    }
    mdr.head.ID = hash.body.dirHash.hashPointer[column] ;
    for( fnd = 0 , xid = mdr.head.ID ; ! mdIsNullID( xid ) ; xid = mdr.head.nextID ){
       mdr.head.ID = xid ;
       if( rc = md2GetRecord( mdl , &mdr , 0 ) )goto abort ;
       for( i = 0 ; i < mdr.body.dirData.dirHead.entries ; i++ ){
         if( ( ! mdIsNullID( mdr.body.dirData.dirItem[i].ID      ) ) &&
             ( ! strcmp( mdr.body.dirData.dirItem[i].name , name ) )   ){
           fnd = 1 ;
		   mdr.body.dirData.dirItem[i].perm = perm ;
           if( rc = md2PutRecord( mdl , &mdr , 0 ) )goto abort ;
           break ;
         }
       }
       if( fnd )break ;
    }
    if( ! fnd ){
       rc = MDEnotFound ;
       goto abort ;
    }
 if(md2CommitLock( mdl ))return MDEnoCommit ;
 return 0 ;
 
abort :

 md2AbortLock( mdl ) ;
 return rc ;  

}
#endif
int md2ExtLookup( MDL * mdl , md_id_t dirID , char *name ,
                  md_id_t *resID , md_dir_item *item         )
{
 return  md2ExtLookupPerm( mdl , dirID , NULL ,name , resID ,item ) ;           
}
int md2ExtLookupPerm( MDL * mdl , md_id_t dirID , md_permission *perm , 
                      char *name , md_id_t *resIDOut , md_dir_item *itemOut )
{
 mdRecord     mdr ;
 int          rc , isSpecial , oType , i ;
 md_object   *object ;
 md_dir_item *item , localItem ;
 md_id_t     *resID , localResID ;
 
 resID = resIDOut == NULL ? &localResID : resIDOut ;
 item  = itemOut  == NULL ? &localItem  : itemOut ;
 
 if(md2ReadLock( mdl ) )return MDEnoLock ;
 
    isSpecial = 0 ;
    if( ( object = md2IsObject( name  ) ) != NULL ){

      oType =  md2ObjectType( object ) ;
      /*
      md2pPrintf( md2pINFO , "Lookup Object : %s -> type %d\n", name , oType ) ;
      */

      if( oType == MDO_TAG ){
      /*  ----------------    */
	if( (rc = md2FindTag( mdl , dirID , md2ObjectName( object ) , &mdr )) )goto abort ;
         memset( (char*)item , 0 , sizeof( md_dir_item ) ) ;
         strcpy( item->name , name ) ;
         item->ID = mdr.head.ID ;         
         goto ok ;
      }else if( (   oType == MDO_REQ_LSTAGS ) ||
                (   oType == MDO_REQ_LSXTAGS) || 
                (   oType == MDO_REQ_CONST  )     ){ 
      /*      ------------------------------------    */
          if(!perm){ rc = MDEnotAllowed ; goto abort ; }
          memset( (char*)item , 0 , sizeof(md_dir_item ) ) ;
          if( ( object -> argc > 0 ) && 
              ( strlen( object -> argv[0] ) > 4 ) ){
            md2ScanId(  object -> argv[0] , &(item->ID) ) ;
          }else{
             item->ID   = dirID ;
          }
          item->perm = *perm ;
          mdpSetSpecial(item->perm,oType);
          mdSetSpecialID(item->ID);
          goto ok ;         
      }else if(   oType == MDO_REQ_GET  ){ 
      /*      ------------------------------------    */
          if(!perm){ rc = MDEnotAllowed ; goto abort ; }
          memset( (char*)item , 0 , sizeof(md_dir_item ) ) ;
          if( object->argc < 1 ){rc = MDEnotFound ; goto abort ; }
          if( ! strcmp( object -> argv[0] , "counters" ) ){
             item->perm = *perm ;
             mdpSetSpecial(item->perm,MDO_REQ_GET_COUNTERS);
             memset( ( char *) &( item->ID) , 0 , sizeof( item->ID) ) ;
             if( object -> argc > 1 ){ 
                int db ;               
                sscanf( object->argv[1] , "%d" , &db ) ;
                item->ID.db = db ;
             }else{
                item->ID.db = dirID.db ;
             }
             mdSetSpecialID(item->ID);
          }else if( ! strcmp( object -> argv[0] , "database" ) ){
             item->perm = *perm ;
             mdpSetSpecial(item->perm,MDO_REQ_GET_DATABASE );
             memset( ( char *) &( item->ID) , 0 , sizeof( item->ID) ) ;
             if( object -> argc > 1 ){ 
                int db ;               
                sscanf( object->argv[1] , "%d" , &db ) ;
                item->ID.db  = 0 ;
                item->ID.low = db << 3;
             }else{
                item->ID.db  = 0 ;
                item->ID.low = dirID.db << 3 ;
             }
             mdSetSpecialID(item->ID);
          }else{
             item->ID   = dirID ;
             item->perm = *perm ;
             mdpSetSpecial(item->perm,oType);
             mdSetSpecialID(item->ID);
          }
          goto ok ;         
      }else if( oType == MDO_REQ_CONFIG ){ 
      /*        ------------------------    */
         char *cur ;
         md_id_t configID ;
         /* 
            I'm using the item.ID part to return the
            configID of this particular database.
            and the name to hold the split up namepart
            of the .(config)( ...   namespace entry.
            This should not be a problem because
            the resulting name is always shorter then
            the source string.
         */
         if( (rc = md2GetRootConfig( mdl , &configID )) )goto abort ;
         if( mdIsNullID( configID ) ){ rc = MDEnotFound ; goto abort ; }
         memset( (char*)item , 0 , sizeof(md_dir_item ) ) ;
         item->ID   = configID ;
         
         for( i = 0 , cur = name ; i < object -> argc ; i++  ){
            strcpy( cur , object -> argv[i] ) ;
            cur += ( strlen( object -> argv[i] ) + 1 ) ;
         }
         *cur = '\0' ;

         rc = MDEdbXsearch ;
         goto abort ; 

      }else if( oType == MDO_REQ_JUMP ){ 
      /*        ----------------------    */
         char *levelName ;
         if(!perm){ rc = MDEnotAllowed ; goto abort ; }
         
            levelName = md2ObjectName(object) ;
            memset( (char*)item , 0 , sizeof(md_dir_item ) ) ;
            item->ID   = dirID ;
            item->perm = *perm ;
            strcpy(item->name,md2ObjectName(object));
            sscanf( levelName , "%d" , &i ) ;
            if((i<0)||(i>7)){ rc = MDEnotFound ; goto abort ; }
            mdpSetLevel(item->perm,i) ;
            mdpModifyLevel(item->perm)   ;
            mdpUnsetNoIO(item->perm)  ;
            mdpModifyNoIO(item->perm) ; 
       
         goto ok ;         
      }else if( oType == MDO_REQ_ACCESS  )   {
      /*        -------------------------    */
         char *c;
         md_id_t id ;
         c = md2ObjectName(object) ;
         if( object -> argc == 1 ){
            md2ScanId( c , &id ) ;
            i = 0 ; 
         }else if( object -> argc == 2 ){
            md2ScanId( c , &id ) ;
            sscanf(object->argv[1],"%d",&i);
         }
      
         memset( (char*)item , 0 , sizeof(md_dir_item ) ) ;
         item->ID = id  ;
         mdpSetLevel(item->perm,i) ;
         mdpModifyLevel(item->perm)   ;
         mdpSetSpecial(item->perm,MDO_REQ_ACCESS);
         
         goto ok ;         
      }else if( oType == MDO_REQ_LEVEL ){ 
      /*        -----------------------    */
         char *levelName ;
         if( object -> argc != 2 ){ rc = MDEnotFound ; goto abort ; }
         name      = object -> argv[1]  ;
         levelName = object->argv[0] ;
         if(      !  strcmp( levelName , "-osm-" )  )i = 1 ;
         else if( ! strcmp( levelName  , "martin" ) )i = 2 ;
         else if( ! strcmp( levelName  , "suse" )   )i = 3 ;
         else if( ! strcmp( levelName  , "meta" )   )i = 6 ;
         else if( ( strlen( levelName ) == 1 ) && 
                  ( *levelName >= '0' ) &&
                  ( *levelName <= '9' )             )
                      sscanf( levelName , "%d" , &i );
                      
         else  {   rc = MDEnotFound ; goto abort ; };
         
         if( (rc = mdFindDirectoryEntry(mdl ,dirID ,name , item )))goto abort ;
         mdpSetLevel(item->perm,i);
         mdpModifyLevel(item->perm)   ;
         if( i ){
             mdpModifyNoIO(item->perm) ;
             mdpUnsetNoIO(item->perm) ;
         }
      

      }else if( oType == MDO_REQ_ID ){ 
      /*        ---------------------    */
         name = md2ObjectName( object ) ;
         isSpecial = MDO_REQ_ID ;
         if( (rc = mdFindDirectoryEntry(mdl ,dirID ,name , item )))goto abort ;
         mdpSetSpecial(item->perm,MDO_REQ_ID);
         mdSetSpecialID(item->ID);
         
      }else if( oType == MDO_REQ_FORCEIO2 ){ 
      /*        ---------------------------    */
         name = md2ObjectName( object ) ;
         if( (rc = mdFindDirectoryEntry(mdl ,dirID ,name , item )))goto abort ;
         mdr.head.ID = item->ID ;
         if( (rc = md2GetRecord( mdl , &mdr , 0 )) )goto abort ;
         if( ! mdIsType( mdr.head.type , mdtForceIO ) ){
            rc = MDEnotFound ;
            goto abort ;
         }
      }else if( oType == MDO_REQ_PSET ){ 
      /*        ----------------------    */
      /* touch .(pset)(<pnfsid>)(<commmand>)( ...) */
         char *c;
         md_id_t id ;
         if( object -> argc < 5 ){ rc = MDEnotFound ; goto abort ; }
         
         c = md2ObjectName(object) ;
         if( object -> argc == 1 ){
            md2ScanId( c , &id ) ;
         }
               
         memset( (char*)item , 0 , sizeof(md_dir_item ) ) ;
         item->ID = id  ;
         mdpSetLevel(item->perm,0) ;
         mdpModifyLevel(item->perm)   ;
         mdpSetSpecial(item->perm,MDO_REQ_PSET);
      
      
      }else if( oType == MDO_REQ_SET ){ 
      /*        ----------------------    */
      /* touch .(fset)(<filename>)(<commmand>)( ...) */
      
         if( object -> argc < 2 ){ rc = MDEnotFound ; goto abort ; }
         if( (rc = mdFindDirectoryEntry(mdl ,dirID ,object->argv[0] , item )))goto abort ;
         mdr.head.ID = item->ID ;
         if( (rc = md2GetRecord( mdl , &mdr , 0 )) )goto abort ;
         if( ! strcmp( object->argv[1] , "size" ) ){
             if( perm != NULL ){
                if( ( mdGetLevel(*perm) != 0 ) ||
                /*  ( ! mdpIsNoIO(*perm)     ) ||  */
                    (   mdIsType( mdr.head.type , mdtForceIO ) ) ){
                    rc = MDEnotFound ; goto abort ;
                }
             }
             if( ( ! mdIsType( mdr.head.type , mdtInode )   ) ||
                 ( ! mdIsType( mdr.head.type , mdtRegular ) )     ){
                  rc = MDEnotFound ;
                  goto abort ;
             }
             /*
              * enforce creation of file
              */
             if( mdr.body.fileInode.attr[0].unixAttr.mst_size == 0 ){
                rc = MDEnotFound ;
                goto abort ;
             }
         }else if( ! strcmp( object->argv[1] , "io" ) ){
            if( ! mdIsType( mdr.head.type , mdtForceIO ) ){
               rc = MDEnotFound ;
               goto abort ;
            }
         }else{
            rc = MDEnotFound ;
            goto abort ;
         }
      }else if( oType == MDO_REQ_FSET ){ 
      /*        ----------------------    */
      /* touch .(fset)(<filename>)(<commmand>)( ...) */
      /*
       * this code is able to do very nasty things just by
       * doing an 'ls -l".
       */
         /* disable this part of the code */
         if( object -> argc != 1000 ){ rc = MDEnotFound ; goto abort ; }
      /* */
         if( object -> argc < 2 ){ rc = MDEnotFound ; goto abort ; }
         if( (rc = mdFindDirectoryEntry(mdl ,dirID ,object->argv[0] , item )))goto abort ;
         mdr.head.ID = item->ID ;
         if( (rc = md2GetRecord( mdl , &mdr , 0 )) )goto abort ;
         if( ! strcmp( object->argv[1] , "group" ) ){
             if( object -> argc < 3 ){ rc = MDEnotFound ; goto abort ; }
             if( mdIsType( mdr.head.type , mdtRegular ) ){
                sscanf(object->argv[2],"%ld",&mdr.body.fileInode.attr[0].group);
             }else if( mdIsType( mdr.head.type , mdtDirectory ) ){
                sscanf(object->argv[2],"%ld",&mdr.body.dirInode.attr.group);
             }else{
                rc = MDEnotFound ;
                goto abort ;
             }
             if( (rc = md2PutRecord( mdl , &mdr , 0 )) )goto abort ;
         }else if( ! strcmp( object->argv[1] , "io" ) ){
             if( object -> argc < 3 ){ rc = MDEnotFound ; goto abort ; }
             if( ( mdr.body.fileInode.attr[0].entries != 0 ) &&
                 ( mdIsNullID( mdr.body.fileInode.attr[0].chain ) ) ) {
                rc = MDEnotEmpty ;
                goto abort ;
             }
             if( ! strcmp( object->argv[2] , "on" ) ){
                 mdAddType( mdr.head.type , mdtForceIO );
             }else if( ! strcmp( object->argv[2] , "off" ) ){
                 mdRmType( mdr.head.type , mdtForceIO );
             }else{
                rc = MDEnotFound ;
                goto abort ;
             }
             if( (rc = md2PutRecord( mdl , &mdr , 0 )) )goto abort ;
         }else if( ! strcmp( object->argv[1] , "permission" ) ){
             if( object -> argc < 3 ){ rc = MDEnotFound ; goto abort ; }
             md2ScanPermission( object->argv[2] , &item->perm );
             if( (rc = mdUpdateDirectoryEntry( mdl , dirID , object->argv[0] ,item)))
                   goto  abort;
         }else if( ! strcmp( object->argv[1] , "size" ) ){
             long size ;
             int  level = mdGetLevel(*perm);
             if( object -> argc < 3 ){ rc = MDEnotFound ; goto abort ; }
             if( ( level ==0 ) &&
                 ( mdpIsNoIO(*perm) ) &&
                 ( ! mdIsType( mdr.head.type , mdtForceIO ) ) ){
                sscanf( object->argv[2] , "%ld" , &size );
                if( (rc = md2ForceSize(mdl,item->ID,0,size )))goto abort;
             }else{
                rc = MDEnotFound ;
                goto abort ;
             }
         }else{
            rc = MDEnotFound ;
            goto abort ;
         }
      }else if(   ( oType == MDO_REQ_SHOWID )  ||
                  ( oType == MDO_REQ_GETATTR)  ||  
                  ( oType == MDO_REQ_NAME   )  ||  
                  ( oType == MDO_REQ_NAMEOF )  ||  
                  ( oType == MDO_REQ_PARENT )      ){ 
      /*      -----------------------------------------    */
         char *c;
         md_id_t id ;
         c = md2ObjectName(object) ;
         md2ScanId( c , &id ) ;
         mdSetSpecialID(id);
         memset( (char*)item , 0 , sizeof(md_dir_item ) ) ;
         item->ID = id  ;
         mdpSetSpecial(item->perm,oType);
     
      }else if( oType == MDO_REQ_LIST ){ 
      /*      -----------------------------------------    */
         char *c;
         int cookie ;
         md_id_t id ;
         if( object -> argc < 2 ){ rc = MDEnotFound ; goto abort ; }
         c = md2ObjectName(object) ;
         md2ScanId( c , &id ) ;
         sscanf( object->argv[1] , "%d" , &cookie ) ;
         mdSetSpecialID(id);
         memset( (char*)item , 0 , sizeof(md_dir_item ) ) ;
         item->ID = id  ;
         mdpSetSpecial(item->perm,oType);
         mdpSetArgs(item->perm,cookie) ;
         md2pPrintf( md2pINFO , 
                     "Cookie value : %s\n", 
                     mdStringPermission(item->perm) ) ;
     
      }else{
         rc = MDEnotFound ;
         goto abort ;
      }
    }else{   /* normal name space   */
       /*       ----------------    */
       /* 
        * this part handles the ordinary name space
        */
      if( (rc = mdFindDirectoryEntry(mdl ,dirID ,name , item )))goto abort ;
    }
    
ok:

 if(md2CommitLock( mdl ))return MDEnoCommit ;
 *resID = item -> ID ;
 return 0 ;
 
abort :

 md2AbortLock( mdl ) ;
 *resID = item -> ID ; /* is needed for database cross lookups */
 return rc ;  

}
int mdDirectoryTree( MDL * mdl , md_id_t dirID , void *glbl , md_dir_item *item )
{
  int rc ;
  mdRecord dir ;
  static int depth ;
  static char zeros[128] ;
  
   if( item == NULL ){
     depth = 0 ;
     if( (rc = mdDirectoryWalk( mdl , dirID , mdDirectoryTree , glbl )) )return rc ;
     return 0 ;
   }
  
   dir.head.ID = item -> ID ;
   if( (rc = md2GetRecord( mdl , &dir , 0 )) )return 0 ; 
             /* otherwise we would loose the rest of the directory */
   if( ! ( mdIsType( dir.head.type, mdtInode     )  &&
           mdIsType( dir.head.type, mdtDirectory )     )  )return 0 ; 

   rc = depth * 3 ;
   rc = rc>127?127:rc ;
   memset( zeros , ' ' , rc ) ;
   zeros[rc] = '\0' ;
  
  depth ++ ;
  if( (rc = mdDirectoryWalk( mdl , item->ID , mdDirectoryTree , glbl )) )return rc ;
  depth -- ;
  
  return 0 ;
}
int mdDirectoryWalk( MDL * mdl , md_id_t dirID , md_filter_func filter , void *glbl )
{
 mdRecord  dir , hash , data ;
 int       rc , i , j , k , fnd = 0 ;
 md_id_t   id ;
 
 if(md2WriteLock( mdl ) )return MDEnoLock ;
 
   dir.head.ID = dirID ;
   if( (rc = md2GetRecord( mdl , &dir , 0 )) )goto abort ;
   if( ! ( mdIsType( dir.head.type, mdtInode     )  &&
           mdIsType( dir.head.type, mdtDirectory )     )  ){ 
         rc = MDEnotDirectory ;
         goto abort ;
   }
   if( dir.body.dirInode.attr.entries <= 0 )goto theEnd ;

   for( i = 0 , fnd = 0  ; i < dir.body.dirInode.hashInfo.rows ; i++ ){
        hash.head.ID = dir.body.dirInode.hashHandle[i] ;
        if( (rc = md2GetRecord( mdl , &hash , 0 )) )goto abort ;  
        for( j = 0 ; j < hash.body.dirHash.hashHead.entries ; j++ ){
        
           if( mdIsNullID( hash.body.dirHash.hashPointer[j] ) )continue ;
           
           for( id = hash.body.dirHash.hashPointer[j] ;
                ! mdIsNullID(id) ; 
                id = data.head.nextID  ){
                
                data.head.ID = id ;
                if( (rc = md2GetRecord( mdl , &data , 0 )) )goto abort ;
                for( k = 0 ; k < data.body.dirData.dirHead.entries ; k++ ){
                  if( ! mdIsNullID( data.body.dirData.dirItem[k].ID)  ){
                    fnd = (*filter)(mdl,dirID,glbl,&data.body.dirData.dirItem[k]) ;
                    if( fnd ) goto theEnd ;
                  }
                }
           }
        }
   }
theEnd :
 
 if(md2CommitLock( mdl ))return MDEnoCommit ;
 return fnd ;

abort :

 md2AbortLock( mdl ) ;
 return rc ;  

}
int mdFindDirectoryEntry( MDL * mdl , md_id_t dirID , char *name , md_dir_item *item
)
{
   mdRecord dir , hash , mdr ;
   int rc , column , fnd ,i ;
   md_id_t xid ;
   
  if(md2ReadLock( mdl ) )return MDEnoLock ;
  
    dir.head.ID = dirID ;
    rc = md2GetDirectoryInfos( mdl , &dir , &hash , name );
    if( rc < 0 )goto abort ;
    column = rc ;
    if( mdIsNullID( hash.body.dirHash.hashPointer[column] ) ){
       rc = MDEnotFound ;
       goto abort ;
    }
    mdr.head.ID = hash.body.dirHash.hashPointer[column] ;
    for( fnd = 0 , xid = mdr.head.ID ; ! mdIsNullID( xid ) ; xid = mdr.head.nextID ){
       mdr.head.ID = xid ;
       if( (rc = md2GetRecord( mdl , &mdr , 0 )) )goto abort ;
       for( i = 0 ; i < mdr.body.dirData.dirHead.entries ; i++ ){
         if( ( ! mdIsNullID( mdr.body.dirData.dirItem[i].ID      ) ) &&
             ( ! strcmp( mdr.body.dirData.dirItem[i].name , name ) )   ){
           fnd = 1 ;
           break ;
         }
       }
       if( fnd )break ;
    }
    if( ! fnd ){
       rc = MDEnotFound ;
       goto abort ;
    }

    if( item )memcpy( item , &mdr.body.dirData.dirItem[i] , sizeof(*item) );
 
 if(md2CommitLock( mdl ))return MDEnoCommit ;
 return 0 ;
 
abort :

 md2AbortLock( mdl ) ;
 return rc ;  


}
int mdUpdateDirectoryEntry( MDL * mdl , md_id_t dirID , char *name , md_dir_item *item )
{
   mdRecord dir , hash , mdr ;
   int rc , column , fnd ,i ;
   md_id_t xid ;
   
  if(md2ReadLock( mdl ) )return MDEnoLock ;
  
    dir.head.ID = dirID ;
    rc = md2GetDirectoryInfos( mdl , &dir , &hash , name );
    if( rc < 0 )goto abort ;
    column = rc ;
    if( mdIsNullID( hash.body.dirHash.hashPointer[column] ) ){
       rc = MDEnotFound ;
       goto abort ;
    }
    mdr.head.ID = hash.body.dirHash.hashPointer[column] ;
    for( fnd = 0 , xid = mdr.head.ID ; ! mdIsNullID( xid ) ; xid = mdr.head.nextID ){
       mdr.head.ID = xid ;
       if( (rc = md2GetRecord( mdl , &mdr , 0 )) )goto abort ;
       for( i = 0 ; i < mdr.body.dirData.dirHead.entries ; i++ ){
         if( ( ! mdIsNullID( mdr.body.dirData.dirItem[i].ID      ) ) &&
             ( ! strcmp( mdr.body.dirData.dirItem[i].name , name ) )   ){
           fnd = 1 ;
           break ;
         }
       }
       if( fnd )break ;
    }
    if( ! fnd ){
       rc = MDEnotFound ;
       goto abort ;
    }

    memcpy( &mdr.body.dirData.dirItem[i] , item , sizeof(*item) );
    if( (rc = md2PutRecord( mdl , &mdr , 0 )) )goto abort ;
 
 if(md2CommitLock( mdl ))return MDEnoCommit ;
 return 0 ;
 
abort :

 md2AbortLock( mdl ) ;
 return rc ;  


}
int md2ExtRemoveFromDirectoryPosition( MDL * mdl , md_id_t dirID , md_id_t rmDir ,
                               int position  )
{
 int rc = 0 , i ;
 mdRecord  dirData;
 if(md2WriteLock( mdl ) )return MDEnoLock ;
   dirData.head.ID = dirID ;
   if( (rc = md2GetRecord( mdl , &dirData , 0 )) )goto abort ;
   
   if( dirData.body.dirInode.attr.flags & MD_INODE_FLAG_NOREMOVE ){
      rc = MDEnotAllowed ;
      goto abort ;
   }
     
   if( position >= dirData.body.dirData.dirHead.maxEntries ){ rc = MDEnotFound ; goto abort ; }
   if( ! mdIsEqualID( dirData.body.dirData.dirItem[position].ID  , rmDir ) ){ rc = MDEnotFound ; goto abort ; }

    mdSetNullID( dirData.body.dirData.dirItem[position].ID ) ;
    /*
     * adjust the high water mark ( entries )
     * looks funny but works ( don't ask why )
     */
    for( i = (dirData.body.dirData.dirHead.entries - 1) ;
         ( i >= 0 ) && mdIsNullID( dirData.body.dirData.dirItem[i].ID ) ;
         i-- )
       dirData.body.dirData.dirHead.entries-- ;
    /*
     * removing of unused records 
     */
    if( dirData.body.dirData.dirHead.entries > 0 ){
      if( (rc = md2PutRecord( mdl , &dirData , 0 )) )goto abort ;
    }else{
       /*
        * for simplicity we don't remove the record if there are no
        * entries left. (this is low level, so not too heavly used)
        * The next entry will be added here.
        */
      if( (rc = md2PutRecord( mdl , &dirData , 0 )) )goto abort ;
    }

 if(md2CommitLock( mdl ))return MDEnoCommit ;
 return 0 ;
 
abort :

 md2AbortLock( mdl ) ;
 return rc ;  
 
}
int md2ExtRemoveFromDirectory( MDL * mdl , md_id_t dirID , char *name ,
                               md_id_t *resID , md_dir_item *item )
{
 mdRecord  dir , hash , mdr , lastMdr  ;
 int       rc , column , fnd , i , position ;
 md_id_t   xid  ;
 md_dir_item xitem ;
 
 if(md2WriteLock( mdl ) )return MDEnoLock ;
    dir.head.ID = dirID ;
    rc = md2GetDirectoryInfos( mdl , &dir , &hash , name );
    if( rc < 0 )goto abort ;
    if( dir.body.dirInode.attr.flags & MD_INODE_FLAG_NOREMOVE ){
       rc = MDEnotAllowed ;
       goto abort ;
    }
    column = rc ;
    if( mdIsNullID( hash.body.dirHash.hashPointer[column] ) ){
       rc = MDEnotFound ;
       goto abort ;
    }
    mdr.head.ID = hash.body.dirHash.hashPointer[column] ;
    mdSetNullID( lastMdr.head.ID ) ;
    for( fnd = 0 , xid = mdr.head.ID ,
         position = 0  ;
         ! mdIsNullID( xid ) ;
         xid = mdr.head.nextID , position ++ ){
         
       /*
        * run along the chain
        */ 
       mdr.head.ID = xid ;
       if( (rc = md2GetRecord( mdl , &mdr , 0 )) )goto abort ;
       for( i = 0 ; i < mdr.body.dirData.dirHead.entries ; i++ ){
         if( ( ! mdIsNullID( mdr.body.dirData.dirItem[i].ID      ) )&&
             ( ! strcmp( mdr.body.dirData.dirItem[i].name , name ) )   ){
           fnd = 1 ;
           break ;
         }
       }
       if( fnd )break ;
       lastMdr = mdr ;
    }
    if( ! fnd ){ rc = MDEnotFound ; goto abort ; }
    xid   = mdr.body.dirData.dirItem[i].ID ;
    xitem = mdr.body.dirData.dirItem[i] ;
    mdSetNullID( mdr.body.dirData.dirItem[i].ID ) ;
    /*
     * adjust the high water mark ( entries )
     * looks funny but works ( don't ask why )
     */
    for( i = (mdr.body.dirData.dirHead.entries - 1) ;
         ( i >= 0 ) && mdIsNullID( mdr.body.dirData.dirItem[i].ID ) ;
         i-- )
       mdr.body.dirData.dirHead.entries-- ;
    /*
     * removing of unused records 
     */
    if( mdr.body.dirData.dirHead.entries > 0 ){
      if( (rc = md2PutRecord( mdl , &mdr , 0 )) )goto abort ;
    }else{
       /* ohoho..... */
      if( (rc = md2DeleteRecord( mdl , mdr.head.ID , 0 )) )goto abort ;
       if( mdIsNullID( lastMdr.head.ID ) ){
         /* we are the first block in the chain, so we have to
          * modify the hash
          */
          hash.body.dirHash.hashPointer[column] = mdr.head.nextID ;
          if( (rc = md2PutRecord( mdl , &hash , 0 )) )goto abort ;
       }else{
         /* we are not the first, so we have to modify
          * the previous block
          */
          lastMdr.head.nextID = mdr.head.nextID ;
          if( (rc = md2PutRecord( mdl , &lastMdr , 0 )) )goto abort ;
       }
    
    }
    
    
    dir.body.dirInode.attr.entries-- ;
    time( (time_t*)&dir.body.dirInode.attr.unixAttr.mst_atime ) ;
    dir.body.dirInode.attr.unixAttr.mst_mtime =
       dir.body.dirInode.attr.unixAttr.mst_atime ;
    if( (rc = md2PutRecord( mdl , &dir , 0 )) )goto abort ;
    
    if( resID )*resID = xid ;
    if( item  )memcpy((char*)item,(char*)&xitem,sizeof(xitem)); 
 if(md2CommitLock( mdl ))return MDEnoCommit ;
 return 0 ;
 
abort :

 md2AbortLock( mdl ) ;
 return rc ;  
}
int md2RemoveDirectory( MDL * mdl , md_id_t dirID , char *name )
{
 mdRecord  dir  ;
 int       rc  ;
 md_id_t   resID ;
 md_dir_item item ;
 
 rc = 0 ;
 if(md2WriteLock( mdl ) )return MDEnoLock ;

 if( (rc = md2ExtLookup( mdl , dirID , name ,&resID , &item )))goto abort;

   dir.head.ID = resID ;
   if( (rc = md2GetRecord( mdl , &dir , 0 )) )goto abort ;
   if( ! ( mdIsType( dir.head.type, mdtInode     )  &&
           mdIsType( dir.head.type, mdtDirectory )     )  ){ 
         rc = MDEnotDirectory ;
         goto abort ;
   }
   if( dir.body.dirInode.attr.entries > 0 ){
         rc = MDEnotEmpty ;
         goto abort ;
   }

   if( dirID.db != resID.db ){ rc = MDEdbMissmatch ; goto abort ; }

   rc = md2ExtRemoveFromDirectory( mdl , dirID , name , &resID , &item);
   if( rc )goto abort ;

   if( ! mdpIsEntryOnly( item.perm ) ){
     if( (rc = md2DeleteDirectory( mdl , resID )) )goto abort ;
   }
   
 if(md2CommitLock( mdl ))return MDEnoCommit ;
 return rc ;
 
abort :

 md2AbortLock( mdl ) ;
 return rc ;  

}
int md2DeleteDirectory( MDL * mdl , md_id_t dirID )
{
 mdRecord  dir , hash , mdr ;
 int       rc , i , j ;
 md_id_t   id ;
 
 if(md2WriteLock( mdl ) )return MDEnoLock ;
 

   dir.head.ID = dirID ;
   if( (rc = md2GetRecord( mdl , &dir , 0 )) )goto abort ;
   if( ! ( mdIsType( dir.head.type, mdtInode     )  &&
           mdIsType( dir.head.type, mdtDirectory )     )  ){ 
         rc = MDEnotDirectory ;
         goto abort ;
   }
   if( dir.body.dirInode.attr.entries > 0 ){
         rc = MDEnotEmpty ;
         goto abort ;
   }
   if( (rc = md2RemoveTags( mdl , dirID  )) )goto abort ;
   
   for( i = 0 ; i < dir.body.dirInode.hashInfo.rows ; i++ ){
        hash.head.ID = dir.body.dirInode.hashHandle[i] ;
        if( (rc = md2GetRecord( mdl , &hash , 0 )) )goto abort ;  
        for( j = 0 ; j < hash.body.dirHash.hashHead.entries ; j++ ){
        
           if( mdIsNullID( hash.body.dirHash.hashPointer[j] ) )continue ;
           
           for( id = hash.body.dirHash.hashPointer[j] ;! mdIsNullID(id) ; ){ 
                
                mdr.head.ID = id ;              
                if( (rc = md2GetRecord( mdl , &mdr , 0 )) )goto abort ;
                id = mdr.head.nextID  ;
                if( (rc = md2DeleteRecord( mdl , mdr.head.ID , 0 )) )goto abort ;
                
           }
        }
        if( (rc = md2DeleteRecord( mdl , hash.head.ID , 0 )) )goto abort ;    
   }
   if( (rc = md2DeleteRecord( mdl , dir.head.ID , 0 )) )goto abort ;
 if(md2CommitLock( mdl ))return MDEnoCommit ;
 return 0 ;
 
abort :

 md2AbortLock( mdl ) ;
 return rc ;  

}
int md2FindEntryByID( MDL * mdl , md_id_t parentID , md_id_t childID , md_dir_item *item )
{
 mdRecord  dir , hash , data ;
 int       rc , i , j , k , fnd ;
 md_id_t   id ;
 
 if(md2WriteLock( mdl ) )return MDEnoLock ;
 
   dir.head.ID = parentID ;
   if( (rc = md2GetRecord( mdl , &dir , 0 )) )goto abort ;
   if( ! ( mdIsType( dir.head.type, mdtInode     )  &&
           mdIsType( dir.head.type, mdtDirectory )     )  ){ 
         rc = MDEnotDirectory ;
         goto abort ;
   }
   if( dir.body.dirInode.attr.entries <= 0 ){
         rc = MDEnotFound ;
         goto abort ;
   }
   for( i = 0 , fnd = 0  ; i < dir.body.dirInode.hashInfo.rows ; i++ ){
        hash.head.ID = dir.body.dirInode.hashHandle[i] ;
        if( (rc = md2GetRecord( mdl , &hash , 0 )) )goto abort ;  
        for( j = 0 ; j < hash.body.dirHash.hashHead.entries ; j++ ){
        
           if( mdIsNullID( hash.body.dirHash.hashPointer[j] ) )continue ;
           
           for( id = hash.body.dirHash.hashPointer[j] ;
                ! mdIsNullID(id) ; 
                id = data.head.nextID  ){
                
                data.head.ID = id ;
                if( (rc = md2GetRecord( mdl , &data , 0 )) )goto abort ;
                for( k = 0 ; k < data.body.dirData.dirHead.entries ; k++ ){
                 if( mdIsEqualID( data.body.dirData.dirItem[k].ID , childID)  )
                     { fnd = 1 ; break ; }
                }
                if(fnd)break ;
           }
           if(fnd)break ;
        }
        if(fnd)break ;
   }
 if(md2CommitLock( mdl ))return MDEnoCommit ;
 if( fnd && item )memcpy((char*)item,(char*)&data.body.dirData.dirItem[k],sizeof(md_dir_item));
 return fnd ? 0 : MDEnotFound ;
 
abort :

 md2AbortLock( mdl ) ;
 return rc ;  

}
int md2FindDirectoryEntryByID( MDL * mdl , md_id_t childID , md_dir_item *item )
{
 mdRecord  dir  ;
 int       rc ;
 
 if(md2WriteLock( mdl ) )return MDEnoLock ;
 
   dir.head.ID = childID ;
   if( (rc = md2GetRecord( mdl , &dir , 0 )) )goto abort ;
   if( mdIsNullID( dir.head.parentID ) ){ rc = MDEnotFound ; goto abort ; }
   
   if( (rc = md2FindEntryByID( mdl , dir.head.parentID , childID ,item )) )goto abort ;
 
 if(md2CommitLock( mdl ))return MDEnoCommit ;
 
abort :

 md2AbortLock( mdl ) ;
 return rc ;  

}
int md2ExtAddToDirectory( MDL * mdl , md_id_t dirID , md_dir_item *newItem )
{
 mdRecord  dir , hash , mdr , nw ;
 int       rc , column , fnd , i ;
 md_id_t   xid ;
 
 if(md2WriteLock( mdl ) )return MDEnoLock ;
    dir.head.ID = dirID ;
    rc = md2GetDirectoryInfos( mdl , &dir , &hash , newItem->name );
    if( rc < 0 )goto abort ;
    column = rc ;
    if( mdIsNullID( hash.body.dirHash.hashPointer[column] ) ){
      if( (rc = md2NewDirectoryData( mdl , &mdr )) )goto abort ;
       mdr.head.parentID = dirID ;
       mdr.head.baseID   = hash.head.ID ;
       mdSetNullID( mdr.head.backID ) ;
       hash.body.dirHash.hashPointer[column] = mdr.head.ID ;
       if( (rc = md2PutRecord( mdl , &hash , 0 )) )goto abort ;
       if( (rc = md2PutRecord( mdl , &mdr  , 0 )) )goto abort ;
    }else{
       mdr.head.ID = hash.body.dirHash.hashPointer[column] ;
    }
    for( fnd = 0 , xid = mdr.head.ID ; ! mdIsNullID( xid ) ; xid = mdr.head.nextID ){
       mdr.head.ID = xid ;
       if( (rc = md2GetRecord( mdl , &mdr , 0 )) )goto abort ;
       for( i = 0 ; i < mdr.body.dirData.dirHead.entries ; i++ ){
         if( mdIsNullID( mdr.body.dirData.dirItem[i].ID ) ){
           fnd = 1 ;
           break ;
         }
       }
       if( fnd ||
          ( mdr.body.dirData.dirHead.entries < mdr.body.dirData.dirHead.maxEntries) 
         )break ;
    }
    if( fnd ){   /* old empty entry */
        mdCopyItem( &mdr.body.dirData.dirItem[i] , newItem ) ;
        if( (rc = md2PutRecord( mdl , &mdr , 0 )) )goto abort ;
    }else if( mdr.body.dirData.dirHead.entries < mdr.body.dirData.dirHead.maxEntries ){
        i = mdr.body.dirData.dirHead.entries ;
	    mdCopyItem( &mdr.body.dirData.dirItem[i] , newItem ) ;
        mdr.body.dirData.dirHead.entries ++ ;
        if( (rc = md2PutRecord( mdl , &mdr , 0 )) )goto abort ;
    }else{
      if( (rc = md2NewDirectoryData( mdl , &nw )) )goto abort ;
        nw.head.parentID = mdr.head.parentID ;
        nw.head.baseID   = mdr.head.baseID ;
        nw.head.backID   = mdr.head.ID ;
        mdr.head.nextID  = nw.head.ID ;
	    mdCopyItem( &nw.body.dirData.dirItem[0] , newItem ) ;
        nw.body.dirData.dirHead.entries = 1 ;
        if( (rc = md2PutRecord( mdl , &mdr , 0 )) )goto abort ;
        if( (rc = md2PutRecord( mdl , &nw , 0 )) )goto abort ;
    }
    dir.body.dirInode.attr.entries++ ;
    time( (time_t*)&dir.body.dirInode.attr.unixAttr.mst_atime ) ;
    dir.body.dirInode.attr.unixAttr.mst_mtime =
       dir.body.dirInode.attr.unixAttr.mst_atime ;
    if( (rc = md2PutRecord( mdl , &dir , 0 )) )goto abort ;
 if(md2CommitLock( mdl ))return MDEnoCommit ;
 return 0 ;
 
abort :

 md2AbortLock( mdl ) ;
 return rc ;  
}
int md2AddToDirectory( MDL * mdl , md_id_t dirID , char *name , md_id_t id )
{
   md_dir_item  item ;
   
   memset( (char*)&item , 0 , sizeof( md_dir_item ) ) ;
   strncpy( item.name , name , MD_MAX_NAME_LENGTH-1 ) ;
   item.ID = id ;
   return md2ExtAddToDirectory( mdl , dirID , &item );
}
int md2AddToDirectoryOnly( MDL * mdl , md_id_t dirID , char *name , md_id_t id )
{
   md_dir_item  item ;
   
   memset( (char*)&item , 0 , sizeof( md_dir_item ) ) ;
   strncpy( item.name , name , MD_MAX_NAME_LENGTH-1 ) ;
   item.ID = id ;
   mdpSetEntryOnly( item.perm ) ;
   return md2ExtAddToDirectory( mdl , dirID , &item );
}

int md2GetDirectoryInfos( MDL * mdl , mdRecord *dir , mdRecord *hash , char *name )
{
 int rc , row , column ;
 
 if(md2WriteLock( mdl ) )return MDEnoLock ;
 if( (rc  =  md2GetRecord( mdl , dir , 0 )) )goto abort ;
   if( ! ( mdIsType( dir->head.type, mdtInode     )  &&
           mdIsType( dir->head.type, mdtDirectory )     )  ){ 
         rc = MDEnotDirectory ;
         goto abort ;
   }
   rc = md2HushFunc0( name , dir->body.dirInode.hashInfo.size ) ;
   if( rc < 0 )goto abort ;
   row    = rc / dir->body.dirInode.hashInfo.entriesPerRow ;
   column = rc % dir->body.dirInode.hashInfo.entriesPerRow ;
   if( row >= dir->body.dirInode.hashInfo.rows ){ rc = MDEpanic1 ; goto abort ; }
   hash->head.ID = dir -> body.dirInode.hashHandle[row] ;
   if( (rc  =  md2GetRecord( mdl , hash , 0 )) )goto abort ;
   if( column >= hash->body.dirHash.hashHead.entries ){ rc = MDEpanic1 ; goto abort ; }
   
 if(md2CommitLock( mdl ))return MDEnoCommit ;
 return column ;
 
abort :

 md2AbortLock( mdl ) ;
 return rc ;  

}
int md2NewDirectoryData( MDL * mdl , mdRecord *mdr )
{
  int rc ;
  
  if( (rc = md2GetNewRecord( mdl , mdr )) )return rc ;
   mdAddType( mdr->head.type , mdtData ) ;
   mdAddType( mdr->head.type , mdtDirectory ) ;
   
   mdr->body.dirData.dirHead.entries    = 0 ;
   mdr->body.dirData.dirHead.maxEntries = DIR_ITEMS ;
   
   return 0 ;
}
int md2GetNewRecord( MDL * mdl , mdRecord *mdr )
{
    mdClearRecord(mdr) ;
    time((time_t*) &( mdr -> head.cTime ) ) ;
    time((time_t*) &( mdr -> head.mTime ) ) ;
    return md2GetNextId( mdl , &(mdr -> head.ID) ) ;
}
#define ac2bin(c) ((((c)>='0')&&((c)<='9'))?(c)-'0':\
                   (((c)>='a')&&((c)<='f'))?(c)-'a'+10:\
                   (((c)>='A')&&((c)<='F'))?(c)-'A'+10:0)
void md2ScanId( char *s , md_id_t *id )
{
  md2ScanIdLevel( s , id , NULL );
  return ;
}
#ifdef _LITTLE_ENDIAN
void md2ScanPermission( char *s , md_permission *id )
{
   char tmp[2*sizeof(md_permission)+2];
   int len = strlen( s ) ;
   
   len = len > 2*sizeof(md_permission) ?  2*sizeof(md_permission)  :len ;
   memset( tmp , '0' , 2*sizeof(md_permission) ) ;
   tmp[2*sizeof(md_permission)] = '\0' ;
   memcpy( &tmp[2*sizeof(md_permission)-len] , s , len ) ;
   sscanf( tmp , "%8lx%8lx", &id->high,&id->low);
   return ;
      
}
void md2ScanIdLevel( char *s , md_id_t *id , int *level )
{
   char tmp[2*sizeof(md_id_t)+2];
   int len = strlen( s ) ;
   
   len = len > 2*sizeof(md_id_t) ?  2*sizeof(md_id_t)  :len ;
   memset( tmp , '0' , 2*sizeof(md_id_t) ) ;
   tmp[2*sizeof(md_id_t)] = '\0' ;
   memcpy( &tmp[2*sizeof(md_id_t)-len] , s , len ) ;
   sscanf( tmp , "%4hx%4hx%8lx%8lx", &id->db,&id->ext,&id->high,&id->low);
   if( level ){
     *level    = (*id).low & 0x7 ;
     (*id).low = (*id).low & ~ 0x7 ;  
   }
}
#else
void md2ScanIdLevel( char *s , md_id_t *id , int *level )
{
   char tmp[2*sizeof(md_id_t)+2];
   int len = strlen( s ) ;
   
   len = len > 2*sizeof(md_id_t) ?  2*sizeof(md_id_t)  :len ;
   memset( tmp , '0' , 2*sizeof(md_id_t) ) ;
   tmp[2*sizeof(md_id_t)] = '\0' ;
   memcpy( &tmp[2*sizeof(md_id_t)-len] , s , len ) ;
   sscanf( tmp , "%4hx%4hx%8x%8x", &id->db,&id->ext,&id->high,&id->low);
   if( level ){
     *level    = (*id).low & 0x7 ;
     (*id).low = (*id).low & ~ 0x7 ;  
   }
}
void md2ScanPermission( char *s , md_permission *id )
{
   char tmp[2*sizeof(md_permission)+2];
   int len = strlen( s ) ;
   
   len = len > 2*sizeof(md_permission) ?  2*sizeof(md_permission)  :len ;
   memset( tmp , '0' , 2*sizeof(md_permission) ) ;
   tmp[2*sizeof(md_permission)] = '\0' ;
   memcpy( &tmp[2*sizeof(md_permission)-len] , s , len ) ;
   sscanf( tmp , "%8lx%8lx", &id->high,&id->low);
   return ;
      
}

#ifdef veryBuggy
void md2ScanIdLevel( char *s , md_id_t *id , int *level )
{
  int len = strlen( s ) ;
  int blen = sizeof( *id ) ;
  int mdsize = sizeof(md_id_t) ;
  unsigned char *o = (unsigned char *)id  ;
  int i , pos , out ;
  
  memset( o , 0 , mdsize );
  len = len > 2*mdsize ? 2*mdsize : len ;
  for( i = 0 , pos = len - 1 , out = mdsize -1  ; 
       ( i < len ) && ( i < mdsize*2 ) ;
        i++ , pos --  ){
    if( i % 2 ){
       o[out] |=  ( ac2bin( s[pos] ) << 4 ) ;
       out-- ;
    
    }else{
       o[out] =  ac2bin( s[pos] ) ; 
    }
  }
  if( level ){
     *level    = (*id).low & 0x7 ;
     (*id).low = (*id).low & ~ 0x7 ;  
  }
  return ;
      
}
void md2ScanPermission( char *s , md_permission *id )
{
  int len = strlen( s ) ;
  int blen = sizeof( *id ) ;
  int mdsize = sizeof(md_permission) ;
  unsigned char *o = (unsigned char *)id  ;
  int i , pos , out ;
  
  memset( o , 0 , mdsize );
  for( i = 0 , pos = len - 1 , out = mdsize -1  ; 
       ( i < len ) && ( i < mdsize*2 ) ;
        i++ , pos --  ){
    if( i % 2 ){
       o[out] |=  ( ac2bin( s[pos] ) << 4 ) ;
       out-- ;
    
    }else{
       o[out] =  ac2bin( s[pos] ) ; 
    }
  }
  return ;
      
}
#endif

#endif
int md2GetNextId( MDL *mdl , md_id_t *id )
{
  int rc ;
  
  mdSetRootID( *id ) ;
  
  if( ( ! mdl ) || ( mdl -> accessMode != mdl_RDWR ) )return -1 ;

  if( mdl -> id.count <= 0 ){
    if( (rc = md2GetIdRange( mdl , mdl->id.inc )) )return rc ;
     if( mdl -> id.count <= 0 )return -2 ;
  }
  if( id ) *id =  mdl -> id.next  ;
  mdAddID( mdl -> id.next ,  8 ) ;
  mdl -> id.count -- ;

  return 0 ;
  
}

#ifdef _LITTLE_ENDIAN
char * md2PrintFhandle( mdFhandle m )
{
   static char str[2*(sizeof(md_id_t)+sizeof(md_permission))+2] ;
   sprintf(str,"%4.4X%4.4X%8.8lX%8.8lX-%8.8lX%8.8lX",
               m.id.db,m.id.ext,m.id.high,m.id.low,
               m.permission.high,m.permission.low) ;
   return str ;
}
char * md2PrintID( md_id_t id )
{
   static char str[2*sizeof(md_id_t)+1] ;
   sprintf(str,"%4.4X%4.4X%8.8lX%8.8lX",id.db,id.ext,id.high,id.low) ;
   str[2*sizeof(md_id_t)] = '\0' ;
   return str ;
}
char * md2PrintPermission( md_permission id )
{
   static char str[2*sizeof(md_permission)+1] ;
   sprintf(str,"%8.8lX%8.8lX",id.high,id.low) ;
   return str ;
}
#else
char * md2PrintFhandle( mdFhandle m )
{
   static char str[2*(sizeof(md_id_t)+sizeof(md_permission))+2] ;
   int i ;
   char *s ;
   unsigned char *x ;
   x = (unsigned char *)&m.id ;
   for(i = 0; i<sizeof(md_id_t) ; i++ )sprintf( &str[i*2] , "%2.2X" , x[i] ) ;
   str[i*2] = '-' ;
   s = &str[i*2+1] ;
   x = (unsigned char *)&m.permission ;
   for(i = 0; i<sizeof(md_permission) ; i++ )sprintf( &s[i*2] , "%2.2X" , x[i] )
   ;
   s[i*2] = '\0' ;
   return str ;
}
char * md2PrintID( md_id_t id )
{
   static char str[2*sizeof(md_id_t)+1] ;
   int i ;
   unsigned char *x ;
   x = (unsigned char *)&id ;
   for(i = 0; i<sizeof(md_id_t) ; i++ )sprintf( &str[i*2] , "%2.2X" , x[i] ) ;
   str[i*2] = '\0' ;
   return str ;
}
char * md2PrintPermission( md_permission id )
{
   static char str[2*sizeof(md_permission)+1] ;
   int i ;
   unsigned char *x ;
   x = (unsigned char *)&id ;
   for(i = 0; i<sizeof(md_permission) ; i++ )sprintf( &str[i*2] , "%2.2X" , x[i]
   ) ;
   str[i*2] = '\0' ;
   return str ;
}
#endif
char * md2PrintTypes( md_type_t t )
{
  unsigned long l ;
  int i ;
  static char str[mdtLast+2] ;
  
  l = t.low ;
  for( i = 0 ; i < (mdtLast+1) ; i++ )
     str[i] = ( l & (1<<i) ) ? mdtToString[i] : '-' ;

  return str ;
}
char * md2PrintUnixAttrS( md_unix *a )
{
  static char str[128] ;
  char dt[64] ;
  time_t clock ;
  struct tm *m ;
  static char *tm_mon_map[] =
  {"Jan","Feb","Mar","Apr","Mai","Jun","Jul","Aug","Sep","Oct","Nov","Dec"} ;
  
  str[0] = S_ISDIR(a->mst_mode)?'d':'-';
  str[1] = a->mst_mode&S_IRUSR?'r':'-';
  str[2] = a->mst_mode&S_IWUSR?'w':'-';
  str[3] = a->mst_mode&S_IXUSR?'x':'-';
  str[4] = a->mst_mode&S_IRGRP?'r':'-';
  str[5] = a->mst_mode&S_IWGRP?'w':'-';
  str[6] = a->mst_mode&S_IXGRP?'x':'-';
  str[7] = a->mst_mode&S_IROTH?'r':'-';
  str[8] = a->mst_mode&S_IWOTH?'w':'-';
  str[9] = a->mst_mode&S_IXOTH?'x':'-';
  str[10] = ' ' ;
  
  time( &clock ) ;
  m = localtime(&clock) ;
  sprintf(dt,"%2.2d:%2.2d %2.2d %s",
          m->tm_hour,m->tm_min , m->tm_mday,tm_mon_map[m->tm_mon]) ;
  
  sprintf(str+11,"%5ld %7ld %-s",a->mst_uid,a->mst_size,dt) ;
  return str ;
}
