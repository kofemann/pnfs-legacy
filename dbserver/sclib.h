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
#ifndef __SCLIB__H__
#define __SCLIB__H__

#include "allowed.h"

#define mdPermission md_permission

typedef struct {

        md_id_t    id ;
           long    offset ;
           long    size ;
  unsigned long    mode ;
            int    level;
            SCL   *scl ;

} sc_fs_block ;

#define SC_FS_BUFFER_SIZE  (512) 

typedef struct {

        sc_fs_block  block ;
	char         buffer[SC_FS_BUFFER_SIZE] ;
	long         maxSize ;
	long         size ;
	long         offset ;
	
} sc_fs_format ;

#define scFsREAD     (1)
#define scFsWRITE    (2)

#define SCEfsACCESS   (-101)

#define setMaxAuth(a)    {memset((char*)a,0,sizeof(md_auth));\
                          (a)->uid=0;(a)->gid=1;(a)->priv=15;\
                          gettimeofday(&((a)->timestamp),NULL);} 
#define setThisAuth(a)    {memset((char*)a,0,sizeof(md_auth));\
                           (a)->uid=getuid();(a)->gid=1;\
                           (a)->priv=(a)->uid?0:15;\
                           gettimeofday(&((a)->timestamp),NULL);} 


int mdmICommand( SCL *scl , int db , char * command , int * comLength , int maxSize );
int mdmGetAttrAuth( SCL *scl , md_auth * auth , md_id_t id , md_permission perm , md_unix *attr );
int mdmGetRootConfig( SCL *scl , int db , md_id_t *id , md_id_t *config );
int mdmGetExtAttrAuth( SCL *scl , md_auth * auth , md_id_t id , md_permission perm,
                   md_unix *attr , mdRecord *rec         );
int mdmTruncate( SCL *scl , md_id_t id , mdPermission perm   );
int mdmGetRootId( SCL *scl , int db , md_id_t *id );
int mdmLookupAuth_0( SCL *scl , md_auth * auth ,  md_id_t dir , char *name , md_id_t *res );
int mdmLookupAuth_1( SCL *scl , md_auth * auth , md_id_t dir , md_permission perm ,
                 char *name , md_dir_item *item , md_unix  *attr  );
int mdmCreateFile( SCL *scl , md_auth *auth , md_id_t dir , md_permission perm ,
                 char *name , md_unix  *attr  , md_permission *resPerm ,
                 md_id_t *resID );
int mdmChangePermission( SCL *scl , md_auth *auth , md_id_t dir , 
                 char *name , md_permission perm  );
int mdmForceSize( SCL *scl , md_auth *auth , md_id_t id ,
                  md_permission perm , md_long size     ); 
int mdmRmFile( SCL *scl , md_auth *auth , md_id_t dir ,
               md_permission perm ,  char *name         );
int mdmRmDir( SCL *scl , md_auth *auth , md_id_t dir ,
               md_permission perm ,  char *name         );
int mdmMkDir( SCL *scl , md_auth *auth , md_id_t dir , md_permission perm ,
                 char *name , md_unix  *attr  , md_id_t *resID );
int mdmRmDirEntry( SCL *scl , md_auth *auth , md_id_t dir ,
                 char *name , md_dir_item *item  );
int mdmAddDirEntry( SCL *scl , md_auth *auth , md_id_t dir , md_permission perm ,
                 char *name , md_dir_item *item  );
int mdmReadData( SCL *scl , md_id_t id , mdPermission perm , 
                 long offset , long size , char *data       );
int mdmReadDataAuth( SCL *scl ,md_auth *auth ,  md_id_t id , mdPermission perm , 
                     long offset , long size , char *data  , md_unix *uattr );
int mdmWriteData( SCL *scl , md_id_t id , mdPermission perm , 
                 long offset , long size , char *data       );
int mdmSetAttr( SCL *scl , md_auth *auth , md_id_t id ,
                md_permission perm , md_unix  *attr      );
int mdmSetAttrs( SCL *scl , md_auth *auth , md_id_t id ,
                 md_permission perm , md_unix  *attr      );
int mdmModLink( SCL *scl , md_auth *auth , md_id_t id ,
                md_permission perm , int  *diff      ) ;
int mdmModFlags( SCL *scl , md_auth *auth , md_id_t id ,
                md_long * flags , md_long mask     ) ;
int mdmGetLink( SCL *scl , md_auth *auth , md_id_t id ,
                md_permission perm , char *path           );
int mdmMkLink( SCL *scl , md_auth *auth , md_id_t dir , md_permission perm ,
               char *name , md_unix  *attr  , char *path ,  md_id_t *resID );
int mdmGetRecord( SCL *scl , md_id_t id ,  mdRecord *rec );
int mdmReadLink( SCL *scl ,md_id_t id ,mdPermission perm ,char *path  );
int mdmGetObjectID( SCL *scl , char *path , md_id_t *res );
int mdmReadDirAuth( SCL *scl ,md_auth *auth , md_id_t id ,mdPermission perm ,
                    long cookie , int count , reqExtItem *extItem  );
int mdmChParent( SCL *scl , md_auth *auth , md_id_t id ,md_id_t parent  );
int mdmDelObject( SCL *scl , md_auth *auth , md_id_t id , int type  );
int mdmDelFile( SCL *scl , md_auth *auth , md_id_t id );
int mdmGetChain( SCL *scl , md_auth *auth , md_id_t dir , md_id_t child ,
                 md_id_t *parent , md_dir_item *item  );
int mdmFindIdAuth( SCL *scl , md_auth *auth , md_id_t id , mdPermission perm ,
                   md_id_t parentId , int size ,  char *data , md_unix *uattr );



#define  mdmDeleteFile(s,a,i)       mdmDelObject(s,a,i,RBR_OBJ_FILE)
#define  mdmDeleteDirectory(s,a,i)  mdmDelObject(s,a,i,RBR_OBJ_DIR)

char *scFs_Ffgets(char *s, int n, sc_fs_format *f);
int scFs_Ffgetc( sc_fs_format *f );
int scFs_Ffputs(const char *s, sc_fs_format *f);
int scFs_Ffputc( int c , sc_fs_format *f );
int scFs_FOpen( SCL *scl , sc_fs_format *f , md_id_t id , int level , char *mode );
int scFs_Open( SCL *scl , sc_fs_block *p , md_id_t id , int level , char *mode );
int scFs_Write( sc_fs_block *p , char *buffer , long size );
int scFs_Read( sc_fs_block *p , char *buffer , long size );

#endif
