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
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "shmcom.h"
#include "md2types.h"
#include "sdef.h"
#include "sclib.h"

typedef int (*sc_func)( int argc , char *argv[] , SCL *scl ) ;

typedef struct _func_item {
   char    *name ;
   sc_func  func ;
   char    *help ;
} scFuncItem ;


int sc_whatever( int argc , char *argv[] , char *doit ,
                 SCL *scl , scFuncItem *list ) ;
int sc_flag( int argc , char *argv[] , SCL *scl );
int sc_getattr( int argc , char *argv[] , SCL *scl );
int sc_getrecord( int argc , char *argv[] , SCL *scl );
int sc_dummy( int argc , char *argv[] , SCL *scl );
int sc_lookup( int argc , char *argv[] , SCL *scl );
int sc_readdir( int argc , char *argv[] , SCL *scl );
int sc_setattr( int argc , char *argv[] , SCL *scl );
int sc_mkdir( int argc , char *argv[] , SCL *scl );
int sc_mkfile( int argc , char *argv[] , SCL *scl );
int sc_rmdir( int argc , char *argv[] , SCL *scl );
int sc_rmfile( int argc , char *argv[] , SCL *scl );
int sc_rename( int argc , char *argv[] , SCL *scl );
int sc_readlink( int argc , char *argv[] , SCL *scl );
int sc_makelink( int argc , char *argv[] , SCL *scl );
int sc_writedata( int argc , char *argv[] , SCL *scl );
int sc_readdata( int argc , char *argv[] , SCL *scl );
int sc_setsize( int argc , char *argv[] , SCL *scl );
int sc_setperm( int argc , char *argv[] , SCL *scl );
int sc_modlink( int argc , char *argv[] , SCL *scl );
int sc_getroot( int argc , char *argv[] , SCL *scl );
int sc_modflags( int argc , char *argv[] , SCL *scl );
int sc_setperm( int argc , char *argv[] , SCL *scl );
int sc_adddirentry( int argc , char *argv[] , SCL *scl );
int sc_rmdirentry( int argc , char *argv[] , SCL *scl );
int sc_rmdirentrypos( int argc , char *argv[] , SCL *scl );
int sc_chparent( int argc , char *argv[] , SCL *scl );
int sc_icommand( int argc , char *argv[] , SCL *scl );
/*
 * macros are commands which need more than a single
 * shm call. In principle they are no longer concurreny 
 * save. But nfs does it the same way.
 */
int sc_macro_ls( int argc , char *argv[] , SCL *scl );
int sc_macro_getid( int argc , char *argv[] , SCL *scl );
int sc_macro_copy( int argc , char *argv[] , SCL *scl );

int scCopyFromPnfs( SCL *scl , char *name ,  md_id_t id , int level );
int scCopyToPnfs( SCL *scl , char *name ,  md_id_t id , int level );
int scPnfsToPnfs( SCL *scl , md_id_t fromId , int fromLevel ,
                             md_id_t toId , int toLevel       );
int mayBeId( char *id );

scFuncItem scFuncList[] = {

{ "ls"         , sc_macro_ls      , "<id>" } ,
{ "getid"      , sc_macro_getid   , "<path>" } ,
{ "copy"       , sc_macro_copy    , "<id> <Xpath>" } ,
{ "copy"       , sc_macro_copy    , "<Xpath> <id>" } ,
{ "lookup"     , sc_lookup        , "<id> <name> [<level>]" } ,
{ "setsize"    , sc_setsize       , "<id> <fromName> <size>" } ,
{ "rename"     , sc_rename        , "<fromDirID> <fromName> <toDirId> <toName>" } ,
{ "getattr"    , sc_getattr       , "<id> [<level>]" } ,
{ "readdir"    , sc_readdir       , "<id> [<cookie>]" } ,
{ "setattr"    , sc_setattr       , "<id> <level> <uid> <gid> <perm>" } ,
{ "modflags"   , sc_modflags      , "<id> <newFlags> <flagMask>" } ,
{ "flag"       , sc_flag          , "<id> remove|move|sec|security on|off" } ,
{ "setperm"    , sc_setperm       , "<dirID> <name> <perm>" } ,
{ "rmfile"     , sc_rmfile        , "<id> <name>" } ,
{ "rmdir"      , sc_rmdir         , "<id> <name>" } ,
{ "mkdir"      , sc_mkdir         , "<id> <name> <uid> <gid> <perm>" } ,
{ "mkfile"     , sc_mkfile        , "<id> <name> <uid> <gid> <perm>" } ,
{ "getroot"    , sc_getroot       , "<db> | <db> <newConfigID>" } ,
{ "getrecord"  , sc_getrecord     , "[<slot>]" } ,
{ "makelink"   , sc_makelink      , "" } ,
{ "readlink"   , sc_readlink      , "" } ,
{ "readdata"   , sc_readdata      , "" } ,
{ "writedata"  , sc_writedata     , "" } ,
{ "setperm"    , sc_setperm       , "<dirID> <objName> <perm>" } ,
{ "modlink"    , sc_modlink       , "<fileID> <linkDiff>" } ,
{ "adddirentry"    , sc_adddirentry  , "<dirID> <name> <id>" } ,
{ "rmdirentry"     , sc_rmdirentry   , "<dirID> <name>" } ,
{ "rmdirentrypos"  , sc_rmdirentrypos   , "<dirID> <rmID> <position>" } ,
{ "chparent"       , sc_chparent     , "<objectId> <parentId>" } ,
{ "exec"       , sc_icommand      , "<dbid> <args... >" } ,
{ "dummy"      , sc_dummy         , "<key> <slot>" } ,
{ NULL , NULL , NULL } 

} ;
 
main( int argc ,char *argv[] )
{
  key_t key ;
  int rc , slot  ;
  SCL *scl ;
  SCLIO *sclio ;
  reqBuffer *rb ;
  char *error ;
  
  scl = NULL ;
  if( argc < 3 ){
    sc_whatever( argc , argv , NULL , NULL , scFuncList ) ;
    exit(1) ;
  }
  
   rc = 0 ;
   sscanf( argv[2] , "%x" , &key ) ;
   if( ! ( scl   = sclClientOpen(  key , 8*1024 , &rc ) ) ){
      error = sclError( "sclClientOpen" , rc ) ;
      fprintf(stderr," Problem : %s(%d)\n",error,errno);
      exit(2) ;
   }
  

   rc = sc_whatever( argc , argv , argv[1] , scl , scFuncList ) ;
   if( rc == -22 )rc = 1 ;
   else if( rc ){
     fprintf(stderr,"problem %d detected\n",rc);
     rc = 3 ;
   }
   (void)sclClientClose( scl );
   exit(rc) ;
 

}
int sc_whatever( int argc , char *argv[] , char *doit , SCL *scl , scFuncItem *list )
{
   scFuncItem *cursor ;
   int rc ;
   
   if(  doit == NULL ){
   
      for( cursor = list ;  cursor -> name != NULL  ;  cursor ++ ) 
         fprintf(stderr," USAGE : %s %s <shmkey> %s\n",
                 argv[0],cursor->name,cursor->help      ) ;
      return -22 ;
   
   }
   for( cursor = list ; 
        ( cursor -> name != NULL ) && ( strcmp( cursor->name, doit ) ) ;
       cursor ++ ) ;
 
   if( cursor -> name == NULL )return -23 ;
   
   rc = (*(cursor->func))( argc , argv , scl ) ; 
   if( rc == -22 ){
   
      fprintf(stderr," USAGE : %s %s <shmkey> %s\n",
              argv[0],cursor->name,cursor->help      ) ;
              
   }
   return rc ;
}
int sc_dummy( int argc , char *argv[] , SCL *scl )
{
  SCLIO *sclio = sclClientGetBuffer( scl , NULL )  ;
  reqBuffer *rb ;
  char *error ;
  int rc ;
  int slot ;
  
        if( argc < 4 ){
            slot = 0 ;
        }else{
           sscanf( argv[3] , "%d" , &slot ) ;
        }
        rb = (reqBuffer *)sclioBuffer(sclio) ;
        setReqBufferHead( rb , RBR_DUMMY , 0 ) ;
        showReqBufferHead( rb ) ;
        
        sclio = sclClientPostAndWait( scl , sclio , slot , 10 , &rc ) ;
        if( ! sclio ){
            if( rc == SCL_TIMEOUT ){
               fprintf(stderr," sclClientPostAndWait : Timeout\n");
            }else{
               error = sclError( "sclClientPostAndWait" , rc ) ;
               fprintf(stderr," Problem : %s(%d)\n",error,errno);
            }
        }else{
           rb = (reqBuffer *)sclioBuffer(sclio) ;
           showReqBufferHead( rb ) ;
        }
        return 0 ;
}
int sc_icommand( int argc , char *argv[] , SCL *scl )
{
 int i , dbid , rc ;
 char command[2048]  , *cur ;
 int replyLen = 0 ;
 
    if( argc < 5 )return -22 ;
    sscanf( argv[3] , "%d" , &dbid ) ;

    for( i = 4 , cur = command ; i < argc ; i++ ){
       strcpy( cur , argv[i] ) ;
       cur += ( strlen( argv[i] ) + 1 ) ; 
    }
    *cur = '\0' ; cur++ ;
    
    replyLen = cur  -  command ;
    rc = mdmICommand( scl , dbid , command , &replyLen , 2048 ) ;
    
    if( replyLen > 0 )fprintf( rc ? stderr : stdout ,"%s\n", command ) ;
    return rc;

}
int sc_adddirentry( int argc , char *argv[] , SCL *scl )
{
 md_id_t dirID , id ;
 int rc ;
 char *name ;
 md_dir_item item ;
 md_permission perm ;

 
    if( argc < 6 )return -22 ;
    md2ScanId( argv[3] , &dirID ) ;
    name = argv[4] ;
    md2ScanId( argv[5] , &id ) ;
    
    memset((char*)&perm,0,sizeof(perm));
    memset((char*)&item,0,sizeof(item));
    
    item.ID = id ;
    strcpy( item.name , name );
    return mdmAddDirEntry( scl , NULL , dirID , perm ,  name , &item   ) ;
    

}
int sc_modlink( int argc , char *argv[] , SCL *scl )
{
 md_id_t fileID ;
 int rc , diff  ;
 md_permission perm ;

 
    if( argc < 5 )return -22 ;
    md2ScanId( argv[3] , &fileID ) ;
    sscanf( argv[4] , "%d" , &diff ) ;
    
    rc = mdmModLink( scl , NULL , fileID , perm ,  &diff   ) ;
    if( rc )return rc ;
    printf( "New nlink of %s : %d\n" , mdStringID(fileID) , diff ) ;
    return 0 ;
}
int sc_rmdirentry( int argc , char *argv[] , SCL *scl )
{
 md_id_t dirID , id ;
 int rc ;
 char *name ;
 md_dir_item item ;

 
    if( argc < 5 )return -22 ;
    md2ScanId( argv[3] , &dirID ) ;
    name = argv[4] ;
        
    return mdmRmDirEntry( scl , NULL , dirID , name , &item   ) ;
    

}
int sc_rmdirentrypos( int argc , char *argv[] , SCL *scl )
{
 md_id_t dirID , rmID ;
 int position ;

 
    if( argc < 6 )return -22 ;
    md2ScanId( argv[3] , &dirID ) ;
    md2ScanId( argv[4] , &rmID ) ;
    sscanf( argv[5] , "%d" , &position ) ;
        
    return mdmRmDirEntryPosition( scl , NULL , dirID , rmID , position   ) ;
    

}
int sc_chparent( int argc , char *argv[] , SCL *scl )
{
 md_id_t id , parent ;
 int rc ;

 
    if( argc < 5 )return -22 ;
    md2ScanId( argv[3] , &id ) ;
    md2ScanId( argv[4] , &parent ) ;
        
    return mdmChParent( scl , NULL , id , parent  ) ;

}

/*
int sc_setperm( int argc , char *argv[] , SCL *scl )
{
 md_id_t dirID ;
 md_permission perm ;
 int rc ;
 char *name ;
    if( argc < 5 )return -22 ;
    md2ScanId( argv[2] , &dirID ) ;
    name = argv[3] ;
    md2ScanPermission( argv[4] , &perm ) ;
    
return mdmChangePermission( scl , NULL , dirID , name ,  perm  );
    

}
*/
#ifdef use_sclib

int sc_getattr( int argc , char *argv[] , SCL *scl )
{
  int rc ;
  md_permission perm;
  md_id_t  id ;
  md_unix attr  ;
        
        if( argc < 4 )return -22 ;
        if( argc == 4 ){
            md2ScanPermission( "0" , &perm ) ;
        }else{
            md2ScanPermission( argv[4] , &perm ) ;
        }
        md2ScanId( argv[3] , &id ) ;
        rc = mdmGetExtAttr( scl , id , perm ,&attr , NULL );
        return rc ;
}
#else
int sc_getattr( int argc , char *argv[] , SCL *scl )
{
  SCLIO *sclio = sclClientGetBuffer( scl , NULL )  ;
  reqBuffer *rb ;
  char *error ;
  int rc ;
  int slot ;
  
        /* sclient getattr <key> <id> [<level>]*/
        reqGetAttr *ga ;
        md_id_t  id ;
        int level ;
        
        if( argc < 4 )return -22 ;
        rb = (reqBuffer *)sclioBuffer(sclio) ;
        setReqBufferHead( rb , RBR_GET_ATTR , 0 ) ;
        ga = (reqGetAttr *)&(rb->data[0]) ;
        if( argc == 4 ){
            md2ScanPermission( "0" , &(ga ->perm) ) ;
        }else{
            md2ScanPermission( argv[4] , &(ga ->perm) ) ;
        }
        md2ScanId( argv[3] , &(ga->id) ) ;
        rb->size = sizeof(reqGetAttr) ;
        showReqBufferHead( rb ) ;
        setThisAuth(&ga->auth);
        
        sclio = sclClientPostAndWait(scl ,sclio ,ga->id.db  ,10 ,&rc ) ;
        if( ! sclio ){
            if( rc == SCL_TIMEOUT ){
               fprintf(stderr," sclClientPostAndWait : Timeout\n");
            }else{
               error = sclError( "sclClientPostAndWait" , rc ) ;
               fprintf(stderr," Problem : %s(%d)\n",error,errno);
            }
        }else{
           rb = (reqBuffer *)sclioBuffer(sclio) ;
           showReqBufferHead( rb ) ;
           if( rb->status == SRB_OK){
               md2PrintUnixAttr( stdout ,
                     &(((reqGetAttr *)&(rb->data[0]))->attr.unixAttr ) ) ;
               rc = 0 ;
           }else{
             printf( "problem %d\n" , rb->answer ) ;
             rc = rb -> answer ;
           }
        }
        return rc ;
}
#endif
int sc_getrecord( int argc , char *argv[] , SCL *scl )
{
  SCLIO *sclio = sclClientGetBuffer( scl , NULL )  ;
  reqBuffer *rb ;
  char *error ;
  int rc , slot ;
  
        /* sclient getrecord <key> <id> */
        mdRecord *mdr ;
        if( argc < 4 )return -22 ;
        if( argc == 4 )slot = -1 ;
        else sscanf( argv[4] , "%d" , &slot ) ;
        rb = (reqBuffer *)sclioBuffer(sclio) ;
        setReqBufferHead( rb , RBR_GET_RECORD , 0 ) ;
        mdr = (mdRecord *)&(rb->data[0]) ;
        md2ScanId( argv[3] , &(mdr -> head.ID) ) ;
        rb->size = sizeof(*mdr) ;
        showReqBufferHead( rb ) ;
        
        sclio = sclClientPostAndWait(scl ,sclio ,
                 slot < 0 ? mdr->head.ID.db : slot ,10 ,&rc ) ;
        if( ! sclio ){
            if( rc == SCL_TIMEOUT ){
               fprintf(stderr," sclClientPostAndWait : Timeout\n");
            }else{
               error = sclError( "sclClientPostAndWait" , rc ) ;
               fprintf(stderr," Problem : %s(%d)\n",error,errno);
            }
        }else{
           rb = (reqBuffer *)sclioBuffer(sclio) ;
           showReqBufferHead( rb ) ;
           if( rb->status == SRB_OK){
               md2PrintRecord( stdout , (mdRecord *)&(rb->data[0]) ) ;
               rc = 0 ;
           }else{
             printf( "problem %d\n" , rb->answer ) ;
             rc = rb -> answer ;
           }
        }
        return rc ;
}
int sc_setattr( int argc , char *argv[] , SCL *scl )
{
 
/* sclient setattr <key> <id> <level> <uid> <gid> <perm>*/
  char       *error ;
  md_id_t     id ;
  int         level , rc , i ;
  md_auth auth ;
  md_unix attr ;
  md_permission perm ;
          
        if( argc < 8 )return -22 ;
        setMaxAuth(&auth);
        md2ScanId( argv[3] , &(id) ) ;
        md2ScanPermission( argv[4] , &(perm) ) ;
        sscanf( argv[5] , "%d" , &attr.mst_uid ) ;
        sscanf( argv[6] , "%d" , &attr.mst_gid  ) ;
        sscanf( argv[7] , "%ho" , &attr.mst_mode  ) ;

        attr.mst_mode = ( attr.mst_mode& ~ S_IFMT)|S_IFDIR ;
        attr.mst_size  = md_no_size ;
        attr.mst_atime = md_no_time ;
        attr.mst_mtime = md_no_time ;
        attr.mst_ctime = md_no_time ;
        
       rc = mdmSetAttr( scl , &auth , id , perm , &attr ) ;

   return rc ;

}        
int sc_modflags( int argc , char *argv[] , SCL *scl )
{
 
/* sclient modflags <key> <id> <flags> <mask> */
  md_id_t     id ;
  int         level , rc , i ;
  md_auth auth ;
  md_long flags , mask ;
          
  if( argc < 4 ){
      return -22 ;
  }else if( argc == 4 ){
      flags = mask = 0 ;
  }else if( argc < 6 ){
      return -22 ;
  }else{
      sscanf( argv[4] , "%x" , &flags ) ;
      sscanf( argv[5] , "%x" , &mask  ) ;
  }
  setMaxAuth(&auth);
  md2ScanId( argv[3] , &(id) ) ;

  rc = mdmModFlags( scl , &auth , id , &flags , mask ) ;
  if( rc )return rc ;
       
  printf("%s %x\n",mdStringID(id),flags);
  return rc ;

}        
int sc_flag( int argc , char *argv[] , SCL *scl )
{
 
/* sclient flags <key> <id> remove|move|security */
  md_id_t     id ;
  int         level , rc , i ;
  md_auth auth ;
  md_long flags , mask ;
          
  if( argc < 4 ){
      return -22 ;
  }else if( argc == 4 ){
      flags = mask = 0 ;
  }else if( argc < 6 ){
      return -22 ;
  }else{
     if( ! strcmp(argv[4],"remove" ) ){
        mask = MD_INODE_FLAG_NOREMOVE ;
        if( ! strcmp(argv[5],"on" ) ){
           flags = 0 ;
        }else if( ! strcmp(argv[5],"off" ) ){
           flags = mask ;
        }else{
           return -22 ;
        }  
     }else if( ! strcmp(argv[4],"move" ) ){
        mask = MD_INODE_FLAG_NOMOVE ;
        if( ! strcmp(argv[5],"on" ) ){
           flags = 0 ;
        }else if( ! strcmp(argv[5],"off" ) ){
           flags = mask ;
        }else{
           return -22 ;
        }  
     }else if( ! strcmp(argv[4],"security" ) ){
        mask = MD_INODE_FLAG_TRUSTED_WRITE |
               MD_INODE_FLAG_TRUSTED_READ ;
        if( ! strcmp(argv[5],"on" ) ){
           flags = mask ;
        }else if( ! strcmp(argv[5],"off" ) ){
           flags = 0 ;
        }else{
           return -22 ;
        }  
     }else{
        return -22 ;
     }  
  }


  setMaxAuth(&auth);
  md2ScanId( argv[3] , &(id) ) ;

  rc = mdmModFlags( scl , &auth , id , &flags , mask ) ;
  if( rc )return rc ;
       
  printf("%s ",mdStringID(id));
  printf("remove=%s ",flags&MD_INODE_FLAG_NOREMOVE?"off":"on");
  printf("move=%s ",flags&MD_INODE_FLAG_NOMOVE?"off":"on");
  printf("sec-write=%s ",flags&MD_INODE_FLAG_TRUSTED_WRITE?"on":"off");
  printf("sec-write=%s ",flags&MD_INODE_FLAG_TRUSTED_READ?"on":"off");
  printf("\n");
  return rc ;

}        
int sc_readdir( int argc , char *argv[] , SCL *scl )
{
 
 /* sclient readdir <key> <id> [<cookie>]*/
  SCLIO     *sclio ;
  reqBuffer *rb ;
  char      *error ;
  md_id_t    id ;
  int        level , rc , i ;
  reqReadDir *lu ;
  long         cookie ;

  sclio = sclClientGetBuffer( scl , NULL ) ;

        
        if( argc < 4 )return -22 ;
        if( argc == 4 )cookie = 0 ;
        else sscanf( argv[4] , "%x" , &cookie ) ;
        rb = (reqBuffer *)sclioBuffer(sclio) ;
        setReqBufferHead( rb , RBR_READDIR , sizeof(reqReadDir)  ) ;
        lu = (reqReadDir *)&(rb->data[0]) ;
        md2ScanId( argv[3] , &(lu->id) ) ;
        lu -> cookie = cookie ;
        lu -> count  = 4 ;
        showReqBufferHead( rb ) ;
        setThisAuth(&lu->auth);
        
        sclio = sclClientPostAndWait(scl ,sclio ,lu->id.db  ,10 ,&rc ) ;
        if( ! sclio ){
            if( rc == SCL_TIMEOUT ){
               fprintf(stderr," sclClientPostAndWait : Timeout\n");
            }else{
               error = sclError( "sclClientPostAndWait" , rc ) ;
               fprintf(stderr," Problem : %s(%d)\n",error,errno);
            }
        }else{
           rb = (reqBuffer *)sclioBuffer(sclio) ;
           lu = (reqReadDir *)&(rb->data[0]) ;
           showReqBufferHead( rb ) ;
           if( rb->status == SRB_OK){
              for(i=0;i<lu->count;i++)
               printf( " %8.8x %s\n",lu->e[i].cookie,lu->e[i].item.name );
               rc = 0 ;
           }else    {
             printf( "problem %d\n" , rb->answer ) ;
             rc = rb -> answer ;
           }

        }

   return rc ;

}        
int sc_lookup( int argc , char *argv[] , SCL *scl )
{
 
  /* sclient lookup <key> <id> <name> [<perm>]*/
  md_id_t       id ;
  md_permission perm ;
  md_dir_item  item ;
  int          rc ;
  md_unix      attr ;

  if( argc < 5 )return -22 ;
  
  memset( (char*)&(perm ) , 0 , sizeof(perm) ) ; 
  if( argc != 5 ){
       md2ScanPermission( argv[5] , &(perm) ) ;
  }
  md2ScanId( argv[3] , &(id) ) ;
  
  rc = mdmLookupAuth_1( scl , NULL , id , perm ,argv[4] , &item , &attr  );
  if(rc)return rc ;     
  printf( "%s %s -> %s\n",argv[3],argv[4],mdStringID(item.ID));
        
  return rc ;

}
int sc_rmdir( int argc , char *argv[] , SCL *scl )
{
  /* sclient rmdir <key> <id> <name> [<level>]*/
  SCLIO     *sclio ;
  reqBuffer *rb ;
  char      *error ;
  reqRmdir  *lu ;
  md_id_t    id ;
  int        level , rc ;

  sclio = sclClientGetBuffer( scl , NULL ) ;
          
        if( argc < 5 )return -22 ;
        if( argc == 5 )level = 0 ;
        else sscanf( argv[5] , "%d" , &level ) ;
        rb = (reqBuffer *)sclioBuffer(sclio) ;
        setReqBufferHead( rb , RBR_RMDIR , sizeof(reqRmdir)  ) ;
        lu = (reqRmdir *)&(rb->data[0]) ;
        md2ScanId( argv[3] , &(lu->id) ) ;
        memset( (char*)&(lu -> perm ) , 0 , sizeof(lu -> perm) ) ; 
        strcpy( lu -> name , argv[4] ) ;
        showReqBufferHead( rb ) ;
        setThisAuth(&lu->auth);
        
        sclio = sclClientPostAndWait(scl ,sclio ,lu->id.db  ,10 ,&rc ) ;
        if( ! sclio ){
            if( rc == SCL_TIMEOUT ){
               fprintf(stderr," sclClientPostAndWait : Timeout\n");
            }else{
               error = sclError( "sclClientPostAndWait" , rc ) ;
               fprintf(stderr," Problem : %s(%d)\n",error,errno);
            }
        }else{
           rb = (reqBuffer *)sclioBuffer(sclio) ;
           lu = (reqRmdir *)&(rb->data[0]) ;
           showReqBufferHead( rb ) ;
           if( rb->status == SRB_OK){
             printf( "%s %s removed\n",argv[3],argv[4]);
               rc = 0 ;
           }else{
             printf( "problem %d\n" , rb->answer ) ;
             rc = rb -> answer ;
           }

        }
  
  return rc ;

}
int sc_rmfile( int argc , char *argv[] , SCL *scl )
{
  /* sclient rmdir <key> <id> <name> [<level>]*/
  SCLIO     *sclio ;
  reqBuffer *rb ;
  char      *error ;
  reqRmdir  *lu ;
  md_id_t    id ;
  int        level , rc ;

  sclio = sclClientGetBuffer( scl , NULL ) ;
          
        if( argc < 5 )return -22 ;
        if( argc == 5 )level = 0 ;
        else sscanf( argv[5] , "%d" , &level ) ;
        rb = (reqBuffer *)sclioBuffer(sclio) ;
        setReqBufferHead( rb , RBR_RMFILE , sizeof(reqRmdir)  ) ;
        lu = (reqRmdir *)&(rb->data[0]) ;
        md2ScanId( argv[3] , &(lu->id) ) ;
        memset( (char*)&(lu -> perm ) , 0 , sizeof(lu -> perm) ) ; 
        strcpy( lu -> name , argv[4] ) ;
        showReqBufferHead( rb ) ;
        setThisAuth(&lu->auth);
        
        sclio = sclClientPostAndWait(scl ,sclio ,lu->id.db  ,10 ,&rc ) ;
        if( ! sclio ){
            if( rc == SCL_TIMEOUT ){
               fprintf(stderr," sclClientPostAndWait : Timeout\n");
            }else{
               error = sclError( "sclClientPostAndWait" , rc ) ;
               fprintf(stderr," Problem : %s(%d)\n",error,errno);
            }
        }else{
           rb = (reqBuffer *)sclioBuffer(sclio) ;
           lu = (reqRmdir *)&(rb->data[0]) ;
           showReqBufferHead( rb ) ;
           if( rb->status == SRB_OK){
             printf( "%s %s removed\n",argv[3],argv[4]);
               rc = 0 ;
           }else {
             printf( "problem %d\n" , rb->answer ) ;
             rc = rb -> answer ;
           }
        }
  
  return rc ;

}
int sc_mkdir( int argc , char *argv[] , SCL *scl )
{
  /* sclient mkdir <key> <id> <name> <uid> <gid> <perm>*/
  SCLIO     *sclio ;
  reqBuffer *rb ;
  char      *error ;
  reqMkdir  *md ;
  md_id_t    id ;
  int        level , rc ;

  sclio = sclClientGetBuffer( scl , NULL ) ;
        
        if( argc < 8 )return -22 ;
        rb = (reqBuffer *)sclioBuffer(sclio) ;
        setReqBufferHead( rb , RBR_MKDIR , sizeof(reqMkdir)  ) ;
        showReqBufferHead( rb ) ;
        md = (reqMkdir *)&(rb->data[0]) ;
        md2ScanId( argv[3] , &(md->id) ) ;
        memset( (char*)&(md -> perm ) , 0 , sizeof(md -> perm) ) ; 
        md -> auth.uid = getuid() ;
        md -> auth.gid = getgid() ;
        strcpy( md -> name , argv[4] ) ;
        sscanf( argv[5] , "%d" , &(md -> attr.mst_uid ) ) ;
        sscanf( argv[6] , "%d" , &(md -> attr.mst_gid ) ) ;
        sscanf( argv[7] , "%ho" , &(md -> attr.mst_mode ) ) ;
        setThisAuth(&md->auth);

        md -> attr.mst_mode = ( md -> attr.mst_mode& ~ S_IFMT)|S_IFDIR ;
        md -> attr.mst_size  = md_no_size ;
        md -> attr.mst_atime = md_no_time ;
        md -> attr.mst_mtime = md_no_time ;
        md -> attr.mst_ctime = md_no_time ;
        /*md2PrintUnixAttr(stdout,&(md->attr ));*/
        
        sclio = sclClientPostAndWait(scl ,sclio ,md->id.db  ,10 ,&rc ) ;
        if( ! sclio ){
            if( rc == SCL_TIMEOUT ){
               fprintf(stderr," sclClientPostAndWait : Timeout\n");
            }else{
               error = sclError( "sclClientPostAndWait" , rc ) ;
               fprintf(stderr," Problem : %s(%d)\n",error,errno);
            }
        }else{
           rb = (reqBuffer *)sclioBuffer(sclio) ;
           md= (reqMkdir *)&(rb->data[0]) ;
           showReqBufferHead( rb ) ;
           if( rb->status == SRB_OK){
             printf( "%s %s -> %s\n",argv[3],argv[4],mdStringID(md->resId));
             /*md2PrintUnixAttr(stdout,&(md->attr ));*/
               rc = 0 ;
           }else {
             printf( "problem %d\n" , rb->answer ) ;
             rc = rb -> answer ;
           }
        }
  return rc;

}
int sc_mkfile( int argc , char *argv[] , SCL *scl )
{
  /* sclient mkfile <key> <id> <name> <uid> <gid> <perm>*/
  SCLIO     *sclio ;
  reqBuffer *rb ;
  char      *error ;
  reqMkfile *md ;
  md_id_t    id ;
  int        level , rc ;

  sclio = sclClientGetBuffer( scl , NULL ) ;
        
        if( argc < 8 )return -22 ;
        rb = (reqBuffer *)sclioBuffer(sclio) ;
        setReqBufferHead( rb , RBR_MKFILE , sizeof(reqMkfile)  ) ;
        showReqBufferHead( rb ) ;
        md = (reqMkfile *)&(rb->data[0]) ;
        md2ScanId( argv[3] , &(md->id) ) ;
        memset( (char*)&(md -> perm ) , 0 , sizeof(md -> perm) ) ; 
        md -> auth.uid = 0 ;
        md -> auth.gid = getgid() ;
        strcpy( md -> name , argv[4] ) ;
        sscanf( argv[5] , "%d" , &(md -> attr.mst_uid ) ) ;
        sscanf( argv[6] , "%d" , &(md -> attr.mst_gid ) ) ;
        sscanf( argv[7] , "%ho" , &(md -> attr.mst_mode ) ) ;
        setThisAuth(&md->auth);

        md -> attr.mst_mode = ( md -> attr.mst_mode& ~ S_IFMT)|S_IFREG ;
        md -> attr.mst_size  = md_no_size ;
        md -> attr.mst_atime = md_no_time ;
        md -> attr.mst_mtime = md_no_time ;
        md -> attr.mst_ctime = md_no_time ;
        /* md2PrintUnixAttr(stdout,&(md->attr )); */
        
        sclio = sclClientPostAndWait(scl ,sclio ,md->id.db  ,10 ,&rc ) ;
        if( ! sclio ){
            if( rc == SCL_TIMEOUT ){
               fprintf(stderr," sclClientPostAndWait : Timeout\n");
            }else{
               error = sclError( "sclClientPostAndWait" , rc ) ;
               fprintf(stderr," Problem : %s(%d)\n",error,errno);
            }
        }else{
           rb = (reqBuffer *)sclioBuffer(sclio) ;
           md= (reqMkfile *)&(rb->data[0]) ;
           showReqBufferHead( rb ) ;
           if( rb->status == SRB_OK){
             printf( "%s %s -> %s\n",argv[3],argv[4],mdStringID(md->resId));
             md2PrintUnixAttr(stdout,&(md->attr ));
               rc = 0 ;
           }else {
             printf( "problem %d\n" , rb->answer ) ;
             rc = rb -> answer ;
           }
        }
  return rc ;

}
int sc_rename( int argc , char *argv[] , SCL *scl )
{
  /* sclient rename <key> <id> <name> <id> <name>*/
  SCLIO     *sclio ;
  reqBuffer *rb ;
  char      *error ;
  reqRename *rn ;
  md_id_t    id ;
  int        level , rc ;

  sclio = sclClientGetBuffer( scl , NULL ) ;
        
        if( argc < 7 )return -22 ;
        rb = (reqBuffer *)sclioBuffer(sclio) ;
        setReqBufferHead( rb , RBR_RENAME , sizeof(reqRename)  ) ;
        showReqBufferHead( rb ) ;
        rn = (reqRename *)&(rb->data[0]) ;
        md2ScanId( argv[3] , &(rn->from.id) ) ;
        strcpy( rn -> from.name , argv[4] ) ;
        md2ScanId( argv[5] , &(rn->to.id) ) ;
        strcpy( rn -> to.name , argv[6] ) ;
        
        setThisAuth(&rn->auth);

        
        sclio = sclClientPostAndWait(scl ,sclio ,rn->from.id.db  ,10 ,&rc ) ;
        if( ! sclio ){
            if( rc == SCL_TIMEOUT ){
               fprintf(stderr," sclClientPostAndWait : Timeout\n");
            }else{
               error = sclError( "sclClientPostAndWait" , rc ) ;
               fprintf(stderr," Problem : %s(%d)\n",error,errno);
            }
        }else{
           rb = (reqBuffer *)sclioBuffer(sclio) ;
           rn= (reqRename *)&(rb->data[0]) ;
           showReqBufferHead( rb ) ;
           if( rb->status == SRB_OK){
             printf( "%s %s -> %s %s\n",mdStringID(rn->from.id),argv[4],
                      mdStringID(rn->to.id),argv[6]);
               rc = 0 ;
           }else {
             printf( "problem %d\n" , rb->answer ) ;
             rc = rb -> answer ;
           }
        }
  return rc ;

}
int sc_readlink( int argc , char *argv[] , SCL *scl )
{
  /* sclient readlink <key> <id> */
  SCLIO     *sclio ;
  reqBuffer *rb ;
  char      *error ;
  reqReadLink *rl  ;
  md_id_t    id ;
  int        level , rc ;

  sclio = sclClientGetBuffer( scl , NULL ) ;
        
        if( argc < 4 )return -22 ;
        rb = (reqBuffer *)sclioBuffer(sclio) ;
        setReqBufferHead( rb , RBR_READLINK , sizeof(reqReadLink)  ) ;
        showReqBufferHead( rb ) ;
        rl = (reqReadLink *)&(rb->data[0]) ;
        md2ScanId( argv[3] , &(rl->id) ) ;
       
        setThisAuth(&rl->auth);
        memset( (char*)&(rl -> perm ) , 0 , sizeof(rl -> perm) ) ; 

        
        sclio = sclClientPostAndWait(scl ,sclio ,rl-> id.db  ,10 ,&rc ) ;
        if( ! sclio ){
            if( rc == SCL_TIMEOUT ){
               fprintf(stderr," sclClientPostAndWait : Timeout\n");
            }else{
               error = sclError( "sclClientPostAndWait" , rc ) ;
               fprintf(stderr," Problem : %s(%d)\n",error,errno);
            }
        }else{
           rb = (reqBuffer *)sclioBuffer(sclio) ;
           rl= (reqReadLink *)&(rb->data[0]) ;
           showReqBufferHead( rb ) ;
           if( rb->status == SRB_OK){
             printf( "%s -> %s\n",mdStringID(rl->id),rl->path);
               rc = 0 ;
           }else {
             printf( "problem %d\n" , rb->answer ) ;
             rc = rb -> answer ;
           }
        }
  return rc ;

}
int sc_makelink( int argc , char *argv[] , SCL *scl )
{
  /* sclient makelink <key> <id> <name> <path>*/
  SCLIO     *sclio ;
  reqBuffer *rb ;
  char      *error ;
  reqMakeLink *rl  ;
  md_id_t    id ;
  int        level , rc ;

  sclio = sclClientGetBuffer( scl , NULL ) ;
        
        if( argc < 4 )return -22 ;
        rb = (reqBuffer *)sclioBuffer(sclio) ;
        setReqBufferHead( rb , RBR_MAKELINK , sizeof(reqMakeLink)  ) ;
        showReqBufferHead( rb ) ;
        rl = (reqMakeLink *)&(rb->data[0]) ;
        md2ScanId( argv[3] , &(rl->id) ) ;
        strcpy( rl-> name , argv[4] ) ;
        strcpy( rl-> path , argv[5] ) ;
        
        setThisAuth(&rl->auth);
        memset( (char*)&(rl -> perm ) , 0 , sizeof(rl -> perm) ) ; 

        
        sclio = sclClientPostAndWait(scl ,sclio ,rl-> id.db  ,10 ,&rc ) ;
        if( ! sclio ){
            if( rc == SCL_TIMEOUT ){
               fprintf(stderr," sclClientPostAndWait : Timeout\n");
            }else{
               error = sclError( "sclClientPostAndWait" , rc ) ;
               fprintf(stderr," Problem : %s(%d)\n",error,errno);
            }
        }else{
           rb = (reqBuffer *)sclioBuffer(sclio) ;
           rl= (reqMakeLink *)&(rb->data[0]) ;
           showReqBufferHead( rb ) ;
           if( rb->status == SRB_OK){
             printf( "%s %s-> %s\n",mdStringID(rl->id),rl->name,mdStringID(rl->resID));
               rc = 0 ;
           }else {
             printf( "problem %d\n" , rb->answer ) ;
             rc = rb -> answer ;
           }
        }
  return rc ;

}
int sc_readdata( int argc , char *argv[] , SCL *scl )
{
  /* sclient readdata <key> <id> <level> <offset> <size>*/
  SCLIO     *sclio ;
  reqBuffer *rb ;
  char      *error ;
  reqReadData *rl  ;
  md_id_t    id ;
  int        level , rc ;

  sclio = sclClientGetBuffer( scl , NULL ) ;
        
        if( argc < 7 )return -22 ;
        rb = (reqBuffer *)sclioBuffer(sclio) ;
        setReqBufferHead( rb , RBR_READDATA,sizeof(reqReadData)  ) ;
        showReqBufferHead( rb ) ;
        rl = (reqReadData *)&(rb->data[0]) ;
        md2ScanId( argv[3] , &(rl->id) ) ;
        sscanf( argv[4] , "%d" , &(level) ) ;
        sscanf( argv[5] , "%d" , &(rl->offset) ) ;
        sscanf( argv[6] , "%d" , &(rl->size) ) ;
        memset( (char*)&(rl -> perm ) , 0 , sizeof(rl -> perm) ) ; 
        mdpSetLevel( rl->perm , level ) ;
        setThisAuth(&rl->auth);

        
        sclio = sclClientPostAndWait(scl ,sclio ,rl-> id.db  ,10 ,&rc ) ;
        if( ! sclio ){
            if( rc == SCL_TIMEOUT ){
               fprintf(stderr," sclClientPostAndWait : Timeout\n");
            }else{
               error = sclError( "sclClientPostAndWait" , rc ) ;
               fprintf(stderr," Problem : %s(%d)\n",error,errno);
            }
        }else{
           rb = (reqBuffer *)sclioBuffer(sclio) ;
           rl= (reqReadData *)&(rb->data[0]) ;
           showReqBufferHead( rb ) ;
           if( rb->status == SRB_OK){
             md2DumpMemory(stdout , rl->data , 0L , rl->size );
               rc = 0 ;
           }else {
             printf( "problem %d\n" , rb->answer ) ;
             rc = rb -> answer ;
           }
        }
  return rc ;

}
int sc_writedata( int argc , char *argv[] , SCL *scl )
{
  /* sclient sc_writedata <key> <id> <level> <offset> <size>*/
  SCLIO     *sclio ;
  reqBuffer *rb ;
  char      *error ;
  reqWriteData *rl  ;
  md_id_t    id ;
  int        level , rc , i ;

  sclio = sclClientGetBuffer( scl , NULL ) ;
        
        if( argc < 7 )return -22 ;
        rb = (reqBuffer *)sclioBuffer(sclio) ;
        setReqBufferHead( rb , RBR_WRITEDATA , sizeof(reqWriteData)  ) ;
        showReqBufferHead( rb ) ;
        rl = (reqWriteData *)&(rb->data[0]) ;
        md2ScanId( argv[3] , &(rl->id) ) ;
        sscanf( argv[4] , "%d" , &(level) ) ;
        sscanf( argv[5] , "%d" , &(rl->offset) ) ;
        sscanf( argv[6] , "%d" , &(rl->size) ) ;
        memset( (char*)&(rl -> perm ) , 0 , sizeof(rl -> perm) ) ; 
        mdpSetLevel( rl->perm , level ) ;
        setThisAuth(&rl->auth);
        rl->size = rl->size > 4*1024 ? 4*1024 : rl->size ;
        for(i=0;i<rl->size;i++)rl->data[i] = i & 0xFF ;

        
        sclio = sclClientPostAndWait(scl ,sclio ,rl-> id.db  ,10 ,&rc ) ;
        if( ! sclio ){
            if( rc == SCL_TIMEOUT ){
               fprintf(stderr," sclClientPostAndWait : Timeout\n");
            }else{
               error = sclError( "sclClientPostAndWait" , rc ) ;
               fprintf(stderr," Problem : %s(%d)\n",error,errno);
            }
        }else{
           rb = (reqBuffer *)sclioBuffer(sclio) ;
           rl= (reqWriteData *)&(rb->data[0]) ;
           showReqBufferHead( rb ) ;
           if( rb->status == SRB_OK){
             printf("written %d\n",rb->size);
               rc = 0 ;
           }else {
             printf( "problem %d\n" , rb->answer ) ;
             rc = rb -> answer ;
           }
        }
  return rc ;

}
int sc_setsize( int argc , char *argv[] , SCL *scl )
{
  /* sclient sc_setsize <key> <id> <level> <size>*/
  SCLIO     *sclio ;
  reqBuffer *rb ;
  char      *error ;
  reqSetSize *rl  ;
  md_id_t    id ;
  int        level , rc , i ;

  sclio = sclClientGetBuffer( scl , NULL ) ;
        
        if( argc < 6 )return -22 ;
        rb = (reqBuffer *)sclioBuffer(sclio) ;
        setReqBufferHead( rb , RBR_SETSIZE , sizeof(reqSetSize)  ) ;
        showReqBufferHead( rb ) ;
        rl = (reqSetSize *)&(rb->data[0]) ;
        md2ScanId( argv[3] , &(rl->id) ) ;
        sscanf( argv[4] , "%d" , &(level) ) ;
        sscanf( argv[5] , "%d" , &(rl->size) ) ;
        memset( (char*)&(rl -> perm ) , 0 , sizeof(rl -> perm) ) ; 
        mdpSetLevel( rl->perm , level ) ;
        setThisAuth(&rl->auth);

        
        sclio = sclClientPostAndWait(scl ,sclio ,rl-> id.db  ,10 ,&rc ) ;
        if( ! sclio ){
            if( rc == SCL_TIMEOUT ){
               fprintf(stderr," sclClientPostAndWait : Timeout\n");
            }else{
               error = sclError( "sclClientPostAndWait" , rc ) ;
               fprintf(stderr," Problem : %s(%d)\n",error,errno);
            }
        }else{
           rb = (reqBuffer *)sclioBuffer(sclio) ;
           rl= (reqSetSize *)&(rb->data[0]) ;
           showReqBufferHead( rb ) ;
           if( rb->status == SRB_OK){
             printf("newsize %d\n",rb->size);
               rc = 0 ;
           }else {
             printf( "problem %d\n" , rb->answer ) ;
             rc = rb -> answer ;
           }
        }
  return rc ;

}
int sc_setperm( int argc , char *argv[] , SCL *scl )
{
  /* sclient sc_setperm <key> <id> <name> <perm>*/
  SCLIO     *sclio ;
  reqBuffer *rb ;
  char      *error ;
  reqSetPerm *rl  ;
  md_id_t    id ;
  int        level , rc , i ;

  sclio = sclClientGetBuffer( scl , NULL ) ;
        
        if( argc < 6 )return -22 ;
        rb = (reqBuffer *)sclioBuffer(sclio) ;
        setReqBufferHead( rb , RBR_SETPERM , sizeof(reqSetPerm)  ) ;
        showReqBufferHead( rb ) ;
        rl = (reqSetPerm *)&(rb->data[0]) ;
        md2ScanId( argv[3] , &(rl->id) ) ;
        strcpy( rl->name , argv[4] ) ;
        memset( (char*)&(rl -> newPerm ) , 0 , sizeof(rl -> newPerm) ) ; 
        md2ScanPermission( argv[5] , &(rl->newPerm) ) ;

        setThisAuth(&rl->auth);

        
        sclio = sclClientPostAndWait(scl ,sclio ,rl-> id.db  ,10 ,&rc ) ;
        if( ! sclio ){
            if( rc == SCL_TIMEOUT ){
               fprintf(stderr," sclClientPostAndWait : Timeout\n");
            }else{
               error = sclError( "sclClientPostAndWait" , rc ) ;
               fprintf(stderr," Problem : %s(%d)\n",error,errno);
            }
        }else{
           rb = (reqBuffer *)sclioBuffer(sclio) ;
           rl= (reqSetPerm *)&(rb->data[0]) ;
           showReqBufferHead( rb ) ;
           if( rb->status == SRB_OK){
             printf("%s %s %s\n",mdStringID(rl->id),
                    mdStringPermission(rl->newPerm),rl->name);
               rc = 0 ;
           }else {
             printf( "problem %d\n" , rb->answer ) ;
             rc = rb -> answer ;
           }
        }
  return rc ;

}
int sc_getroot( int argc , char *argv[] , SCL *scl )
{
  /* sclient getroot <key> <db>               */
  /* sclient getroot <key> <db> <newConfigID> */
  SCLIO     *sclio ;
  reqBuffer *rb ;
  char      *error ;
  reqGetRoot *rl  ;
  md_id_t    id ;
  int        level , rc , i ;
  char tmp[128] ;

  sclio = sclClientGetBuffer( scl , NULL ) ;
        
     if( argc < 4 )return -22 ;

        rb = (reqBuffer *)sclioBuffer(sclio) ;
        setReqBufferHead( rb , RBR_GETROOT , sizeof(reqGetRoot)  ) ;
        showReqBufferHead( rb ) ;
        rl = (reqGetRoot *)&(rb->data[0]) ;
        sscanf( argv[3] , "%d" ,  &level ) ;
        if( argc > 4 ){
            md2ScanId( argv[4] , &(rl->config) ) ;
        }else{
            mdSetNullID( rl->config ) ;
        }
        setThisAuth(&rl->auth);

        
        sclio = sclClientPostAndWait(scl ,sclio ,level  ,10 ,&rc ) ;
        if( ! sclio ){
            if( rc == SCL_TIMEOUT ){
               fprintf(stderr," sclClientPostAndWait : Timeout\n");
            }else{
               error = sclError( "sclClientPostAndWait" , rc ) ;
               fprintf(stderr," Problem : %s(%d)\n",error,errno);
            }
        }else{
           rb = (reqBuffer *)sclioBuffer(sclio) ;
           rl= (reqGetRoot *)&(rb->data[0]) ;
           showReqBufferHead( rb ) ;
           if( rb->status == SRB_OK){
             strcpy( tmp , mdStringID(rl->id) ) ;
             printf("%d %s %s\n",level,tmp,mdStringID(rl->config));
             rc = 0 ;
           }else{
             printf( "problem %d\n" , rb->answer ) ;
             rc = rb -> answer ;
           }
        }

     
  return rc ;

}
int sc_macro_ls( int argc , char *argv[] , SCL *scl )
{
 
 /* sclient ls <key> <id>*/
  SCLIO     *sclio ;
  reqBuffer *rb ;
  char      *error ;
  md_id_t    id ;
  int        level , rc , i , countPerCall = 4 ;
  reqReadDir *lu ;
  long         cookie ;

  if( argc < 4 )return -22 ;
  
  md2ScanId( argv[3] , &id ) ;
  
  for( cookie = 0 ; ; cookie = lu->e[lu->count-1].cookie ){
  
        sclio = sclClientGetBuffer( scl , NULL ) ;
        rb = (reqBuffer *)sclioBuffer(sclio) ;
        
        setReqBufferHead( rb , RBR_READDIR , sizeof(reqReadDir)  ) ;
        lu = (reqReadDir *)&(rb->data[0]) ;
        lu -> cookie = cookie ;
        lu -> count  = countPerCall ;
        lu -> id     = id ;
        setThisAuth(&lu->auth);
        
        sclio = sclClientPostAndWait(scl ,sclio ,lu->id.db  ,10 ,&rc ) ;
        if( ! sclio ){
            if( rc == SCL_TIMEOUT ){
               fprintf(stderr," sclClientPostAndWait : Timeout\n");
            }else{
               error = sclError( "sclClientPostAndWait" , rc ) ;
               fprintf(stderr," Problem : %s(%d)\n",error,errno);
            }
            break ;
        }else{
           rb = (reqBuffer *)sclioBuffer(sclio) ;
           lu = (reqReadDir *)&(rb->data[0]) ;
           if( rb->status == SRB_OK){
              rc = 0 ;
              for(i=0;i<lu->count;i++)
                printf( "%s %s %s\n",mdStringID(lu->e[i].item.ID),
                     mdStringPermission(lu->e[i].item.perm),lu->e[i].item.name );
              if( ( lu->count <=0 ) || ( lu -> count < countPerCall ) )break ;
           }else{
             rc = rb -> answer ;
             break ;
           }
        }
        
   }

   return rc ;

}        
int sc_macro_copy( int argc , char *argv[] , SCL *scl )
{
 int level , rc , first , secnd , level2 ;
 md_id_t id , id2 ;
 
 /* sclient copy <key> <Xpath> <id>*/
 /* sclient copy <key> <id> <Xpath>*/
 
 if( argc < 5 )return -22 ;
 first = mayBeId( argv[3] ) ;
 secnd = mayBeId( argv[4] ) ;
 
 if( ( ( ! first ) && ( ! secnd ) ) )return -22 ;
     
 if( first && ! secnd ){
 
     md2ScanIdLevel( argv[3] , &id , &level );
     rc = scCopyFromPnfs( scl , argv[4] ,  id , level );
 
 }else if( secnd && ! first ){
 
     md2ScanIdLevel( argv[4] , &id , &level );
     rc = scCopyToPnfs( scl , argv[3] ,  id , level );
 
 }else{
     md2ScanIdLevel( argv[3] , &id , &level );
     md2ScanIdLevel( argv[4] , &id2 , &level2 );
     rc = scPnfsToPnfs( scl , id , level , id2 , level2 );
 
 }
 return rc ;
}
int mayBeId( char *id )
{
 char *x ;
 
  for(x=id;(*x!='\0')&&isxdigit(*x);x++);
  return *x=='\0'?1:0;
}
int sc_macro_getid( int argc , char *argv[] , SCL *scl )
{
 
 /* sclient getid <key> <path>*/
  int rc , i , sl , p , db ;
  md_id_t current ;
  char  string[1024] , *s , *cptr[128] ;
  

  if( argc < 4 )return -22 ;
  strncpy( string , argv[3] , sizeof( string) -1  ) ;
  string[sizeof(string)-1] = '\0' ;
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
     /* printf( " %d : %s\n" , i , cptr[i] ) ;  */
     if( rc = mdmLookupAuth_0( scl , NULL , current , cptr[i] , &current ) )return rc ;
  }
  printf( "%s %s\n",mdStringID(current),argv[3]);
  return 0 ; 
 
} 
#define SC_COPY_BUFFER_SIZE   (1024)       
int scPnfsToPnfs( SCL *scl , md_id_t fromId , int fromLevel ,
                             md_id_t toId , int toLevel       )
{
  int rc , fh;
  sc_fs_block from , to ;
  char buffer[SC_COPY_BUFFER_SIZE] ;
  
  if( rc = scFs_Open( scl , &from , fromId , fromLevel , "rl" )  )return rc ;
  if( rc = scFs_Open( scl , &to , toId , toLevel , "wl" )  )return rc ;

  while( ( rc = scFs_Read( &from , buffer , SC_COPY_BUFFER_SIZE ) ) > 0 ){	
    if( ( rc = scFs_Write( &to , buffer , rc ) ) < 0 )break ;  
  }
  if(rc)fprintf(stderr," Something returned with %d\n",rc );
  return 0 ;
}
int scCopyToPnfs( SCL *scl , char *name ,  md_id_t id , int level )
{
  int rc , fh;
  sc_fs_block p ;
  char buffer[SC_COPY_BUFFER_SIZE] ;
  
  if( rc = scFs_Open( scl , &p , id , level , "wl" )  )return rc ;
  if( ( fh = open( name , O_RDONLY ) ) < 0 ){
     fprintf(stderr," Can't open %s : %d\n" , name , errno ) ;
     return -200 ;
  }
  while( ( rc = read( fh , buffer , SC_COPY_BUFFER_SIZE ) ) > 0 ){	
    if( ( rc = scFs_Write( &p , buffer , rc ) ) < 0 )break ;  
  }
  if(rc)fprintf(stderr," Something returned with %d\n",rc );
  close(fh);
  return 0 ;
}
int scCopyFromPnfs( SCL *scl , char *name ,  md_id_t id , int level )
{
  int rc , fh;
  sc_fs_block p ;
  char buffer[SC_COPY_BUFFER_SIZE] ;
  
  if( rc = scFs_Open( scl , &p , id , level , "rl" )  )return rc ;

  if( ( fh = open( name , O_WRONLY | O_CREAT | O_TRUNC  , 0600 ) ) < 0 ){

     fprintf(stderr," Can't open %s : %d\n" , name , errno ) ;
     return -200 ;
  }
  while( ( rc = scFs_Read( &p , buffer , SC_COPY_BUFFER_SIZE ) ) > 0 ){	
    if( ( rc = write( fh , buffer , rc ) ) < 0 )break ;  
  }

  close(fh);
  return rc ;
}
