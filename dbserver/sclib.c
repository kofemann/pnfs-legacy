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
#include <errno.h>
#include <unistd.h>
#include <strings.h>
#include "shmcom.h"
#include "md2types.h"
#include "sdef.h"
#include "sclib.h"
#include "md2log.h"
#include "allowed.h"
/*
 * this is the time we are willing to wait for
 * a free dbserver. This is NOT the time we
 * wait for the dbserver to finish OUR task.
 */
#ifndef POST_TIMEOUT
#define POST_TIMEOUT    (5)
#endif
int mdmGetRootId( SCL *scl , int db , md_id_t *id )
{
  reqGetRoot *rl  ;
  SCLIO      *sclio ;
  reqBuffer  *rb ;
  reqGetRoot *gr ;
  int         rc ;
  
  sclio = sclClientGetBuffer( scl , NULL ) ;
  rb = (reqBuffer *)sclioBuffer(sclio) ;
  gr = (reqGetRoot *)&(rb->data[0]) ;
 
  mdSetNullID( gr->config ) ;
       
  setReqBufferHead( rb , RBR_GETROOT , sizeof(reqGetRoot)  ) ;
  sclio = sclClientPostAndWait(scl ,sclio ,db  ,POST_TIMEOUT ,&rc ) ;
  if( ! sclio )return rc ;
  
  rb = (reqBuffer *)sclioBuffer(sclio) ;
  gr = (reqGetRoot *)&(rb->data[0]) ;
  if( rb->status != SRB_OK )return rb -> answer ;
  setMaxAuth( &gr -> auth ) ;
  
  if( id )*id = gr -> id ;
  return 0 ;
}
int mdmGetRootConfig( SCL *scl , int db , md_id_t *id , md_id_t *config )
{
  reqGetRoot *rl  ;
  SCLIO      *sclio ;
  reqBuffer  *rb ;
  reqGetRoot *gr ;
  int         rc ;
  
  sclio = sclClientGetBuffer( scl , NULL ) ;
  rb = (reqBuffer *)sclioBuffer(sclio) ;
  gr = (reqGetRoot *)&(rb->data[0]) ;
 
  mdSetNullID( gr->config ) ;
        
  setReqBufferHead( rb , RBR_GETROOT , sizeof(reqGetRoot)  ) ;
  sclio = sclClientPostAndWait(scl ,sclio ,db  ,POST_TIMEOUT ,&rc ) ;
  if( ! sclio )return rc ;
  
  rb = (reqBuffer *)sclioBuffer(sclio) ;
  gr = (reqGetRoot *)&(rb->data[0]) ;
  if( rb->status != SRB_OK )return rb -> answer ;
  setMaxAuth( &gr -> auth ) ;
  
  if( id )*id = gr -> id ;
  if( config )*config = gr -> config ;
  return 0 ;
}
int mdmICommand( SCL *scl , int db , char * command , int * comLength , int maxSize )
{
  reqGetRoot *rl  ;
  SCLIO      *sclio ;
  reqBuffer  *rb ;
  reqICommand *gr ;
  int         rc ;
  
  sclio = sclClientGetBuffer( scl , NULL ) ;
  rb = (reqBuffer *)sclioBuffer(sclio) ;
  gr = (reqICommand *)&(rb->data[0]) ;
 

        
  setReqBufferHead( rb , RBR_I_COMMAND , sizeof(reqICommand)  ) ;
  setMaxAuth( &gr -> auth ) ;

  gr -> size = (*comLength)  ;
  (*comLength) = 0 ;
  memcpy( gr -> command , command , gr -> size ) ;
  sclio = sclClientPostAndWait(scl ,sclio ,db  ,POST_TIMEOUT ,&rc ) ;
  if( ! sclio )return rc ;
  
  rb = (reqBuffer *)sclioBuffer(sclio) ;
  gr = (reqICommand *)&(rb->data[0]) ;
  maxSize -- ;
  if( gr -> size > 0 ){  
    maxSize =  gr->size > maxSize ? maxSize : gr -> size  ;
    memcpy( command , gr -> command , maxSize ) ;
    command[maxSize] = '\0' ;
    *comLength = gr -> size ;
  }
  return rb -> answer ;
}
int mdmLookupAuth_0( SCL *scl , md_auth * auth , md_id_t dir , char *name , md_id_t *res )
{
  SCLIO      *sclio ;
  reqBuffer  *rb ;
  reqLookup  *lu ;
  int         rc ;
  
  sclio = sclClientGetBuffer( scl , NULL ) ;
  rb = (reqBuffer *)sclioBuffer(sclio) ;
  lu = (reqLookup *)&(rb->data[0]) ;
  
  memset( (char *)&(lu->perm) , 0 , sizeof( lu->perm ) )  ;;
  strcpy( lu -> name , name ) ;
  lu -> id = dir ;
  if( auth == NULL ){
    setMaxAuth( &lu -> auth ) ;
  }else{
    lu -> auth = *auth ;
  }
  setReqBufferHead( rb , RBR_LOOKUP_ONLY , sizeof(reqLookup)  ) ;
  sclio = sclClientPostAndWait(scl ,sclio ,dir.db  ,POST_TIMEOUT ,&rc ) ;
  if( ! sclio )return rc ;
  
  rb = (reqBuffer *)sclioBuffer(sclio) ;
  lu = (reqLookup *)&(rb->data[0]) ;
  if( rb->status != SRB_OK )return rb -> answer ;
  
  if( res )*res = lu -> item.ID ;
  return 0 ;
}
int mdmForceSize( SCL *scl , md_auth *auth , md_id_t id ,
                  md_permission perm , md_long size     ) 
{
  SCLIO      *sclio ;
  reqBuffer  *rb ;
  reqForceSize *lu ;
  int         rc ;
  
  sclio = sclClientGetBuffer( scl , NULL ) ;
  rb = (reqBuffer *)sclioBuffer(sclio) ;
  lu = (reqForceSize *)&(rb->data[0]) ;
  
  if( auth )  lu -> auth  = *auth ; else  setMaxAuth( &lu -> auth ) ;
  
  lu -> id       = id ;
  lu -> perm     = perm ;
  lu -> size     = size ;
  lu -> sizeHigh = 0 ;
  
  setReqBufferHead( rb , RBR_FORCE_SIZE , sizeof(reqForceSize)  ) ;
  sclio = sclClientPostAndWait(scl ,sclio ,id.db  ,POST_TIMEOUT ,&rc ) ;
  if( ! sclio )return rc ;
  
  rb = (reqBuffer *)sclioBuffer(sclio) ;
  lu = (reqForceSize *)&(rb->data[0]) ;
  if( rb->status != SRB_OK )return rb -> answer ;
    
  return 0 ;

}       

int mdmSetAttr( SCL *scl , md_auth *auth , md_id_t id ,
                md_permission perm , md_unix  *attr      )
{
  SCLIO      *sclio ;
  reqBuffer  *rb ;
  reqSetAttr *lu ;
  int         rc ;
  
  sclio = sclClientGetBuffer( scl , NULL ) ;
  rb = (reqBuffer *)sclioBuffer(sclio) ;
  lu = (reqSetAttr *)&(rb->data[0]) ;
  
  if( auth ){
     lu -> auth  = *auth ;
  }else{
     setMaxAuth( &lu -> auth ) ;
  }
  lu -> id   = id ;
  lu -> perm = perm ;
  memcpy( (char *)&(lu->attr) , (char *)attr ,sizeof(*attr)) ;
  setReqBufferHead( rb , RBR_SET_ATTR , sizeof(reqSetAttr)  ) ;
  sclio = sclClientPostAndWait(scl ,sclio ,id.db  ,POST_TIMEOUT ,&rc ) ;
  if( ! sclio )return rc ;
  
  rb = (reqBuffer *)sclioBuffer(sclio) ;
  lu = (reqSetAttr *)&(rb->data[0]) ;
  if( rb->status != SRB_OK )return rb -> answer ;
  
  memcpy( (char *)attr , (char *)&(lu->attr) , sizeof(*attr) ) ;
  
  return 0 ;
}
int mdmSetAttrs( SCL *scl , md_auth *auth , md_id_t id ,
                 md_permission perm , md_unix  *attr      )
{
  SCLIO      *sclio ;
  reqBuffer  *rb ;
  reqSetAttr *lu ;
  int         rc ;
  
  sclio = sclClientGetBuffer( scl , NULL ) ;
  rb = (reqBuffer *)sclioBuffer(sclio) ;
  lu = (reqSetAttr *)&(rb->data[0]) ;
  
  if( auth ){
     lu -> auth  = *auth ;
  }else{
     setMaxAuth( &lu -> auth ) ;
  }
  lu -> id   = id ;
  lu -> perm = perm ;
  memcpy( (char *)&(lu->attr) , (char *)attr ,sizeof(*attr)) ;
  setReqBufferHead( rb , RBR_SET_ATTRS , sizeof(reqSetAttr)  ) ;
  sclio = sclClientPostAndWait(scl ,sclio ,id.db  ,POST_TIMEOUT ,&rc ) ;
  if( ! sclio )return rc ;
  
  rb = (reqBuffer *)sclioBuffer(sclio) ;
  lu = (reqSetAttr *)&(rb->data[0]) ;
  if( rb->status != SRB_OK )return rb -> answer ;
  
  memcpy( (char *)attr , (char *)&(lu->attr) , sizeof(*attr) ) ;
  
  return 0 ;
}
int mdmModFlags( SCL *scl , md_auth *auth , md_id_t id ,
                 md_long * flags , md_long mask      )
{
  SCLIO       *sclio ;
  reqBuffer   *rb ;
  reqModFlags *lu ;
  int         rc ;
  
  if( flags == NULL )return -1 ;
  
  sclio = sclClientGetBuffer( scl , NULL ) ;
  rb = (reqBuffer *)sclioBuffer(sclio) ;
  lu = (reqModFlags *)&(rb->data[0]) ;
  
  if( auth ){
     lu -> auth  = *auth ;
  }else{
     setMaxAuth( &lu -> auth ) ;
  }
  lu -> id   = id ;

  lu -> flags = *flags ;
  lu -> mask  = mask ;
  
  setReqBufferHead( rb , RBR_MOD_FLAGS , sizeof(reqModFlags)  ) ;
  sclio = sclClientPostAndWait(scl ,sclio ,id.db  ,POST_TIMEOUT ,&rc ) ;
  if( ! sclio )return rc ;
  
  rb = (reqBuffer *)sclioBuffer(sclio) ;
  lu = (reqModFlags *)&(rb->data[0]) ;
  if( rb->status != SRB_OK )return rb -> answer ;
  
  *flags = lu -> flags ;
  
  return 0 ;
}
int mdmModLink( SCL *scl , md_auth *auth , md_id_t id ,
                md_permission perm , int  *diff      )
{
  SCLIO      *sclio ;
  reqBuffer  *rb ;
  reqSetAttr *lu ;
  int         rc ;
  
  sclio = sclClientGetBuffer( scl , NULL ) ;
  rb = (reqBuffer *)sclioBuffer(sclio) ;
  lu = (reqSetAttr *)&(rb->data[0]) ;
  
  if( auth ){
     lu -> auth  = *auth ;
  }else{
     setMaxAuth( &lu -> auth ) ;
  }
  lu -> id   = id ;
  lu -> perm = perm ;
  
  memset( (char *)&(lu->attr) , 0 , sizeof(lu->attr)) ;
  lu -> attr.mst_nlink = *diff ;
  
  setReqBufferHead( rb , RBR_MOD_LINK , sizeof(reqSetAttr)  ) ;
  sclio = sclClientPostAndWait(scl ,sclio ,id.db  ,POST_TIMEOUT ,&rc ) ;
  if( ! sclio )return rc ;
  
  rb = (reqBuffer *)sclioBuffer(sclio) ;
  lu = (reqSetAttr *)&(rb->data[0]) ;
  if( rb->status != SRB_OK )return rb -> answer ;
  
  *diff = lu -> attr.mst_nlink ;
  
  return 0 ;
}
int mdmChParent( SCL *scl , md_auth *auth , md_id_t id ,md_id_t parent  )
{
 SCLIO      *sclio ;
  reqBuffer  *rb ;
  reqChParent  *lu ;
  int         rc ;
  
  sclio = sclClientGetBuffer( scl , NULL ) ;
  rb = (reqBuffer *)sclioBuffer(sclio) ;
  lu = (reqChParent *)&(rb->data[0]) ;
  
  if( auth ) lu -> auth  = *auth ; else setMaxAuth( &lu -> auth ) ;
  
  lu -> id       = id ;
  lu -> parent   = parent ;

  setReqBufferHead( rb , RBR_CH_PARENT , sizeof(reqChParent)  ) ;
  sclio = sclClientPostAndWait(scl ,sclio ,id.db  ,POST_TIMEOUT ,&rc ) ;
  if( ! sclio )return rc ;
  
  rb = (reqBuffer *)sclioBuffer(sclio) ;
  lu = (reqChParent *)&(rb->data[0]) ;
  if( rb->status != SRB_OK )return rb -> answer ;
  
  return 0 ;

}
int mdmDelFile( SCL *scl , md_auth *auth , md_id_t id )
{
  return mdmDelObject( scl , auth , id , RBR_OBJ_FILE  );
}
int mdmDelObject( SCL *scl , md_auth *auth , md_id_t id , int type  )
{
 SCLIO      *sclio ;
  reqBuffer  *rb ;
  reqDelObject  *lu ;
  int         rc ;
  
  sclio = sclClientGetBuffer( scl , NULL ) ;
  rb = (reqBuffer *)sclioBuffer(sclio) ;
  lu = (reqDelObject *)&(rb->data[0]) ;
  
  if( auth ) lu -> auth  = *auth ; else setMaxAuth( &lu -> auth ) ;
  
  lu -> id       = id ;
  lu -> type     = type ;

  setReqBufferHead( rb , RBR_DEL_OBJECT , sizeof(reqDelObject)  ) ;
  sclio = sclClientPostAndWait(scl ,sclio ,id.db  ,POST_TIMEOUT ,&rc ) ;
  if( ! sclio )return rc ;
  
  rb = (reqBuffer *)sclioBuffer(sclio) ;
  lu = (reqDelObject *)&(rb->data[0]) ;
  if( rb->status != SRB_OK )return rb -> answer ;
  
  return 0 ;

}
int mdmGetChain( SCL *scl , md_auth *auth , md_id_t dir , md_id_t child ,
                 md_id_t *parent , md_dir_item *item  )
{
 SCLIO         *sclio ;
  reqBuffer    *rb ;
  reqGetChain  *lu ;
  int           rc ;
  
  sclio = sclClientGetBuffer( scl , NULL ) ;
  rb = (reqBuffer *)sclioBuffer(sclio) ;
  lu = (reqGetChain *)&(rb->data[0]) ;
  
  if( auth ) lu -> auth  = *auth ; else setMaxAuth( &lu -> auth ) ;
  
  lu -> dir     = dir ;
  lu -> child   = child ;

  setReqBufferHead( rb , RBR_GET_CHAIN , sizeof(reqGetChain)  ) ;
  sclio = sclClientPostAndWait(scl ,sclio ,dir.db  ,POST_TIMEOUT ,&rc ) ;
  if( ! sclio )return rc ;
  
  rb = (reqBuffer *)sclioBuffer(sclio) ;
  lu = (reqGetChain *)&(rb->data[0]) ;
  if( rb->status != SRB_OK )return rb -> answer ;
  
  memcpy( (char*)item  , (char*)&lu->item  , sizeof(md_dir_item)) ;
  *parent = lu -> parent ;
  return 0 ;

}
int mdmRmDirEntry( SCL *scl , md_auth *auth , md_id_t dir ,
                 char *name , md_dir_item *item  )
{
 SCLIO      *sclio ;
  reqBuffer  *rb ;
  reqRmFromDir  *lu ;
  int         rc ;
  
  sclio = sclClientGetBuffer( scl , NULL ) ;
  rb = (reqBuffer *)sclioBuffer(sclio) ;
  lu = (reqRmFromDir *)&(rb->data[0]) ;
  
  if( auth ) lu -> auth  = *auth ; else setMaxAuth( &lu -> auth ) ;
  
  lu -> id   = dir ;
  memset( ( char *)&(lu -> perm) , 0 , sizeof( md_permission ) )  ;
  strcpy( lu -> name , name ) ;

  setReqBufferHead( rb , RBR_RM_FROM_DIR , sizeof(reqRmFromDir)  ) ;
  sclio = sclClientPostAndWait(scl ,sclio ,dir.db  ,POST_TIMEOUT ,&rc ) ;
  if( ! sclio )return rc ;
  
  rb = (reqBuffer *)sclioBuffer(sclio) ;
  lu = (reqRmFromDir *)&(rb->data[0]) ;
  if( rb->status != SRB_OK )return rb -> answer ;
  
  memcpy( (char*)item  , (char*)&lu->item  , sizeof(md_dir_item)) ;
  return 0 ;

}
int mdmRmDirEntryPosition( SCL *scl , md_auth *auth , md_id_t dir , md_id_t rmId , int position )
{
 SCLIO      *sclio ;
  reqBuffer  *rb ;
  reqRemovePosition  *lu ;
  int         rc ;
  
  sclio = sclClientGetBuffer( scl , NULL ) ;
  rb = (reqBuffer *)sclioBuffer(sclio) ;
  lu = (reqRemovePosition *)&(rb->data[0]) ;
  
  if( auth ) lu -> auth  = *auth ; else setMaxAuth( &lu -> auth ) ;
  
  lu -> id       = dir ;
  lu -> rmId     = rmId ;
  lu -> position = position ;
  
  setReqBufferHead( rb , RBR_RM_POSITION , sizeof(reqRemovePosition)  ) ;
  sclio = sclClientPostAndWait(scl ,sclio ,dir.db  ,POST_TIMEOUT ,&rc ) ;
  if( ! sclio )return rc ;
  
  rb = (reqBuffer *)sclioBuffer(sclio) ;
  lu = (reqRemovePosition *)&(rb->data[0]) ;
  if( rb->status != SRB_OK )return rb -> answer ;
  
  return 0 ;

}
int mdmAddDirEntry( SCL *scl , md_auth *auth , md_id_t dir , md_permission perm ,
                 char *name , md_dir_item *item   )
{
 SCLIO      *sclio ;
  reqBuffer  *rb ;
  reqAddToDir  *lu ;
  int         rc ;
  
  sclio = sclClientGetBuffer( scl , NULL ) ;
  rb = (reqBuffer *)sclioBuffer(sclio) ;
  lu = (reqAddToDir *)&(rb->data[0]) ;
  
  if( auth ) lu -> auth  = *auth ; else setMaxAuth( &lu -> auth ) ;
 
  lu -> id    = dir ;
  lu -> perm  = perm ;
  strcpy( lu -> name , name ) ;
  memcpy( (char*)&lu->item , item, sizeof(md_dir_item) ) ;

  setReqBufferHead( rb , RBR_ADD_TO_DIR , sizeof(reqAddToDir)  ) ;
  sclio = sclClientPostAndWait(scl ,sclio ,dir.db  ,POST_TIMEOUT ,&rc ) ;
  if( ! sclio )return rc ;
  
  rb = (reqBuffer *)sclioBuffer(sclio) ;
  lu = (reqAddToDir *)&(rb->data[0]) ;
  if( rb->status != SRB_OK )return rb -> answer ;
  memcpy( (char*)item  , (char*)&lu->item  , sizeof(md_dir_item)) ;
  return 0 ;

}
int mdmChangePermission( SCL *scl , md_auth *auth , md_id_t dir , 
                         char *name , md_permission perm  )
{
  md_permission tperm ;
  md_dir_item item ;
  int rc ;
  
   rc = mdmRmDirEntry( scl , auth , dir , name , &item  ) ;
   if(rc)return rc ;
   memset( (char *)&tperm , 0 , sizeof( md_permission ) ) ;
   item.perm = perm ;
   rc = mdmAddDirEntry( scl , auth , dir , tperm ,name , &item   );
   return rc ;
}
int mdmCreateFile( SCL *scl , md_auth *auth , md_id_t dir , md_permission perm ,
                 char *name , md_unix  *attr  , md_permission *resPerm ,
                 md_id_t *resID )
{
  SCLIO      *sclio ;
  reqBuffer  *rb ;
  reqMkfile  *lu ;
  int         rc ;
  
  sclio = sclClientGetBuffer( scl , NULL ) ;
  rb = (reqBuffer *)sclioBuffer(sclio) ;
  lu = (reqMkfile *)&(rb->data[0]) ;
  
  if( auth ){
     lu -> auth  = *auth ;
  }else{
     setMaxAuth( &lu -> auth ) ;
  }
  lu -> id   = dir ;
  lu -> perm = perm ;
  strcpy( lu -> name , name ) ;
  memcpy( (char *)&(lu->attr) , (char *)attr ,sizeof(*attr)) ;
      
  setReqBufferHead( rb , RBR_MKFILE , sizeof(reqMkfile)  ) ;
  sclio = sclClientPostAndWait(scl ,sclio ,dir.db  ,POST_TIMEOUT ,&rc ) ;
  if( ! sclio )return rc ;
  
  rb = (reqBuffer *)sclioBuffer(sclio) ;
  lu = (reqMkfile *)&(rb->data[0]) ;
  if( rb->status != SRB_OK )return rb -> answer ;
  
  memcpy( (char *)attr , (char *)&(lu->attr) , sizeof(*attr) ) ;
  if(resPerm)*resPerm = lu -> resPerm  ;
  *resID = lu -> resId ;
  
  return 0 ;
}
int mdmRename( SCL *scl , md_auth *auth ,
               md_id_t fDir , md_permission fPerm , char *fName ,
               md_id_t tDir , md_permission tPerm , char *tName    )
{
  SCLIO      *sclio ;
  reqBuffer  *rb ;
  reqRename  *lu ;
  int         rc ;
  
  sclio = sclClientGetBuffer( scl , NULL ) ;
  rb = (reqBuffer *)sclioBuffer(sclio) ;
  lu = (reqRename *)&(rb->data[0]) ;
  
  if( auth ){
     lu -> auth  = *auth ;
  }else{
     setMaxAuth( &lu -> auth ) ;
  }
  lu -> from.id   = fDir ;
  lu -> from.perm = fPerm ;
  strcpy( lu -> from.name , fName ) ;
  lu -> to.id   = tDir ;
  lu -> to.perm = tPerm ;
  strcpy( lu -> to.name , tName ) ;
      
  setReqBufferHead( rb , RBR_RENAME , sizeof(reqRename)  ) ;
  sclio = sclClientPostAndWait(scl ,sclio ,fDir.db  ,POST_TIMEOUT ,&rc ) ;
  if( ! sclio )return rc ;
  
  rb = (reqBuffer *)sclioBuffer(sclio) ;
  lu = (reqRename *)&(rb->data[0]) ;
  if( rb->status != SRB_OK )return rb -> answer ;
  
  return 0 ;
}
int mdmGetLink( SCL *scl , md_auth *auth , md_id_t id ,
                md_permission perm , char *path           )
{
  SCLIO      *sclio ;
  reqBuffer  *rb ;
  reqReadLink   *lu ;
  int         rc ;
  
  sclio = sclClientGetBuffer( scl , NULL ) ;
  rb = (reqBuffer *)sclioBuffer(sclio) ;
  lu = (reqReadLink  *)&(rb->data[0]) ;
  
  if( auth ){
     lu -> auth  = *auth ;
  }else{
     setMaxAuth( &lu -> auth ) ;
  }
  lu -> id   = id ;
  lu -> perm = perm ;
      
  setReqBufferHead( rb , RBR_READLINK , sizeof(reqReadLink )  ) ;
  sclio = sclClientPostAndWait(scl ,sclio ,id.db  ,POST_TIMEOUT ,&rc ) ;
  if( ! sclio )return rc ;
  
  rb = (reqBuffer *)sclioBuffer(sclio) ;
  lu = (reqReadLink  *)&(rb->data[0]) ;
  if( rb->status != SRB_OK )return rb -> answer ;
  strcpy( path , lu -> path  ) ;
  
  
  return 0 ;
}
int mdmMkLink( SCL *scl , md_auth *auth , md_id_t dir , md_permission perm ,
               char *name , md_unix  *attr  , char *path ,  md_id_t *resID )
{
  SCLIO      *sclio ;
  reqBuffer  *rb ;
  reqMakeLink   *lu ;
  int         rc ;
  
  sclio = sclClientGetBuffer( scl , NULL ) ;
  rb = (reqBuffer *)sclioBuffer(sclio) ;
  lu = (reqMakeLink  *)&(rb->data[0]) ;
  
  if( auth ){
     lu -> auth  = *auth ;
  }else{
     setMaxAuth( &lu -> auth ) ;
  }
  lu -> id   = dir ;
  lu -> perm = perm ;
  strcpy( lu -> name , name ) ;
  strcpy( lu -> path , path ) ;
  memcpy( (char *)&(lu->attr) , (char *)attr ,sizeof(*attr)) ;
      
  setReqBufferHead( rb , RBR_MAKELINK , sizeof(reqMakeLink )  ) ;
  sclio = sclClientPostAndWait(scl ,sclio ,dir.db  ,POST_TIMEOUT ,&rc ) ;
  if( ! sclio )return rc ;
  
  rb = (reqBuffer *)sclioBuffer(sclio) ;
  lu = (reqMakeLink  *)&(rb->data[0]) ;
  if( rb->status != SRB_OK )return rb -> answer ;
  
  if(resID)*resID = lu -> resID ;
  
  return 0 ;
}
int mdmMkDir( SCL *scl , md_auth *auth , md_id_t dir , md_permission perm ,
                 char *name , md_unix  *attr  , md_id_t *resID )
{
  SCLIO      *sclio ;
  reqBuffer  *rb ;
  reqMkdir   *lu ;
  int         rc ;
  
  sclio = sclClientGetBuffer( scl , NULL ) ;
  rb = (reqBuffer *)sclioBuffer(sclio) ;
  lu = (reqMkdir  *)&(rb->data[0]) ;
  
  if( auth ){
     lu -> auth  = *auth ;
  }else{
     setMaxAuth( &lu -> auth ) ;
  }
  lu -> id   = dir ;
  lu -> perm = perm ;
  strcpy( lu -> name , name ) ;
  memcpy( (char *)&(lu->attr) , (char *)attr ,sizeof(*attr)) ;
      
  setReqBufferHead( rb , RBR_MKDIR , sizeof(reqMkdir )  ) ;
  sclio = sclClientPostAndWait(scl ,sclio ,dir.db  ,POST_TIMEOUT ,&rc ) ;
  if( ! sclio )return rc ;
  
  rb = (reqBuffer *)sclioBuffer(sclio) ;
  lu = (reqMkdir  *)&(rb->data[0]) ;
  if( rb->status != SRB_OK )return rb -> answer ;
  
  memcpy( (char *)attr , (char *)&(lu->attr) , sizeof(*attr) ) ;
  *resID = lu -> resId ;
  
  return 0 ;
}
int mdmRmDir( SCL *scl , md_auth *auth , md_id_t dir ,
              md_permission perm ,  char *name         )
{
  SCLIO      *sclio ;
  reqBuffer  *rb ;
  reqRmdir   *lu ;
  int         rc ;
  md_dir_item item ;
  
  sclio = sclClientGetBuffer( scl , NULL ) ;
  rb = (reqBuffer *)sclioBuffer(sclio) ;  lu = (reqRmdir  *)&(rb->data[0]) ;
  
  if( auth ){
     lu -> auth  = *auth ;
  }else{
     setMaxAuth( &lu -> auth ) ;
  }
  lu -> id   = dir ;
  lu -> perm = perm ;
  strcpy( lu -> name , name ) ;
      
  setReqBufferHead( rb , RBR_RMDIR , sizeof(reqRmdir )  ) ;
  sclio = sclClientPostAndWait(scl ,sclio ,dir.db  ,POST_TIMEOUT ,&rc ) ;
  if( ! sclio )return rc ;
  
  rb = (reqBuffer *)sclioBuffer(sclio) ;
  lu = (reqRmdir  *)&(rb->data[0]) ;
  if( rb->status != SRB_OK ){

    if( rb -> answer == MDEdbMissmatch ){
      if( auth -> priv != 15 )return MDEnotAllowed ;
      rc = mdmLookupAuth_1( scl , auth , dir ,  perm ,name , &item , NULL  );
      if( rc  )return rc ;
      rc = mdmDeleteDirectory( scl , auth , item.ID ) ;
      if( rc )return rc ;
      rc = mdmRmDirEntry( scl , auth , dir , name , &item  );
      return rc;
    }else{
      return rb -> answer ;
    }
  }
  return 0 ;
}
int mdmRmFile( SCL *scl , md_auth *auth , md_id_t dir ,
               md_permission perm ,  char *name         )
{
  SCLIO      *sclio ;
  reqBuffer  *rb ;
  reqRmfile  *lu ;
  int         rc ;
  
  sclio = sclClientGetBuffer( scl , NULL ) ;
  rb = (reqBuffer *)sclioBuffer(sclio) ;
  lu = (reqRmfile  *)&(rb->data[0]) ;

  if( auth ){
     lu -> auth  = *auth ;
  }else{
     setMaxAuth( &lu -> auth ) ;
  }
  lu -> id   = dir ;
  lu -> perm = perm ;
  strcpy( lu -> name , name ) ;
      
  setReqBufferHead( rb , RBR_RMFILE , sizeof(reqRmfile )  ) ;
  sclio = sclClientPostAndWait(scl ,sclio ,dir.db  ,POST_TIMEOUT ,&rc ) ;
  if( ! sclio )return rc ;
  
  rb = (reqBuffer *)sclioBuffer(sclio) ;
  lu = (reqRmfile  *)&(rb->data[0]) ;
  if( rb->status != SRB_OK )return rb -> answer ;
  
  return 0 ;
}
int mdmLookupAuth_1( SCL *scl , md_auth * auth , md_id_t dir , md_permission perm ,
                 char *name , md_dir_item *item , md_unix  *attr  )
{
  SCLIO      *sclio ;
  reqBuffer  *rb ;
  reqLookup  *lu , luBase ;
  int         rc ;
  md_dir_item tmpItem ;
  
  sclio = sclClientGetBuffer( scl , NULL ) ;
  rb = (reqBuffer *)sclioBuffer(sclio) ;
  lu = (reqLookup *)&(rb->data[0]) ;
  
  lu -> id   = dir ;
  lu -> perm = perm ;
  strcpy( lu -> name , name ) ;
  if( auth == NULL ){
     setMaxAuth( &lu -> auth ) ;
  }else{
     lu -> auth = *auth ;
  }
  setReqBufferHead( rb , RBR_LOOKUP , sizeof(reqLookup)  ) ;
  sclio = sclClientPostAndWait(scl ,sclio ,dir.db  ,POST_TIMEOUT ,&rc ) ;
  if( ! sclio )return rc ;
  
  rb = (reqBuffer *)sclioBuffer(sclio) ;
  lu = (reqLookup *)&(rb->data[0]) ;

  if( rb->status != SRB_OK ){
     if( rb -> answer == MDEdbMissmatch ){
        memcpy( (char*)&tmpItem,(char*)&lu -> item,sizeof(md_dir_item));
        rc = mdmGetAttrAuth( scl , auth , tmpItem.ID ,tmpItem.perm ,attr);
        if(rc)return rc ;
        /* since we the default mount option is 30.
           we need not reset the noio bit
           memset((char*)&tmpItem.perm,0,sizeof(tmpItem.perm));*/
        if( item )*item = tmpItem ;
        return 0 ;
     }else if( rb -> answer == MDEdbXsearch ){
        char *cursor ;
        md_id_t id ;
        memcpy( (char *)&luBase, (char *)lu , sizeof( luBase ) ) ;
        
        for( cursor = luBase.name , id = luBase.item.ID ; 
             strlen( cursor ) ;
             cursor += ( strlen( cursor ) + 1 )  ){
             if( rc = mdmLookupAuth_0( scl , auth , id , cursor , &id ) )return rc ;     
        }
        rc = mdmGetAttrAuth( scl , auth , id ,perm ,attr);
        if(rc)return rc ;
        if( item ){
           item->ID   = id ;
           item->perm = perm ;
        }
        return 0 ;
     }else{
        return rb -> answer ;
     }
  }
  if( item )*item = lu -> item ;
  if( attr )*attr = lu -> attr ;
  return 0 ;
}
int mdmGetAttrAuth( SCL *scl , md_auth * auth ,  md_id_t id , md_permission perm , md_unix *attr )
{
       
   return mdmGetExtAttrAuth( scl , auth , id , perm , attr , NULL ) ;
}
int mdmGetExtAttrAuth( SCL *scl , md_auth * auth ,  md_id_t id , md_permission perm ,
                      md_unix *attr , mdRecord *rec )
{
  SCLIO *sclio = sclClientGetBuffer( scl , NULL )  ;
  reqBuffer *rb ;
  char *error ;
  int rc ;
  reqGetAttr *ga ;
        
        rb = (reqBuffer *)sclioBuffer(sclio) ;
        setReqBufferHead( rb , RBR_GET_ATTR , 0 ) ;
        ga = (reqGetAttr *)&(rb->data[0]) ;

        setMaxAuth( &ga -> auth ) ;
        
        ga->id      = id ; 
        ga -> perm = perm ;
        if( auth != NULL )ga -> auth = *auth ;
        rb->size = sizeof(reqGetAttr) ;
        
        sclio = sclClientPostAndWait(scl ,sclio ,ga->id.db  ,POST_TIMEOUT ,&rc ) ;
        if( ! sclio )return rc ;
        
        rb = (reqBuffer *)sclioBuffer(sclio) ;
        if( rb->status == SRB_OK){
           if(attr)*attr = ga -> attr.unixAttr ;
           if(rec)memcpy((char*)rec,(char*)&(ga->rec),sizeof(mdRecord));
           rc = 0 ;
        }else rc = rb -> answer ;
        
        return rc ;
}
int mdmGetRecord( SCL *scl , md_id_t id ,  mdRecord *rec )
{
  SCLIO *sclio = sclClientGetBuffer( scl , NULL )  ;
  reqBuffer *rb ;
  int rc ;
  mdRecord *mdr ;
        
        rb = (reqBuffer *)sclioBuffer(sclio) ;
        setReqBufferHead( rb , RBR_GET_RECORD , 0 ) ;
        mdr = (mdRecord *)&(rb->data[0]) ;
        mdr -> head.ID = id ;
                
        sclio = sclClientPostAndWait(scl ,sclio ,mdr -> head.ID.db  ,POST_TIMEOUT ,&rc ) ;
        if( ! sclio )return rc ;
        
        rb = (reqBuffer *)sclioBuffer(sclio) ;
        if( rb->status == SRB_OK){
           memcpy( (char *)rec , (char *)mdr , sizeof(mdRecord ) ) ;
           rc = 0 ;
        }else rc = rb -> answer ;
        
        return rc ;
}
int mdmTruncate( SCL *scl , md_id_t id , mdPermission perm   )
{
  SCLIO *sclio = sclClientGetBuffer( scl , NULL )  ;
  reqBuffer *rb ;
  char *error ;
  int rc ;
  reqTruncate *ga ;
        
        rb = (reqBuffer *)sclioBuffer(sclio) ;
        setReqBufferHead( rb , RBR_TRUNCATE , 0 ) ;
        ga = (reqTruncate *)&(rb->data[0]) ;
        
        ga -> id      = id ;
        ga -> perm    = perm ;
        rb -> size    = sizeof(reqTruncate) ;
        setMaxAuth( &ga -> auth ) ;
        
        sclio = sclClientPostAndWait(scl ,sclio ,ga->id.db  ,POST_TIMEOUT ,&rc ) ;
        if( ! sclio )return rc ;
        
        rb = (reqBuffer *)sclioBuffer(sclio) ;
        if( rb->status == SRB_OK){
           rc = 0 ;
        }else rc = rb -> answer ;
        
        return rc ;
}
int mdmReadLink( SCL *scl ,md_id_t id ,mdPermission perm ,char *path  )
{
  SCLIO       *sclio ;
  reqBuffer   *rb ;
  reqReadLink *rl  ;
  int          rc ;

        sclio = sclClientGetBuffer( scl , NULL ) ;
        
        rb = (reqBuffer *)sclioBuffer(sclio) ;
        setReqBufferHead( rb , RBR_READLINK,sizeof(reqReadLink)  ) ;
        rl = (reqReadLink *)&(rb->data[0]) ;
        rl-> id     = id ;
        rl -> perm  = perm ; 

        setMaxAuth( &rl -> auth ) ;

        
        sclio = sclClientPostAndWait(scl ,sclio ,rl-> id.db  ,POST_TIMEOUT ,&rc ) ;
        if( ! sclio )return rc ;
        
        rb = (reqBuffer *)sclioBuffer(sclio) ;
        if( rb->status == SRB_OK){
           rc = 0 ;
           strcpy( path , rl -> path ) ;
        }else rc = rb -> answer ;
 
        return rc ;
}
int mdmReadDirAuth( SCL *scl ,md_auth *auth , md_id_t id ,mdPermission perm ,
                    long cookie , int count , reqExtItem *extItem  )
{
  SCLIO       *sclio ;
  reqBuffer   *rb ;
  reqReadDir  *rl  ;
  int          rc , i ;

        sclio = sclClientGetBuffer( scl , NULL ) ;
        
        rb = (reqBuffer *)sclioBuffer(sclio) ;
        setReqBufferHead( rb , RBR_READDIR,sizeof(reqReadDir)  ) ;
        rl = (reqReadDir *)&(rb->data[0]) ;
        rl -> id     = id ;
        rl -> perm  = perm ; 
        rl -> auth  = *auth ;
        rl -> cookie  = cookie ;
        rl -> count   = count ;
        
        sclio = sclClientPostAndWait(scl ,sclio ,rl-> id.db  ,POST_TIMEOUT ,&rc ) ;
        if( ! sclio )return rc ;
        
        rb = (reqBuffer *)sclioBuffer(sclio) ;
        if( rb->status == SRB_OK){
           rl = (reqReadDir *)&(rb->data[0]) ;
           for( i = 0 ; i < rl -> count ; i ++ )
             memcpy( (char *)&extItem[i] , (char *)&(rl -> e[i]),sizeof(reqExtItem));
           rc = rl -> count ;
        }else rc = rb -> answer ;
 
        return rc ;
}
int mdmFindIdAuth( SCL *scl , md_auth *auth , md_id_t id , mdPermission perm ,
                   md_id_t parentId , int size ,  char *data , md_unix *uattr ){


  SCLIO       *sclio ;
  reqBuffer   *rb ;
  reqFindId   *rl  ;
  int          rc ;

        sclio = sclClientGetBuffer( scl , NULL ) ;
        
        rb = (reqBuffer *)sclioBuffer(sclio) ;
        setReqBufferHead( rb , RBR_FIND_ID,sizeof(reqFindId)  ) ;
        rl = (reqFindId *)&(rb->data[0]) ;
        rl -> id       = id ;
        rl -> perm     = perm ; 
        rl -> parentId = parentId ;
        rl-> offset    = 0 ;
        rl-> size      = size > MAX_REQ_DATA_SIZE ? MAX_REQ_DATA_SIZE : size ;
        if( auth ){
          rl -> auth  = *auth ;
        }else{
           setMaxAuth( &rl -> auth ) ;
        }

        
        sclio = sclClientPostAndWait(scl ,sclio ,rl-> parentId.db  ,15 ,&rc ) ;
        if( ! sclio )return rc ;
        
        rb = (reqBuffer *)sclioBuffer(sclio) ;
        if( rb->status == SRB_OK){
           rc = rl -> size ;
           if( rl -> size > 0 )memcpy( data ,  rl -> data , rl -> size ) ;
           if(uattr)memcpy((char*)uattr,(char*)&(rl->attr),sizeof(md_unix));
        }else rc = rb -> answer ;
 
        return rc ;

}
int mdmReadData( SCL *scl , md_id_t id , mdPermission perm , 
                 long offset , long size , char *data       )
{
  return mdmReadDataAuth(scl,NULL,id , perm ,offset , size , data , NULL) ;
}
int mdmReadDataAuth( SCL *scl ,md_auth *auth ,  md_id_t id , mdPermission perm , 
                     long offset , long size , char *data  , md_unix *uattr     )
{
  SCLIO       *sclio ;
  reqBuffer   *rb ;
  reqReadData *rl  ;
  int          rc ;

        sclio = sclClientGetBuffer( scl , NULL ) ;
        
        rb = (reqBuffer *)sclioBuffer(sclio) ;
        setReqBufferHead( rb , RBR_READDATA,sizeof(reqReadData)  ) ;
        rl = (reqReadData *)&(rb->data[0]) ;
        rl-> id     = id ;
        rl-> offset = offset ;
        rl-> size   = size > MAX_REQ_DATA_SIZE ? MAX_REQ_DATA_SIZE : size ;
        rl -> perm  = perm ; 
        if( auth ){
          rl -> auth  = *auth ;
        }else{
           setMaxAuth( &rl -> auth ) ;
        }

        
        sclio = sclClientPostAndWait(scl ,sclio ,rl-> id.db  ,POST_TIMEOUT ,&rc ) ;
        if( ! sclio )return rc ;
        
        rb = (reqBuffer *)sclioBuffer(sclio) ;
        if( rb->status == SRB_OK){
           rc = rl -> size ;
           if( rl -> size > 0 )memcpy( data ,  rl -> data , rl -> size ) ;
           if(uattr)memcpy((char*)uattr,(char*)&(rl->attr),sizeof(md_unix));
        }else rc = rb -> answer ;
 
        return rc ;
}
int mdmWriteData( SCL *scl , md_id_t id , mdPermission perm , 
                  long offset , long size , char *data       )
{
  return mdmWriteDataAuth(scl,NULL,id , perm ,offset , size , data , NULL) ;
}
int mdmWriteDataAuth( SCL *scl ,md_auth *auth , md_id_t id , mdPermission perm , 
                  long offset , long size , char *data  , md_unix *uattr )
{
  SCLIO        *sclio ;
  reqBuffer    *rb ;
  reqWriteData *rl  ;
  int           rc  ;

        sclio = sclClientGetBuffer( scl , NULL ) ;
        
        rb = (reqBuffer *)sclioBuffer(sclio) ;
        setReqBufferHead( rb , RBR_WRITEDATA , sizeof(reqWriteData)  ) ;
        rl = (reqWriteData *)&(rb->data[0]) ;
        rl-> id     = id ;
        rl-> offset = offset ;
        rl-> size   = size > MAX_REQ_DATA_SIZE ? MAX_REQ_DATA_SIZE : size ;
        rl-> perm   = perm ; 
        memcpy( rl -> data , data , rl -> size ) ;

        if( auth ){
          rl -> auth  = *auth ;
        }else{
          setMaxAuth( &rl -> auth ) ;
        }


        
        sclio = sclClientPostAndWait(scl ,sclio ,rl-> id.db  ,POST_TIMEOUT ,&rc ) ;
        if( ! sclio )return rc ;
        
        rb = (reqBuffer *)sclioBuffer(sclio) ;
        if( rb->status == SRB_OK){
           rc = rl -> size ;
           if(uattr){
              memcpy((char*)uattr,(char*)&(rl->attr),sizeof(md_unix));
              /*
              md2pPrintf(md2pMODINFO,"mdmWriteDataAuth uid=%d;gid=%d;size=%d\n",
                       uattr->mst_uid,uattr->mst_gid,uattr->mst_size);
              */
           }
        }else rc = rb -> answer ;
        
  return rc ;

}
int mdmGetObjectID( SCL *scl , char *path , md_id_t *res )
{
 
  int rc , i , sl , p , db ;
  md_id_t current ;
  char  string[1024] , *s , *cptr[128] ;
  

  strncpy( string , path , sizeof( string) -1  ) ;
  if( ( strlen(string) < 1 ) || ( string[0] != '/' ) )return -1 ;
  cptr[0] = &string[0] ;
  for( i = 0 , p = 0 , sl = strlen( string ) ; i < sl ; i++ ){
     if( string[i] == '/' ){
         cptr[p++] = &string[i+1] ;
         string[i] = '\0' ;
     }
  }
  if( ( p > 0 )  && ( strlen( cptr[p-1] ) == 0 ) )p-- ; 
  if( p <= 0 )return 0 ;
  
  if( strlen( cptr[0] ) == 0 ) {
     db = 0 ; 
     i = 1 ;
  }else if( ( *cptr[0] >= '0' ) && ( *cptr[0] <= '9' ) ){
     sscanf( cptr[0] , "%d" , &db ) ;
     i = 1 ;
  }else{
     i = 0 ;
     db = 0 ;
  }
  if( rc = mdmGetRootId( scl , db , &current ) )return rc ;
  for( ; i < p ; i++ ){
     if( rc = mdmLookupAuth_0( scl , NULL , current , cptr[i] , &current ) )return rc ;
  }
  /*printf( "%s %s\n",mdStringID(current),argv[3]);*/
  if(res) *res = current ;
  return 0 ; 
 
} 

/*
 * ============================================================================
 *
 *   file io simulation based on shm client  calls
 */
char *scFs_Ffgets(char *s, int n, sc_fs_format *f)
{
  int   c , i ;
  char *x ;
  
   for( i=0 , x = s , n-- ; i<n ; i++ , x++ ){
   
      c = scFs_Ffgetc( f ) ;
	  if( c == EOF ){ *x = '\0' ; return i==0?NULL:s; }
	  *x = c ;
	  if( c == '\n' )break ;
	  
   }
   x++ ;
   *x = '\0' ;
   return s ;
}
int scFs_Ffgetc( sc_fs_format *f )
{
  int rc ;
  
   if( f->offset >= f->size ){
      if( f->size < f->maxSize )return EOF ;
      rc = scFs_Read( &f->block , f->buffer , f-> maxSize );
      if( rc <= 0 )return rc ;
	  f -> size   = rc ;
      f -> offset = 0 ;
   }
   return f -> buffer[f->offset++] ;
}
int scFs_Ffputs(const char *s, sc_fs_format *f)
{
  for( ; *s != '\0' ; s++ )scFs_Ffputc( (int)*s , f );
  return 0 ;
}
int scFs_Ffputc( int c , sc_fs_format *f )
{
  int rc ;
  
   f -> buffer[f->offset++] = c ;
   if( f->offset >= f->maxSize ){
      rc = scFs_Write( &f->block , f->buffer , f-> maxSize );
      if( rc < 0 )return rc ;
      f -> offset = 0 ;
   }
   return 0 ;
}
int scFs_FOpen( SCL *scl , sc_fs_format *f , md_id_t id , int level , char *mode )
{
  int rc ;
  
  f-> maxSize = SC_FS_BUFFER_SIZE ;
  f-> size    = 0 ;
  f-> offset  = 0 ;
  if( rc = scFs_Open( scl , &f->block , id , level , mode ) )return rc ;
  if(  f->block.mode  == scFsREAD ){
     /*
      *   read ahead
      */
     rc = scFs_Read( &f->block , f->buffer , f-> maxSize );
     if( rc < 0 )return rc ;
     f -> size = rc ;
   }else if( f->block.mode  == scFsWRITE ){
   }
  
  return 0 ;
}
int scFs_Open( SCL *scl , sc_fs_block *p , md_id_t id , int level , char *mode )
{
#define max_link_follow   (20)
  md_unix uattr ;
  int     rc , i ;
  mdPermission perm ;
  md_dir_item item ;
  mdRecord rec ;
  md_id_t resID ;
  char path[1024] ;
  md_unix attr ;
  
  p->scl    = scl ;
  p->level  = level ;
  p->id     = id ;

  memset( (char*)&perm , 0 , sizeof(perm) );
  mdpSetLevel( perm , p->level ) ;
  if( strchr( mode , 'l' ) ){
    for(i=0;i<max_link_follow;i++){
       if( rc = mdmGetAttrAuth( p->scl , NULL ,  p->id , perm , &uattr) )return rc ;
       if( ( uattr.mst_mode & 0170000 ) != 0120000 )break ;
       if( rc = mdmGetRecord( p->scl , p->id , &rec ) )return rc ;
       if( rc = mdmReadLink( p->scl , p->id , perm , path ))return rc ;
       if( rc = mdmLookupAuth_1( p->scl , NULL , rec.head.parentID ,
                             perm , path , &item , &attr ) )return rc ;
            
       p -> id = item.ID ;
    }
    if( i == max_link_follow )return -2 ;
  }
  if( strchr( mode , 'r' ) ){
     if( rc = mdmGetAttrAuth( p->scl , NULL ,  p->id , perm , &uattr) )return rc ;
     if( ( uattr.mst_mode & 0170000 ) != 0100000 )return -10 ;
     p->size   = uattr.mst_size ;
     p->mode   = scFsREAD ;
     p->offset = 0 ;
  }else if( strchr( mode , 'w' ) ){
     mdpSetLevel( perm , p->level ) ;
     if( rc = mdmTruncate( p->scl , p->id , perm ))return rc ;
     p->size   = 0 ;
     p->mode   = scFsWRITE ;
     p->offset = 0 ;
  }else return -1 ;
  return 0 ;
}
int scFs_Write( sc_fs_block *p , char *buffer , long size )
{
  int rc ;
  mdPermission perm ;
     
  memset((void*)(&perm),0,sizeof(perm)) ;
  
  mdpSetLevel( perm , p->level ) ;
  
  rc = mdmWriteData( p->scl , p->id , perm , p->offset , size , buffer );
  if( rc < 0 )return rc ;
  p->offset += size ;
  
  return size ;

}
int scFs_Read( sc_fs_block *p , char *buffer , long size )
{
  int rc ;
  mdPermission perm ;
  
  memset((void*)(&perm),0,sizeof(perm)) ;
  
  mdpSetLevel( perm , p->level ) ;
  
  size = md_min( p->size - p->offset , size ) ;
  if( size <= 0 )return 0 ;
  rc = mdmReadData( p->scl , p->id , perm , p->offset , size , buffer );
  /*fprintf(stderr, " offset %d size %d\n" , p->offset , size ) ;*/
  if( rc < 0 )return rc ;
  p->offset += size ;
  return size ;
}
