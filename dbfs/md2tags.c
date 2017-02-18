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
#include <string.h>

#include "md2types.h"
#include "md2scan.h"
/*
#define sscanID(str,id)  {sscanf((str),"%x",&(id.low));id.high=0;}
*/
#define sscanID(str,id)  md2ScanId(str,&(id))

void tag_usage();
static char buffer[1024] ;

int tagmain( MDL * mdl , int argc , char *argv[] )
{
 int rc ;
 md_id_t id , toID ;
 mdRecord tag ;
 
  if( argc <= 0 ){ tag_usage() ; return -1 ; }
    
  if( ! strcmp( argv[0] , "help" ) ){
     tag_usage() ;
     return -1 ;
  }else if( ! strcmp( argv[0] , "create" ) ){
     if( argc < 3 ){ tag_usage() ; return -1 ; }
     sscanID( argv[1] ,id ) ;
     rc = md2AddNewTag( mdl , id , argv[2] , &tag );
     if(rc){fprintf(stderr," md2AddNewTag : %d\n",rc);return rc;}
     printf( " %s %s\n", mdStringID(tag.head.ID) , argv[2] ) ;
  }else if( ! strcmp( argv[0] , "find" ) ){
     if( argc < 3 ){ tag_usage() ; return -1 ; }
     sscanID( argv[1] ,id ) ;
     rc = md2FindTag( mdl , id , argv[2] , &tag );
     if(rc){fprintf(stderr," md2FindTag : %d\n",rc);return rc;}
     printf( " %s %s\n", mdStringID(tag.head.ID) , argv[2] ) ;
  }else if( ! strcmp( argv[0] , "attr" ) ){
     md_unix uattr ;
     if( argc < 2 ){ tag_usage() ; return -1 ; }
     sscanID( argv[1] ,id ) ;
     rc = md2GetTagUnixAttr( mdl , id  , &uattr) ;
     if(rc){fprintf(stderr," md2GetTagUnixAttr : %d\n",rc);return rc;}
     md2PrintUnixAttr( stdout , &uattr ) ;
  }else if( ! strcmp( argv[0] , "duplicate" ) ){
     if( argc < 3 ){ tag_usage() ; return -1 ; }
     sscanID( argv[1] ,id ) ;
     sscanID( argv[2] ,toID ) ;
     rc = md2DuplicateTags( mdl , id , toID );
     if(rc){fprintf(stderr," md2DuplicateTags : %d\n",rc);return rc;}
  }else if( ! strcmp( argv[0] , "write" ) ){
     if( argc < 3 ){ tag_usage() ; return -1 ; }
     sscanID( argv[1] ,id ) ;
     rc = md2WriteTag( mdl , id , argv[2] , 0 , strlen( argv[2] )+1 );
     if(rc){fprintf(stderr," md2WriteTag : %d\n",rc);return rc;}
  }else if( ! strcmp( argv[0] , "rm" ) ){
     if( argc < 2 ){ tag_usage() ; return -1 ; }
     sscanID( argv[1] ,id ) ;
     rc = md2RemoveTag( mdl , id  );
     if(rc<0){fprintf(stderr," md2RemoveTag : %d\n",rc);return rc;}
  }else if( ! strcmp( argv[0] , "rmall" ) ){
     if( argc < 2 ){ tag_usage() ; return -1 ; }
     sscanID( argv[1] ,id ) ;
     rc = md2RemoveTags( mdl , id  );
     if(rc<0){fprintf(stderr," md2RemoveTag : %d\n",rc);return rc;}
  }else if( ! strcmp( argv[0] , "read" ) ){
     if( argc < 2 ){ tag_usage() ; return -1 ; }
     sscanID( argv[1] ,id ) ;
     rc = md2ReadTag( mdl , id , buffer ,0 ,  sizeof(buffer) );
     if(rc<0){fprintf(stderr," md2WriteTag : %d\n",rc);return rc;}
     if(rc>0)printf( "%s %s\n" , mdStringID(id),buffer) ;
  }else{
     tag_usage() ;
     return -1 ;
  }   
   return 0 ;
}
void tag_usage()
{

 fprintf(stderr,"   Tag Help\n --------------------\n");
 fprintf(stderr," tag create     <dirID>     <tagName>\n");
 fprintf(stderr," tag find       <dirID>     <tagName>\n");
 fprintf(stderr," tag duplicate  <fromDirID> <toDirID>\n");
 fprintf(stderr," tag attr       <tagID>\n");
 fprintf(stderr," tag write  <tagID> <tagMessage>\n");
 fprintf(stderr," tag read   <tagID>\n");

}
md_object * md2IsObject( char * string  )
{
 static md_object obj ;
 
  if( md2scanObjectString( string , &obj ) )return NULL ;
  if( ! strcmp( obj.type , "tag" )          )obj.numType = MDO_TAG ;
  else if( ! strcmp( obj.type , "id" )      )obj.numType = MDO_REQ_ID ;
  else if( ! strcmp( obj.type , "path" )    )obj.numType = MDO_REQ_PATH ;
  else if( ! strcmp( obj.type , "jump" )    )obj.numType = MDO_REQ_JUMP ;
  else if( ! strcmp( obj.type , "access" )  )obj.numType = MDO_REQ_ACCESS ;
  else if( ! strcmp( obj.type , "puse" )    )obj.numType = MDO_REQ_ACCESS ;
  else if( ! strcmp( obj.type , "showid" )  )obj.numType = MDO_REQ_SHOWID ;
  else if( ! strcmp( obj.type , "getattr" ) )obj.numType = MDO_REQ_GETATTR ;
  else if( ! strcmp( obj.type , "tags" )    )obj.numType = MDO_REQ_LSTAGS ;
  else if( ! strcmp( obj.type , "ptags" )   )obj.numType = MDO_REQ_LSXTAGS ;
  else if( ! strcmp( obj.type , "forceIO" ) )obj.numType = MDO_REQ_FORCEIO ;
  else if( ! strcmp( obj.type , "forceio" ) )obj.numType = MDO_REQ_FORCEIO2 ;
  else if( ! strcmp( obj.type , "config" )  )obj.numType = MDO_REQ_CONFIG ;
  else if( ! strcmp( obj.type , "set"  )    )obj.numType = MDO_REQ_FSET ;
  else if( ! strcmp( obj.type , "const" )   )obj.numType = MDO_REQ_CONST ;
  else if( ! strcmp( obj.type , "name" )    )obj.numType = MDO_REQ_NAME ;
  else if( ! strcmp( obj.type , "level" )   )obj.numType = MDO_REQ_LEVEL ;
  else if( ! strcmp( obj.type , "use" )     )obj.numType = MDO_REQ_LEVEL ;
  else if( ! strcmp( obj.type , "parent" )  )obj.numType = MDO_REQ_PARENT ;
  else if( ! strcmp( obj.type , "nameof" )  )obj.numType = MDO_REQ_NAMEOF ;
  else if( ! strcmp( obj.type , "fset" )    )obj.numType = MDO_REQ_SET ;
  else if( ! strcmp( obj.type , "pset" )    )obj.numType = MDO_REQ_PSET ;
  else if( ! strcmp( obj.type , "get" )     )obj.numType = MDO_REQ_GET ;
  else if( ! strcmp( obj.type , "list" )    )obj.numType = MDO_REQ_LIST ;
  else {
     char *x ;
     for( x = obj.type ; (*x != '\0')&& isdigit(*x) ; x++ );
     if( *x != '\0' )obj.numType = MDO_UNKNOWN ;   
     else            obj.numType = MDO_REQ_DIGITS ;
  }

  return &obj ;

}
int md2GetTagName( char * string , char *tagName )
{
 md_object obj ;
 
  if( md2scanObjectString( string , &obj ) )return -1 ;
  if( strcmp( obj.type , "tag" ) )return -2 ;
  if(tagName)strcpy( tagName , obj.name ) ;
  return 0 ;

}
int md2TagUnChain( MDL *mdl , mdRecord *tag )
{
 mdRecord base ;
 int      rc ;
 
 if(md2WriteLock( mdl ))return MDEnoLock ;
 
 
   if( rc = md2TagUnChainLow( mdl , tag ) )goto abort ;
   if( rc = md2PutRecord( mdl , tag , 0 ) )goto abort ;

 if(md2CommitLock( mdl ))return MDEnoCommit ;
 return 0 ;
 
abort :

 md2AbortLock( mdl ) ;
 return rc ;  


}
int md2TagUnChainLow( MDL *mdl , mdRecord *tag )
{
 mdRecord base ;
 int      rc ;
 
 
 if( tag->body.tagInode.attr.flags & mdTagInherited ){
     memcpy( (char*)&base , (char *)tag , sizeof(md_record) ) ;
     if( rc = md2GetTopTagByRec( mdl , &base , NULL ) )return rc ;
     memcpy( (char *)&(tag->body.tagInode.attr.unixAttr),
             (char *)&base.body.tagInode.attr.unixAttr ,
             sizeof( md_unix ) ) ;
     time( (time_t*)&base.body.tagInode.attr.unixAttr.mst_mtime ) ;
     time( (time_t*)&base.body.tagInode.attr.unixAttr.mst_atime ) ;
     tag->body.tagInode.attr.flags &= ~mdTagInherited;
 }else{
     tag->body.tagInode.attr.flags &= ~mdTagInvalid;
 }
 /* this part destoys the content of a tag on setAttribute
  * but it certainly had a reason to be here.
  *
 tag->body.tagInode.attr.unixAttr.mst_size = 
 tag->body.tagInode.attr.entries = 0 ;
 */
 return 0 ;


}
int md2GetTopTag( MDL *mdl , md_id_t id , mdRecord *res )
{
 mdRecord tag ;
 int      rc ;

 rc = 0 ; 
 if(md2ReadLock( mdl ))return MDEnoLock ;
 
    for( ; ! mdIsNullID( id )  ; id = tag.head.parentID ){
       tag.head.ID = id ;
       rc  =  md2GetRecord( mdl , &tag , 0 ) ;
       if(  rc == MDEnotFound )goto ok ;
       if(  rc != 0 )goto abort ;
       if( ( ! ( tag.body.tagInode.attr.flags & mdTagInherited ) )||
               ( tag.body.tagInode.attr.flags & mdTagInvalid   )    )break ;
    }
    if( mdIsNullID( id ) ) rc = MDEnotFound ; 
	
ok:

 if(md2CommitLock( mdl ))return MDEnoCommit ;
 if( res )mdCopyRecord( res , &tag ) ;
 return rc ;
 
abort :

 md2AbortLock( mdl ) ;
 return rc ;  

}
int md2GetTopTagByRec( MDL *mdl , mdRecord *tag  , mdRecord *res )
{
 int      rc , i ;
 md_id_t id ;

 rc = 0 ; 
 if(md2ReadLock( mdl ))return MDEnoLock ;
    id = tag -> head.ID ;
    for( i = 0 ; ! mdIsNullID( id )  ; id = tag->head.parentID , i++){
       if( i ){
          tag->head.ID = id ;
          rc  =  md2GetRecord( mdl , tag , 0 ) ;
          if(  rc == MDEnotFound )goto ok ;
          if(  rc != 0 )goto abort ;
       }
       if( ( ! ( tag->body.tagInode.attr.flags & mdTagInherited ) )||
               ( tag->body.tagInode.attr.flags & mdTagInvalid   )    )break ;
    }
    if( mdIsNullID( id ) ) rc = MDEnotFound ; 
	
ok:

 if(md2CommitLock( mdl ))return MDEnoCommit ;
 if( res )mdCopyRecord( res , tag ) ;
 return rc ;
 
abort :

 md2AbortLock( mdl ) ;
 return rc ;  

}
int md2WriteTag( MDL * mdl , md_id_t tagID , char *ptr , 
                 md_long offset , md_long size           )
{
 mdRecord tag ;
 int      rc ;
 
 if(md2WriteLock( mdl ))return MDEnoLock ;
   tag.head.ID = tagID ;
   if( rc = md2GetRecord( mdl , &tag , 0 ) )goto abort ;
   if( rc = md2WriteTagLow( mdl , &tag , ptr ,offset , size) )goto abort ;

   if( rc  =  md2PutRecord( mdl , &tag , 0 ) )goto abort ;
 
 if(md2CommitLock( mdl ))return MDEnoCommit ;
 return 0 ;
abort :

 md2AbortLock( mdl ) ;
 return rc ;  
}
int md2WriteTagLow( MDL * mdl , mdRecord *tag , char *ptr , 
                      md_long offset , md_long size           )
{
   int rc ;
   mdRecord topTag ;
   
 
   if( offset < TAG_DATA_SIZE ){
   
      tag->body.tagInode.attr.unixAttr.mst_size = 
      tag->body.tagInode.attr.entries = md_min( offset+size , TAG_DATA_SIZE ) ;
      memcpy( &tag->body.tagInode.data[offset] , ptr , 
	           tag->body.tagInode.attr.entries - offset         ) ;
      
      if( tag->body.tagInode.attr.flags & mdTagInherited ) {
          /**
              if this tag was an inherited one, we cut the
              chain and declare this tag to be a
              pseodo primary tag.
           */
          if( rc = md2TagUnChainLow( mdl , tag ) )return rc ;
           
      }else if( tag->body.tagInode.attr.flags & mdTagInvalid   ){
          /* 
           * writing to a tag makes it valid again
           */
          tag->body.tagInode.attr.flags &= ~mdTagInvalid ;
          
      }
      if( rc = md2GetTopTag( mdl , tag->head.ID  , &topTag ) )return rc ;
      /*
       *  we need to update the top tag to keep the client cash consistent.
       */
      if( ! mdIsEqualID( topTag.head.ID , tag->head.ID ) ){
         /*
          *  if they are equal we will overwrite in md2WriteTag
          */
         time( (time_t*)&topTag.body.tagInode.attr.unixAttr.mst_mtime ) ;
         time( (time_t*)&topTag.body.tagInode.attr.unixAttr.mst_atime ) ;
         if( rc  =  md2PutRecord( mdl , &topTag , 0 ) )return rc ;
         
      }else{
         time( (time_t*)&tag->body.tagInode.attr.unixAttr.mst_mtime ) ;
         time( (time_t*)&tag->body.tagInode.attr.unixAttr.mst_atime ) ;
      }

   }
 
 return 0 ;

}
void md2PrintTagInode( FILE *f , md_tag_inode  *tagInode )
{

   fprintf(f," Name     : %s\n" ,tagInode -> name );
   fprintf(f," Flags    : ");
   if( tagInode -> attr.flags & mdTagInvalid )
      fprintf(f," invalid" ) ;
   if( tagInode -> attr.flags & mdTagInherited )
      fprintf(f," inherited" ) ;
   fprintf(f,"\n" ) ;
   fprintf(f," Size     : %d\n" ,tagInode -> attr.entries );
   md2DumpMemory(f , (unsigned char *)(tagInode->data) ,
                 0L , tagInode -> attr.entries ) ;
                 
   return ;

}
int md2RemoveTags( MDL * mdl , md_id_t dirID  )
{
 mdRecord tag , dir ;
 int rc ;
  
 if(md2ReadLock( mdl ))return MDEnoLock ;
 
    dir.head.ID = dirID ;
    if( rc  =  md2GetRecord( mdl , &dir , 0 ) )goto abort ;

    for( tag.head.ID = dir.body.dirInode.attr.tag  ;
         ! mdIsNullID( tag.head.ID ) ;
         tag.head.ID = tag.head.nextID           ){
         
        if( rc  =  md2GetRecord( mdl , &tag , 0 ) )goto abort ;
        if( rc  =  md2DeleteRecord( mdl , tag.head.ID , 0 ) )goto abort ;
        
    }
    mdSetNullID( dir.body.dirInode.attr.tag ) ;
    if( rc  =  md2PutRecord( mdl , &dir , 0 ) )goto abort ;
 if(md2CommitLock( mdl ))return MDEnoCommit ;
 return 0 ;
abort :

 md2AbortLock( mdl ) ;
 return rc ;  

}
int md2RemoveTag( MDL * mdl , md_id_t tagID  )
{
 mdRecord tag  ;
 int rc ;
  
 if(md2ReadLock( mdl ))return MDEnoLock ;
 
    tag.head.ID = tagID ;
    if( rc  =  md2GetRecord( mdl , &tag , 0 ) )goto abort ;

       if( ! ( tag.body.tagInode.attr.flags & mdTagInherited ) ){
    
        if( ! mdIsNullID( tag.head.parentID ) ){
          tag.body.tagInode.attr.flags |= mdTagInherited ;
          tag.body.tagInode.attr.entries   = 0 ;
        }else{
          tag.body.tagInode.attr.flags |= mdTagInvalid ;
          tag.body.tagInode.attr.entries   = 0 ;
        }
        time( (time_t*)&tag.body.tagInode.attr.unixAttr.mst_mtime ) ;
        if( rc  =  md2PutRecord( mdl , &tag , 0 ) )goto abort ;
       }
       
 if(md2CommitLock( mdl ))return MDEnoCommit ;
 return 0 ;
abort :

 md2AbortLock( mdl ) ;
 return rc ;  

}
int md2GetTagUnixAttr( MDL * mdl , md_id_t tagID  , md_unix *uattr)
{
 mdRecord tag  ;
 int rc  ;
  
 if(md2ReadLock( mdl ))return MDEnoLock ;

    if( rc = md2GetTopTag( mdl , tagID , &tag ) )goto abort ;
    memcpy( (char *)uattr , 
            (char *)&tag.body.tagInode.attr.unixAttr , 
            sizeof( md_unix ) ) ;
    uattr->mst_nlink  = 0 ;
    /*
     * Linux : I'm so confused ( you are such an idiot )
     *
    uattr->mst_dev    = mdDeviceID( tag.head.ID) ;
    uattr->mst_ino    = mdInodeID( tag.head.ID) ;
    */
    uattr->mst_dev    = mdDeviceID( tagID) ;
    uattr->mst_ino    = mdInodeID( tagID) ;
    
    if( memcmp( (char*)&tagID,(char*)&tag.head.ID,sizeof(md_id_t)) ){
       uattr -> mst_nlink = 10 ;
    } 
    if( !(tag.body.tagInode.attr.flags & mdTagInvalid) )uattr -> mst_nlink += 1 ;

 if(md2CommitLock( mdl ))return MDEnoCommit ;
 return 0 ;
abort :

 md2AbortLock( mdl ) ;
 return rc ;  

}
int md2FindTag( MDL * mdl , md_id_t dirID , char *name , mdRecord *resTag  )
{
 mdRecord tag , dir ;
 int rc , f ;
  
 if(md2ReadLock( mdl ))return MDEnoLock ;
 
    dir.head.ID = dirID ;
    if( rc  =  md2GetRecord( mdl , &dir , 0 ) )goto abort ;

    for( tag.head.ID = dir.body.dirInode.attr.tag  , f = 0 ;
         ! mdIsNullID( tag.head.ID ) ;
         tag.head.ID = tag.head.nextID           ){
         
        if( rc  =  md2GetRecord( mdl , &tag , 0 ) )goto abort ;
        if( ! strncmp( tag.body.tagInode.name , name , MAX_TAG_NAME_SIZE-1) ){
            f = 1 ;
            break ; 
        } 
    }
    if( resTag && f )mdCopyRecord(resTag,&tag);
 if(md2CommitLock( mdl ))return MDEnoCommit ;
 return f ? 0 : MDEnotFound ;
abort :

 md2AbortLock( mdl ) ;
 return rc ;  
 
}

int md2DuplicateTags( MDL * mdl , md_id_t fromID , md_id_t toID )
{
 int rc ;
 mdRecord to , from , lastTag , tag , newTag ;
 md_id_t  id , *lastIdPtr ;
 
 if(md2ReadLock( mdl ))return MDEnoLock ;
 
    from.head.ID = fromID ;
    if( rc  =  md2GetRecord( mdl , &from , 0 ) )goto abort ;
    
    to.head.ID = toID ;
    if( rc  =  md2GetRecord( mdl , &to , 0 ) )goto abort ;
    
    if( ! mdIsNullID( to.body.dirInode.attr.tag ) ){ rc = MDEexists ; goto abort ;}
    
    mdSetNullID( lastTag.head.ID )   ;
    for( id = from.body.dirInode.attr.tag ,
         lastIdPtr = &to.body.dirInode.attr.tag ;
         ! mdIsNullID( id ) ;
         id = tag.head.nextID           ){
         
        tag.head.ID = id ;
        if( rc  =  md2GetRecord( mdl , &tag , 0 ) )goto abort ;
        
        if( rc = md2GetNewRecord( mdl , &newTag ) )goto abort ;
        mdAddType( newTag.head.type , mdtInode | mdtTag ) ;
        newTag.head.baseID          = to.head.ID ; 
        newTag.head.parentID        = tag.head.ID ;
        strcpy( newTag.body.tagInode.name , tag.body.tagInode.name ) ;
        newTag.body.tagInode.attr.flags |= mdTagInherited ;
        
        *lastIdPtr = newTag.head.ID ;
        
        if( ! mdIsNullID( lastTag.head.ID ) ){
            if( rc  =  md2PutRecord( mdl , &lastTag , 0 ) )goto abort ;
        } 
        mdCopyRecord( &lastTag , &newTag ) ;
        lastIdPtr  = &lastTag.head.nextID ;
    }
    
    if( ! mdIsNullID( lastTag.head.ID ) ){
         if( rc  =  md2PutRecord( mdl , &lastTag , 0 ) )goto abort ;
    } 
    if( rc  =  md2PutRecord( mdl , &to , 0 ) )goto abort ;
	
 if(md2CommitLock( mdl ))return MDEnoCommit ;
 return 0 ;
abort :

 md2AbortLock( mdl ) ;
 return rc ;  


}
int md2ReadTagLow( MDL * mdl ,mdRecord *inTag , char *ptr ,
                     md_long offset ,md_long size)
{
 int      rc ;
 mdRecord tag ;

 if(md2ReadLock( mdl ))return MDEnoLock ;
   if( rc = md2GetTopTagByRec( mdl , inTag , &tag ) )goto abort ;
   if( ! ( tag.body.tagInode.attr.flags & mdTagInvalid ) ){
     if( offset < TAG_DATA_SIZE ){
        size = md_min( offset+size , tag.body.tagInode.attr.entries ) - offset ;
        memcpy( ptr , &tag.body.tagInode.data[offset] , size ) ;
     }
   }else{
      size = MDEnotFound ;
   }
 
 if(md2CommitLock( mdl ))return MDEnoCommit ;
 return size ;
abort :

 md2AbortLock( mdl ) ;
 return rc ;  
}

int md2ReadTag( MDL * mdl , md_id_t tagID , char *ptr , 
                 md_long offset , md_long size           )
{
 mdRecord tag ;
 int      rc ;

 if(md2ReadLock( mdl ))return MDEnoLock ;
 
     tag.head.ID = tagID ;
     if( rc = md2GetRecord( mdl , &tag , 0 ) )goto abort ;
   
     rc = md2ReadTagLow( mdl , &tag , ptr , offset , size ) ;
     if( rc < 0 )goto abort ;
    
 if(md2CommitLock( mdl ))return MDEnoCommit ;
 return rc ;
abort :

 md2AbortLock( mdl ) ;
 return rc ;  
}
#ifdef OLD_ADD_NEW_TAG
int md2AddNewTag( MDL * mdl , md_id_t dirID , char *name , mdRecord *tag )
{
 int rc ;
 mdRecord base , newTag , dir ;
 md_id_t id ;
 md_unix  attr ;
 
 if( mdIsNullID( dirID ) )return  -1 ;
 if(md2WriteLock( mdl ))return MDEnoLock ;
    dir.head.ID = dirID ;
    if( rc  =  md2GetRecord( mdl , &dir , 0 ) )goto abort ;
    if( ! mdIsType( dir.head.type , mdtDirectory ) ){
       rc = MDEnoDir ;
       goto abort ;
    }

       memset( (char *)&attr , 0 , sizeof( attr ) ) ;
       attr.mst_rdev    = 100 ;
       time((time_t*)& attr.mst_atime)  ;
       time((time_t*)& attr.mst_mtime) ;
       time((time_t*)& attr.mst_ctime) ;
       attr.mst_mode    = 0100444 ;
       attr.mst_nlink   = 1 ;
       attr.mst_uid     = 0 ;
       attr.mst_gid     = 0 ;
       attr.mst_size    = 0 ;
       attr.mst_blksize = 512 ;
       attr.mst_blocks  =  0 ;
    
    if( mdIsNullID( dir.body.dirInode.attr.tag ) ){
       if( rc = md2GetNewRecord( mdl , &newTag ) )goto abort ;
       mdAddType( newTag.head.type , mdtInode | mdtTag ) ;
       newTag.head.baseID      = dirID ;
       dir.body.dirInode.attr.tag   = newTag.head.ID ;
       strncpy( newTag.body.tagInode.name , name , MAX_TAG_NAME_SIZE-1 ) ;
       attr.mst_dev    = mdDeviceID( newTag.head.ID) ;
       attr.mst_ino    = mdInodeID( newTag.head.ID) ;
       memcpy((char*)&newTag.body.tagInode.attr.unixAttr,
              (char*)&attr,
              sizeof(md_unix));
       if( rc  =  md2PutRecord( mdl , &newTag , 0 ) )goto abort ;
       if( rc  =  md2PutRecord( mdl , &dir , 0 ) )goto abort ;
    
    }else{
       for( id = dir.body.dirInode.attr.tag ;
            ! mdIsNullID( id ) ;   id = base.head.nextID ){
          base.head.ID = id ;
          if( rc  =  md2GetRecord( mdl , &base , 0 ) )goto abort ;
       }
       if( rc = md2GetNewRecord( mdl , &newTag ) )goto abort ;
       mdAddType( newTag.head.type , mdtInode | mdtTag ) ;
       newTag.head.baseID = dirID ;
       base.head.nextID   = newTag.head.ID ;
       strncpy( newTag.body.tagInode.name , name , MAX_TAG_NAME_SIZE-1 ) ;
       attr.mst_dev    = mdDeviceID( newTag.head.ID) ;
       attr.mst_ino    = mdInodeID( newTag.head.ID) ;
       memcpy((char*)&newTag.body.tagInode.attr.unixAttr,
              (char*)&attr,
              sizeof(md_unix));
       if( rc  =  md2PutRecord( mdl , &base , 0 ) )goto abort ;
       if( rc  =  md2PutRecord( mdl , &newTag , 0 ) )goto abort ;

    }
    if( tag )mdCopyRecord( tag , &newTag ) ;
 if(md2CommitLock( mdl ))return MDEnoCommit ;
 return 0 ;
 
abort :

 md2AbortLock( mdl ) ;
 return rc ;  

}
#else
typedef struct md_tagprop_info_ {
   md_id_t parent ;
   char    name[MAX_TAG_NAME_SIZE] ;
} md_tagprop_info ;

int md2TagPropagateRec(MDL*mdl,md_id_t dirID,void *glbl,md_dir_item *item );

int md2TagPropagateLow( MDL * mdl , md_id_t tagID  )
{
 int rc ;
 mdRecord tag ;
 md_tagprop_info  info ;
 
 if( md2WriteLock( mdl ))return MDEnoLock ;
   tag.head.ID = tagID ;
   if( rc = md2GetRecord( mdl , &tag , 0 ) )goto abort ; 
   
   if( mdIsNullID( tag.head.baseID ) ){ rc = MDEnotFound ; goto abort ; }
   info.parent = tag.head.ID ;
   strcpy( info.name , tag.body.tagInode.name ) ;
   if( rc = mdDirectoryWalk(mdl,tag.head.baseID,
                            md2TagPropagateRec,(void*)&info))goto abort;

 if( md2CommitLock( mdl ))return MDEnoCommit ;
 return 0 ;
 
abort :

 md2AbortLock( mdl ) ;
 return rc ;  


}
int md2TagPropagateRec( MDL * mdl , md_id_t dirID , void *glbl , md_dir_item *item )
{
  int rc ;
  mdRecord dir , newTag ;
  md_tagprop_info  info , *pInfo = (md_tagprop_info *) glbl ;
  
  static int depth ;
  static char zeros[128] ;
    
   dir.head.ID = item -> ID ;
   if( rc = md2GetRecord( mdl , &dir , 0 ) )return 0 ; 
             /* otherwise we would loose the rest of the directory */
   if( ! ( mdIsType( dir.head.type, mdtInode     )  &&
           mdIsType( dir.head.type, mdtDirectory )     )  )return 0 ; 

   rc = depth * 3 ;
   rc = rc>127?127:rc ;
   memset( zeros , ' ' , rc ) ;
   zeros[rc] = '\0' ;

  fprintf(stdout,"dirtree : %s [%2.2d]%s %s\n",
             zeros,depth,mdStringID(item->ID),item->name );
             
  rc = md2FindTag( mdl , dir.head.ID , pInfo -> name , NULL );
  if( ! rc )return 0 ; /* don't touch existing tags */
  
  rc = md2AddNewTagInh( mdl , dir.head.ID ,
                        pInfo->parent , pInfo->name, &newTag );
  info.parent = newTag.head.ID ;
  strcpy( info.name , newTag.body.tagInode.name ) ;
  
  depth ++ ;
  if( rc = mdDirectoryWalk( mdl , item->ID ,          
                            md2TagPropagateRec , (void*)&info ) )return rc ;
  depth -- ;
  
  return 0 ;
}

int md2AddNewTagInh( MDL * mdl , md_id_t dirID , md_id_t parent , 
                     char *name , mdRecord *inTag )
{
 int rc ;
 mdRecord aTag , *tag = inTag ? inTag : &aTag ;
 
 if( md2WriteLock( mdl ))return MDEnoLock ;

   if( rc = md2CreateNewTag( mdl , tag , name ) )goto abort ;
   tag->body.tagInode.attr.flags  |= mdTagInherited;
   tag->head.parentID              = parent ;
   if( rc = md2AddTagToDirectory( mdl , dirID , tag ) )goto abort ;
 
 if( md2CommitLock( mdl ))return MDEnoCommit ;
 return 0 ;
 
abort :

 md2AbortLock( mdl ) ;
 return rc ;  
}
int md2AddNewTag( MDL * mdl , md_id_t dirID , char *name , mdRecord *tag )
{
 int rc ;
 
 if( md2WriteLock( mdl ))return MDEnoLock ;

   if( rc = md2CreateNewTag( mdl , tag , name ) )goto abort ;
   if( rc = md2AddTagToDirectory( mdl , dirID , tag ) )goto abort ;
 
 if( md2CommitLock( mdl ))return MDEnoCommit ;
 return 0 ;
 
abort :

 md2AbortLock( mdl ) ;
 return rc ;  
}
int md2AddTagToDirectory( MDL * mdl , md_id_t dirID , mdRecord *tag )
{
 mdRecord dir , base ;
 int      rc ;
 md_id_t  id ;
  
 if(md2WriteLock( mdl ))return MDEnoLock ;

    dir.head.ID = dirID ;
    if( rc  =  md2GetRecord( mdl , &dir , 0 ) )goto abort ;
    if( ! mdIsType( dir.head.type , mdtDirectory ) ){
       rc = MDEnoDir ;
       goto abort ;
    }
    if( mdIsNullID( dir.body.dirInode.attr.tag ) ){
        dir.body.dirInode.attr.tag = tag -> head.ID ;
        tag -> head.baseID = dirID ;
        if( rc  =  md2PutRecord( mdl ,  tag , 0 ) )goto abort ;
        if( rc  =  md2PutRecord( mdl , &dir , 0 ) )goto abort ;
    }else{
       for( id = dir.body.dirInode.attr.tag ;
            ! mdIsNullID( id ) ; 
            id = base.head.nextID ){
            
          base.head.ID = id ;
          if( rc = md2GetRecord( mdl , &base , 0 ) )goto abort ;
          if( ! strcmp( tag->body.tagInode.name , base.body.tagInode.name ) )
             { rc = MDEexists ; goto abort ; }
       }
       base.head.nextID = tag -> head.ID ;
       tag -> head.baseID = dirID ;
       if( rc  =  md2PutRecord( mdl ,  tag  , 0 ) )goto abort ;
       if( rc  =  md2PutRecord( mdl , &base , 0 ) )goto abort ;   
    }

 if( md2CommitLock( mdl ) )return MDEnoCommit ;
 return 0 ;
 
abort :

 md2AbortLock( mdl ) ;
 return rc ;  

} 
int md2CreateNewTag( MDL * mdl , mdRecord *newTag , char *name )
{
  md_unix  attr ;
  int rc ;
  
 if(md2WriteLock( mdl ))return MDEnoLock ;
 
    memset( (char *)&attr , 0 , sizeof( attr ) ) ;
    attr.mst_rdev    = 100 ;
    time((time_t*)& attr.mst_atime)  ;
    time((time_t*)& attr.mst_mtime) ;
    time((time_t*)& attr.mst_ctime) ;
    attr.mst_mode    = 0100444 ;
    attr.mst_nlink   = 1 ;
    attr.mst_uid     = 0 ;
    attr.mst_gid     = 0 ;
    attr.mst_size    = 0 ;
    attr.mst_blksize = 512 ;
    attr.mst_blocks  =  0 ;
    
    if( rc = md2GetNewRecord( mdl , newTag ) )goto abort ;
    mdAddType( newTag->head.type , mdtInode | mdtTag ) ;
    strncpy( newTag->body.tagInode.name , name , MAX_TAG_NAME_SIZE-1 ) ;
    newTag->body.tagInode.name[MAX_TAG_NAME_SIZE] = '\0' ;
    attr.mst_dev    = mdDeviceID( newTag->head.ID) ;
    attr.mst_ino    = mdInodeID( newTag->head.ID) ;
    memcpy((char*)&newTag->body.tagInode.attr.unixAttr,
           (char*)&attr,
           sizeof(md_unix));
    /*
     * still to be set 
     *   tag.head.baseID    =  to the parent directory
     *   tag.head.parentId  =  to our parent tag
     *   tag.head.nextID    =  to the next tag in this directory (if)
     */
 if(md2CommitLock( mdl ))return MDEnoCommit ;
 return 0 ;
 
abort :

 md2AbortLock( mdl ) ;
 return rc ;  
}
#endif

