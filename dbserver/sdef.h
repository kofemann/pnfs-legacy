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
#ifndef _SDEF__H__
#define _SDEF__H__

#define MAX_REQ_DATA_SIZE   (8*1024)
 
                                 /* sizeof(*rb) would clear to much */
#define  setReqBufferHead(rb,r,s)  memset((char*)(rb),0,5*sizeof(long));\
                                  (rb)->version=1;(rb)->request=(r);\
                                  (rb)->size=(s);

#ifdef showBuffer
#define  showReqBufferHead(rb)  { int i ; \
   printf(" Version : %d\n",(rb)->version);\
   printf(" Request : 0x%x\n",(rb)->request);\
   printf(" Status  : 0x%x\n",(rb)->status);\
   printf(" Answer  : 0x%x(%d)\n",(rb)->answer,(rb)->answer);\
   printf(" Size    : %d\n",(rb)->size);\
   printf(" Data    : ");\
   for(i=0;i<16;i++)printf("%2.2X ",(rb)->data[i]);printf("\n");}
#else
#define  showReqBufferHead(rb)  
#endif                              
  typedef struct req_buffer {
      long version ;
      long request ;
      long status ;
      long answer ;
      long size ;
      char data[1];
  } reqBuffer ;
  /*
     whatever new request you will invent, MAKE SURE, 
     md_auth is the first item in the structure ....
  */
  typedef struct req_backup_ {
      md_auth        auth ;         /*  -->   */
      char           name[1024] ;   /*  -->   */
      md_long        force  ;       /*  -->   */
  } reqBackup ;
  typedef struct req_icommand_ {
      md_auth        auth ;           /*  -->   */
      long           size ;           /*  <->   */
      char           command[2048] ;  /*  -->   */
  } reqICommand ;
  typedef struct req_getattr_ {
      md_auth        auth ;  /*  -->   */
      md_id_t        id ;    /*  -->   */
      md_permission  perm ;  /*  -->   */
      md_attr        attr ;  /*  <--   */
      mdRecord       rec ;   /*  <--   */
  } reqGetAttr ;
  typedef struct req_modflags_ {
      md_auth        auth ;  /*  -->   */
      md_id_t        id ;    /*  -->   */
      md_long        flags ; /*  <->   */
      md_long        mask ;  /*  -->   */
  } reqModFlags ;
  typedef struct req_lookup_ {
     md_auth        auth ;         /*  -->   */
     md_id_t        id ;           /*  -->   */
     md_permission  perm ;         /*  -->   */
     char           name[1024] ;   /*  -->   */
     md_dir_item    item ;         /*  <--   */
     md_unix        attr ;         /*  <--   */
  } reqLookup ; 
  
  typedef struct req_mkfile_ {
     md_auth        auth ;         /*  -->   */
     md_id_t        id ;           /*  -->   */
     md_permission  perm ;         /*  -->   */
     char           name[1024] ;   /*  -->   */
     md_unix        attr ;         /*  <->   */
     md_id_t        resId ;        /*  <--   */
     md_permission  resPerm ;      /*  <--   */
  } reqMkfile ; 
  typedef struct req_mkfile_  reqMkdir ; 


  typedef struct req_rmfile_ {
     md_auth        auth ;         /*  -->   */
     md_id_t        id ;           /*  -->   */
     md_permission  perm ;         /*  -->   */
     char           name[1024] ;   /*  -->   */
     md_long        type  ;        /*  -->   */
     md_dir_item    item ;         /*  <--   */
  } reqRmfile ; 
  typedef struct req_rmfile_  reqRmdir ; 
  typedef struct req_rmfile_  reqRmFromDir ; 
  typedef struct req_rmfile_  reqAddToDir ; 
  typedef struct req_rmfile_  reqDelObject ; 

  
  typedef struct req_setattr_ {
     md_auth        auth ;         /*  -->   */
     md_id_t        id ;           /*  -->   */
     md_permission  perm ;         /*  -->   */
     md_unix        attr ;         /*  <->   */
  } reqSetAttr ; 
  
  typedef struct req_makelink_ {
     md_auth        auth ;         /*  -->   */
     md_id_t        id ;           /*  -->   */
     md_permission  perm ;         /*  -->   */
     char           name[1024] ;   /*  -->   */     
     char           path[1024] ;   /*  -->   */     
     md_unix        attr ;         /*  -->   */
     md_id_t        resID ;        /*  <--   */
  } reqMakeLink ; 
  typedef struct req_makelink_  reqReadLink ;
  
  typedef struct req_rename_ {
     md_auth      auth ;               /*  -->   */
     struct {
         md_id_t        id ;           /*  -->   */
         md_permission  perm ;         /*  -->   */
         char           name[1024] ;   /*  -->   */     
     } from , to ;
  } reqRename ;

  typedef struct req_readdata_ {
     md_auth        auth ;         /*  -->   */
     md_id_t        id ;           /*  -->   */
     md_permission  perm ;         /*  -->   */
     md_long        offset ;       /*  -->   */
     md_long        size ;         /*  -->   */
     md_unix        attr ;         /*  <--   */
     char           data[MAX_REQ_DATA_SIZE];  /*  <->   */
  } reqReadData ;
  typedef struct req_readdata_  reqWriteData ;

  typedef struct req_findid_ {
     md_auth        auth ;         /*  -->   */
     md_id_t        id ;           /*  -->   */
     md_permission  perm ;         /*  -->   */
     md_id_t        parentId ;     /*  -->   */
     md_long        offset ;       /*  -->   */
     md_long        size ;         /*  -->   */
     md_unix        attr ;         /*  <--   */
     char           data[MAX_REQ_DATA_SIZE];  /*  <->   */
  } reqFindId ;
  
  typedef struct req_remove_postion_ {
     md_auth        auth ;         /*  -->   */
     md_id_t        id ;           /*  -->   */
     md_id_t        rmId ;         /*  -->   */
     md_long        position ;     /*  -->   */
  } reqRemovePosition ;

  typedef struct req_setsize_ {
     md_auth        auth ;         /*  -->   */
     md_id_t        id ;           /*  -->   */
     md_permission  perm ;         /*  -->   */
     md_long        size ;         /*  -->   */
  } reqSetSize ;
  
  typedef struct req_setperm_ {
     md_auth        auth ;         /*  -->   */
     md_id_t        id ;           /*  -->   */
     md_permission  perm ;         /*  -->   */
     char           name[1024] ;   /*  -->   */     
     md_permission  newPerm ;      /*  -->   */
  } reqSetPerm ;
  
  typedef struct req_getroot_ {
     md_auth        auth ;         /*  -->   */
     md_id_t        id ;           /*  <--   */
     md_id_t        config ;       /*  <--   */
  } reqGetRoot ;
  
  typedef struct req_truncate_ {
     md_auth        auth ;         /*  -->   */
     md_id_t        id ;           /*  -->   */
     md_permission  perm ;         /*  -->   */
  } reqTruncate ;
  typedef struct req_forcesize_ {
     md_auth        auth ;         /*  -->   */
     md_id_t        id ;           /*  -->   */
     md_permission  perm ;         /*  -->   */
     md_long        size  ;        /*  -->   */
     md_long        sizeHigh  ;    /*  -->   */
  } reqForceSize ;
  typedef struct req_chparent_ {
     md_auth        auth ;         /*  -->   */
     md_id_t        id ;           /*  -->   */
     md_id_t        parent ;       /*  -->   */
  } reqChParent ;
  typedef struct req_get_chain_ {
       md_auth        auth ;         /*  -->   */
       md_id_t        dir ;          /*  -->   */
       md_id_t        child ;        /*  -->   */
       md_id_t        parent ;       /*  <--   */
       md_dir_item    item ;         /*  <--   */
  } reqGetChain  ;
  typedef struct req_ext_item_ {
       md_long      cookie ;
       md_dir_item  item ;
  } reqExtItem  ;
  typedef struct req_readdir_ {
     md_auth        auth ;         /*  -->   */
     md_id_t        id ;           /*  -->   */
     md_permission  perm ;         /*  -->   */
     md_long        cookie ;       /*  -->   */
     md_long        count  ;       /*  <->   */
     struct req_ext_item_  e[1] ;  /*  <--   */     
  } reqReadDir ; 
  
#define RBR_DUMMY        (1)
#define RBR_FIRST        (0x10)
#define RBR_GETROOT      (0x10)
#define RBR_GET_RECORD   (0x11)
#define RBR_GET_ATTR     (0x12)
#define RBR_LOOKUP       (0x13)
#define RBR_MKDIR        (0x14)
#define RBR_SET_ATTR     (0x15)
#define RBR_RMDIR        (0x16)
#define RBR_READDIR      (0x17)
#define RBR_MKFILE       (0x18)
#define RBR_RMFILE       (0x19)
#define RBR_RENAME       (0x1A)
#define RBR_MAKELINK     (0x1B)
#define RBR_READLINK     (0x1C)
#define RBR_READDATA     (0x1D)
#define RBR_WRITEDATA    (0x1E)
#define RBR_SETSIZE      (0x1F)
#define RBR_SETPERM      (0x20)
#define RBR_TRUNCATE     (0x21)
#define RBR_RM_FROM_DIR  (0x22)
#define RBR_ADD_TO_DIR   (0x23)
#define RBR_CH_PARENT    (0x24)
#define RBR_DEL_OBJECT   (0x25)
#define RBR_FORCE_SIZE   (0x26)
#define RBR_BACKUP       (0x27)
#define RBR_LOOKUP_ONLY  (0x28)
#define RBR_I_COMMAND    (0x29)
#define RBR_GET_CHAIN    (0x2a)
#define RBR_FIND_ID      (0x2b)
#define RBR_MOD_LINK     (0x2c)
#define RBR_SET_ATTRS    (0x2d)
#define RBR_RM_POSITION  (0x2e)
#define RBR_MOD_FLAGS    (0x2f)
#define RBR_LAST         (0x2f)

#define RBR_OBJ_FILE      (1)
#define RBR_OBJ_DIR       (2)
#define RBR_OBJ_LINK      (3)

#define SRB_ERROR  (1)
#define SRB_OK     (0)

#endif
