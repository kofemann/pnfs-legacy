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
#ifndef __MD2PTYPES__H__
#define __MD2PTYPES__H__


#define MD2_P_VERSION    (0x30109)

#define MD2_RECORD_LENGTH   (1012)
#define MD_MAX_NAME_LENGTH  (200)
#define MD_HASH_SIZE        (128)


/*
 * the basic types
 */
typedef long             md_s_long ;
typedef unsigned long    md_long ;
typedef unsigned short   md_short   ;
typedef char             md_char ;
typedef char             md_data ;

typedef unsigned long    md_cookie ;

typedef   md_long   md_ino_t  ;
typedef   md_short  md_dev_t  ;
typedef   md_short  md_mode_t ;
typedef   md_long   md_uid_t  ;
typedef   md_long   md_gid_t  ;
typedef   md_long   md_time_t ;

typedef struct { md_short db ; md_short ext ; md_long high ; md_long low ; } md_id_t ;
typedef struct { md_long high ; md_long low ; } md_type_t ;
typedef struct { md_long high ; md_long low ; } md_vlong ;
typedef struct { md_long high ; md_long low ; } md_permission ;


typedef unsigned long    md_group_id ; 
typedef unsigned long    md_p_id ; 
typedef unsigned long    md_host_id ;
 
typedef struct { md_p_id     pid ;
                 md_host_id  host ;
                 md_long     dummy ;
               } md_lock_id ; 
               
typedef struct { md_lock_id  id ;
                 md_time_t   expire ;
               } md_lock ; 
/*
 *
 *   1  1  1  1  1  1                        
 *   5  4  3  2  1  0  9  8  7  6  5  4  3  2  1  0
 *                     |  |  |  |  |  |  |  *--*--*-- new level
 *                     |  |  |  |  |  |  +----------- modify level
 *                     |  |  |  |  |  +-------------- no IO
 *                     |  |  |  |  +----------------- modify no IO
 *                     |  |  |  +-------------------- no way back
 *                     |  |  +----------------------- modify no way back
 *                     |  +-------------------------- privileged
 *                     +----------------------------- is link only
 *
 *   3  3  2  2  2  2  2  2  2  2  2  2  1  1  1  1    
 *   1  0  9  8  7  6  5  4  3  2  1  0  9  8  7  6  
 *   *--*--*--*--*--*--*--*  *--*--*--*--*--*--*--*-- directory invisible 
 *            +-------------------------------------- lookup invisible 
 *
 *       or the directory lookup cookie (low part )
 *
 *   permssion high part 
 *
 *   1  1  1  1  1  1                        
 *   5  4  3  2  1  0  9  8  7  6  5  4  3  2  1  0
 *                     |  |  |  |  *--*--*--*--*--*-- detail of special ID ( MDO_???? )
 *    high bit 4 and 5 were added 17.11.97 for MDO_PARENT ....
 *
 *   3  3  2  2  2  2  2  2  2  2  2  2  1  1  1  1    
 *   1  0  9  8  7  6  5  4  3  2  1  0  9  8  7  6  
 *   *--*--*--*--*--*--*--*  *--*--*--*--*--*--*--*-- directory lookup cookie ( high part )
 * 
 */
#define  mdpLevelBits           (0x7)
#define  mdpModifyLevelBit      (0x8)
#define  mdpNoIOBit             (0x10)
#define  mdpModifyNoIOBit       (0x20)
#define  mdpNoWayBackBit        (0x40)
#define  mdpModifyNoWayBackBit  (0x80)
#define  mdpPrivilegedBit       (0x100)
#define  mdpEntryOnlyBit        (0x200)
#define  mdpDirInvisible        (16)
#define  mdpLookupInvisible     (24)

#define  mdpGetSpecial(p)        ((p).high&0x3F)
#define  mdpSetSpecial(p,v)      {(p).high=((p).high&(~0x3f))|((v)&0x3F);}

#define  mdpGetArgs(p)           (((p).high&0xffff0000)|(((p).low>>16)&0xffff))
#define  mdpSetArgs(p,v)         {(p).high=((p).high&0xffff)|((v)&0xffff0000); \
                                  (p).low=((p).low&0xffff)|(((v)<<16)&0xffff0000);}

#define  mdGetLevel(p)      ((p).low&mdpLevelBits)
#define  mdpGetLevel(p)     ((p).low&mdpLevelBits)
/*
#define  mdpSetLevel(p,l)    {(p).low=(l)&mdpLevelBits;}
*/
#define  mdpSetLevel(p,l)    {(p).low=((p).low&(~mdpLevelBits))|\
                              ((l)&mdpLevelBits);}


#define  mdpModifyLevel(p)  ((p).low|=mdpModifyLevelBit)
#define  mdpModifyNoIO(p)   ((p).low|=mdpModifyNoIOBit)
#define  mdpAllLevels       (8)

#define mdpDoModification(p,perm,b,m)  if((perm).low&(m)){\
                                        (p).low=((p).low&(~(b)))|\
                                        ((perm).low&(b));}
#define  mdpUnsetNoIO(p)             {(p).low&=~mdpNoIOBit;}                                              
#define  mdpIsDirInvisible(p,n)      (((p).low)&(1<<(mdpDirInvisible+(n))))
#define  mdpIsLupInvisible(p,n)      (((p).low)&(1<<(mdpLookupInvisible+(n))))
#define  mdpIsNoIO(p)                ((p).low&mdpNoIOBit)
#define  mdpIsNoWayBack(p)           ((p).low&mdpNoWayBackBit)
#define  mdpIsPrivileged(p)          ((p).low&mdpPrivilegedBit)
#define  mdpIsEntryOnly(p)           ((p).low&mdpEntryOnlyBit)
#define  mdpSetEntryOnly(p)          ((p).low|=mdpEntryOnlyBit)


#define  md_no_mode  ((md_mode_t)0xffffffff)
#define  md_no_uid   ((md_uid_t)0xffffffff)
#define  md_no_gid   ((md_gid_t)0xffffffff)
#define  md_no_time  ((md_time_t)0xffffffff)
#define  md_no_size  ((md_long)0xffffffff)

#define  MD_INODE_FLAG_NOREMOVE      (1) 
#define  MD_INODE_FLAG_NOMOVE        (2)
#define  MD_INODE_FLAG_TRUSTED_WRITE (4)
#define  MD_INODE_FLAG_TRUSTED_READ  (8)

typedef struct	{
     md_ino_t   mst_ino;
     md_dev_t   mst_dev;
     md_dev_t   mst_rdev;
     md_mode_t  mst_mode;
     md_short   mst_nlink;
     md_uid_t   mst_uid;
     md_gid_t   mst_gid;
#ifdef MD_VLONG
     md_vlong   mst_size ; 
#else
     md_long    mst_sizeHigh ;
     md_long    mst_size ;
#endif
     md_time_t  mst_atime;
     md_long	mst_spare1;
     md_time_t  mst_mtime;
     md_long	mst_spare2;
     md_time_t  mst_ctime;
     md_long    mst_blksize;
     md_long    mst_blocks;
     md_long	mst_spare4[2];
} md_unix ;

/*
 * the ID handling functions
 */
#define  mdidRoot               (0x100)
#define  mdidNULL               (0)
#define  mdidFirst              (0x1000)

#define  mdSetSpecialID(id)       {(id).low|=1;}
#define  mdClearSpecialID(id)     {(id).low&=~0x7;}
#define  mdIsSpecialID(id)        ((id).low&1)
#define  mdSetRootID(id)          {(id).db=0;(id).ext=0;(id).high=0;(id).low=(mdidRoot);}
#define  mdSetNullID(id)          {(id).db=0;(id).ext=0;(id).high=0;(id).low=0;}
#define  mdSetFirstID(id,d)       {(id).db=(d);(id).ext=0;(id).high=0;(id).low=(mdidFirst);}
#define  mdAddID(to,c)            {md_long tmp;tmp=(to).low;(to).low+=(c);\
                                   if(tmp>(to).low){tmp=(to).high;(to).high++;\
                                    if(tmp>(to).high)(to).ext++;}} 
#define  mdGetDbId(id)            ((id).db)
#define  mdStringID(id)           md2PrintID(id)
#ifdef inodeVersion1
#define  mdInodeID(id)            (id).low
#define  mdDeviceID(id)           ((id).high>>8)
#else
/*
#define  mdInodeID(id)            ((id).low|(id).db)
*/
#define  mdInodeID(id)            (((id).db<<24)|((id).low&0xFFFFF8))
#define  mdInodeLevelID(id,l)     (((id).db<<24)|((id).low&0xFFFFF8)|((l)&0x7))
#define  mdDeviceID(id)           ((id).db)
#endif
#define  mdIsNullID(i)            (((i).ext==0)&&((i).high==0)&&((i).low==0))
#define  mdIsEqualID(i,j)         (((i).db==(j).db)&&((i).ext==(j).ext)&&\
                                   ((i).high==(j).high)&&((i).low==(j).low))

#define  mdStringHandle(id)            md2PrintFhandle(id)
#define  mdStringFhandle(id)           md2PrintFhandle(id)
#define  mdStringPermission(id)        md2PrintPermission(id)

/*
 * the types
 */
#define  mdtRoot               (1<<1)
#define  mdtInode              (1<<2)
#define  mdtData               (1<<3)
#define  mdtIdList             (1<<4)
#define  mdtDirectory          (1<<5)
#define  mdtRegular            (1<<6)
#define  mdtLink               (1<<7)
#define  mdtHash               (1<<8)
#define  mdtTag                (1<<9)
#define  mdtForceIO            (1<<10)
#define  mdtLast               (10)
#define  mdtToString           "xRIDydrlHtF"


#define  mdClearType(t)             {(t).high=0;(t).low=0;}
#define  mdAddType(t,v)             {(t).high=0;(t).low|=(v);}
#define  mdRmType(t,v)              {(t).high=0;(t).low&=~(v);}
#define  mdIsType(t,v)              ((t).low&(v))
#define  mdShowType(t)              (t).low

#define  mdClearRecord(mdr)       memset((void*)(mdr),0,sizeof(*mdr))
#define  mdCopyRecord(to,from)    memcpy((void*)(to),(void*)(from),sizeof(*from))
#define  md_min(x,y)              ((x)<(y)?(x):(y))
#define  md_max(x,y)              ((x)>(y)?(x):(y))
#define  mdCopyItem(t,f)          memcpy((char*)t,(char*)f,sizeof(md_dir_item))

#define mdTagInvalid        (1<<1)
#define mdTagInherited      (1<<2)

typedef struct md_statistics_ {
     md_long  fileObjects , dirObjects ;  
     char dummy[128] ;   
} md_statistics ;

typedef struct {

   md_id_t       ID  ;
   md_type_t     type ;
   md_id_t       nextID ;
   md_id_t       baseID ;
   md_id_t       backID ;
   md_id_t       parentID ;
   time_t        cTime ;
   time_t        mTime ;
   md_long       links ;
   md_long       version ;

}  md_head ;
#define MAX_BODY_SIZE  (MD2_RECORD_LENGTH-sizeof(md_head))
typedef struct {

   md_id_t        nextFreeID ;
   md_long        DB ;
   md_id_t        configID ;
   md_statistics  statistics ;
   md_long        backupMode ;
   md_data        string[252] ;
} md_root ;
typedef struct {
  md_long function ;
  md_long size ;
  md_long entriesPerRow ;
  md_long rows ;
} md_hash_info ;

typedef struct {
  md_long bytesPerBlock ;
  md_long blocksPerHash ;
} md_file_info ;

typedef struct {
  md_long entries ;
} md_hash_head ;

typedef struct {
   md_unix     unixAttr ;
   md_id_t     chain ;
#ifdef MD_VLONG
   md_vlong    entries ; 
#else
   md_long     entriesHigh ;
   md_long     entries ;
#endif
   md_long     flags ;
   md_id_t     tag ;
   md_group_id group ;
} md_attr ;

typedef struct {
  md_long entries , maxEntries ;
} md_dir_head ;

typedef struct {
  md_id_t        ID ;
  md_permission  perm ;
  md_time_t      expire ;
  md_id_t        tag ;
  char           name[MD_MAX_NAME_LENGTH] ;
} md_dir_item ;
/*
 *      -----------  md_tag_inode ----------------
 */
#define MAX_TAG_NAME_SIZE   (62)
#define TAG_DATA_SIZE    (MAX_BODY_SIZE-sizeof(md_attr)-MAX_TAG_NAME_SIZE)
typedef struct {
    md_attr  attr ;
    md_char  name[MAX_TAG_NAME_SIZE] ;
    md_data  data[TAG_DATA_SIZE] ;
} md_tag_inode ;
/*
 *      -----------  md_dir_inode ----------------
 */
#define HASH_HANDLES ((MAX_BODY_SIZE-sizeof(md_attr)-sizeof(md_lock)\
                                    -sizeof(md_hash_info))\
                      /sizeof(md_id_t))
typedef struct {
     md_attr       attr ;
     md_hash_info  hashInfo ;
     md_id_t       hashHandle[HASH_HANDLES] ;
     md_lock       lock ;
} md_dir_inode ;
/*
 *      -----------  md_dir_hash ----------------
 */
#define HASH_POINTERS ((MAX_BODY_SIZE-sizeof(md_hash_head))/sizeof(md_id_t))
typedef struct {
     md_hash_head  hashHead ;
     md_id_t       hashPointer[HASH_POINTERS] ;
} md_dir_hash ;
typedef md_dir_hash md_file_hash ;
#define DATA_POINTERS HASH_POINTERS
/*
 *      -----------  md_dir_data ----------------
 */
#define DIR_ITEMS ((MAX_BODY_SIZE-sizeof(md_dir_head))/sizeof(md_dir_item))
typedef struct {
     md_dir_head   dirHead ;
     md_dir_item   dirItem[DIR_ITEMS] ;
} md_dir_data ;
/*
 *      -----------  md_file_inode ----------------
 */
typedef struct {
     md_attr       attr[8] ;
     md_file_info  fileInfo ;
     md_lock       lock ;
} md_file_inode ;
/*
 *      -----------  md_file_data  ----------------
 */
#define DATA_UNITS ((MAX_BODY_SIZE)/sizeof(md_data))
typedef struct {
     md_data       data[DATA_UNITS] ;
} md_file_data ;
/*
 *      -----------  md_link_data  ----------------
 */
typedef struct {
     md_data       data[DATA_UNITS] ;
} md_link_data ;



typedef struct {
   md_head head ;
   union {
      char           raw[MAX_BODY_SIZE] ;
      md_root        root ;
      md_dir_inode   dirInode ;   
      md_dir_hash    dirHash ;   
      md_dir_data    dirData ;   
      md_file_inode  fileInode ;   
      md_file_hash   fileHash ;      
      md_file_data   fileData ;      
      md_link_data   linkData ;      
      md_tag_inode   tagInode ;      
   } body ;

} md_record ;
typedef md_record mdRecord ;


#endif
