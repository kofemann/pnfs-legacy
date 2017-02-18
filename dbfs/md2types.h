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
#ifndef __MD2TYPES__H__
#define __MD2TYPES__H__

#include <time.h>
#ifdef  SYSTEM7
#include "unistd.h"
#else
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <dirent.h>
#endif
#include "dbglue.h"

/*
  the persistant storage classes
 */
#include "md2ptypes.h"
/*---------------------------------------*/

/* 
 * to get rid of the bloody bzero stuff
#define bzero(p,s)      memset((char*)(p),0,(s))
 */

/*
 * error codes
 */
#define  MDEbase         (-1010)
#define  MDEnoLock       (-1010)
#define  MDEnoCommit     (-1011)
#define  MDEnotFound     (-1012)
#define  MDEeof          (-1013)
#define  MDEdbOpen       (-1014)
#define  MDEnotRegular   (-1015)
#define  MDEdbError      (-1016)
#define  MDEpanic1       (-1017)
#define  MDEnotDirectory (-1018)
#define  MDEnotEmpty     (-1019)
#define  MDEnoSpace      (-1020)
#define  MDEnoDir        (-1021)
#define  MDEexists       (-1022)
#define  MDEnotAllowed   (-1023)
#define  MDEdbMissmatch  (-1024)
#define  MDEnoIO         (-1025)
#define  MDEquota        (-1026)
#define  MDEdbXsearch    (-1027)
#define  MDElast         (-1027)

#define   mdl_RDWR      (1)
#define   mdl_RDONLY    (2)
/*
 * ----------------------------------------------------------------------------
           the nfs interface
 * ----------------------------------------------------------------------------
 */
typedef struct {

         md_id_t   id ;
         md_id_t   mountID ;
   md_permission   permission ;
   
} md_fhandle ;
typedef md_fhandle mdFhandle ;
typedef struct {

        md_uid_t   uid ;
        md_gid_t   gid ;
        md_gid_t   gids[16] ;
           short   gidsLen ;
   unsigned long   host ;
            long   priv ;
   struct timeval  timestamp ;
         md_id_t   mountID ;
	 
   
} md_auth ;
/*
 * ----------------------------------------------------------------------------
           the scan directory stuff
 * ----------------------------------------------------------------------------
 */
 typedef struct {
     int     hash ,position ;
     int     hashBlock , hashRel ;
     int     posBlock  , posRel ;
 } md_dir_cookie ;
 /*
 * ----------------------------------------------------------------------------
 *           the cache level
 * ----------------------------------------------------------------------------
 */
#define mdrbGet    (1)
#define mdrbPut    (2)
#define mdrbDelete (3)

typedef struct {

         md_id_t    id ;
   struct mdHead   *next ;
             int    command ;
} mdrbHead ;

typedef struct {

   mdrbHead  head ;
   mdRecord  body ;
   
} mdrbRecord ;


typedef struct {

   int hashSize ;
   mdrbHead  **root ;
   MDX_FILE  *db ;
   
}  mdrbRoot ;

#define mdrbHash(r,id)      (((id).low>>4)%(r)->hashSize)

#define mdrbIsEqualID(x,y)   (!memcmp((char*)&(x),(char*)&(y),sizeof(x))

#define BM_NORMAL    (0)
#define BM_BACKUP    (1)
#define BM_RECOVER   (2)
/*
 * ----------------------------------------------------------------------------
           the private structure
 * ----------------------------------------------------------------------------
 */
 typedef struct {
 
      MDX_FILE  db  ;
      MDX_FILE  dbBackup  ;
      int       accessMode ;
      int       backupMode ;
      char      dbName[1024] ;
      char      dbBackupName[1024] ;
      struct {
          md_id_t   next ;
          int       count , inc ;
      } id ;
      struct {
          int            valid , eof ;
          md_dir_cookie  current ;
          md_record      dir , hash , block ;
      } dir ;
      mdrbRoot *mdrb ;
 }   md_private ;
 
 typedef md_private  MDL ;
  
 typedef int (*modDirItemFunc)(MDL *mdl, md_dir_item *item , void *data ) ;
/* 
 * for the directory walk functions
 */
 typedef int (md_filter_func)(MDL *m,md_id_t id , void *g , md_dir_item *i ); 

/*
 * ----------------------------------------------------------------------------
 *           the external event handler
 * ----------------------------------------------------------------------------
 */
typedef struct {
  md_id_t       dirId ;
  md_permission dirPermission ;
  md_dir_item   objEntry ;
} md_e_dir_entry ;

int md2ExtHandlerRENAME( MDL *mdl , md_e_dir_entry *from , md_e_dir_entry *to );
int md2ExtHandlerREMOVE( MDL *mdl , md_e_dir_entry *obj ) ;
int md2ExtHandlerINIT( MDL *mdl );

#ifdef ExtendedExtenalHandler
 typedef struct {
    typedef struct {    
       char name[MAX_NAME_LENGTH] ;       
    } file ;
    typedef struct {   
      md_record  *inode ;
    } dir ;
     
 } md_e_dir ;
 
 typedef union {
    md_e_dir   dir ;
 /*
    md_e_file  file ;
 */
 } md_e_object ;
#define mdeDirFileName(o)              ((o)->dir.file.name)
#define mdeDirDirectoryTagGet(o,tn)     
#endif  


typedef struct {
    char  * cursor , * base ; 
    int     rest , size ;
    FILE  * stream ;
} md2sp ;

typedef md2sp md2File ;


#define md2sfGetBuffer(x)  ((x)->base)
#define md2sfGetSize(x)    ((x)->size)

#define md_scan_records
/*
 * ----------------------------------------------------------------------------
 *           the function definitions
 * ----------------------------------------------------------------------------
 */
int   md2Init( char * (*getenv)(const char *name) ) ;
int   md2Close( MDL *mdl );
MDL * md2OpenReadWrite( char *dbName , int *rcode, int dreg );
MDL * md2OpenReadOnly( char *dbName , int *rcode );
int   md2Create( char *dbName , int db  );
void  md2PrepareRoot( mdRecord *mdr , int db );
int   md2GetIdRange( MDL * mdl , int range );
int   md2GetNextId( MDL *mdl , md_id_t *id );
int   md2Flush( MDL *mdl , int flags );
#ifdef md_scan_records
int   md2GetNextKey( MDL * mdl , md_id_t this , md_id_t *id );
int   md2GetFirstKey( MDL * mdl , md_id_t *id );
#endif
/*
 * the basic io and lock stuff
 */
int md2Ioctl( MDL *mdl , char * level , int argc , char * argv [] ,
                         int * replyLen , char * reply ) ;
int md2PutRecord( MDL *mdl , mdRecord *mdr , int );
int md2DeleteRecord( MDL *mdl , md_id_t id , int );
int md2GetRecord( MDL *mdl , mdRecord *mdr , int );
#ifdef USE_MD_LOCKS
int md2ReadLock( MDL *mdl ) ;
int md2WriteLock( MDL *mdl ) ;
int md2CommitLock( MDL *mdl ) ;
int md2AbortLock( MDL *mdl ) ;
#else
#define  md2InitLock(mdl)      (0)
#define  md2CloseLock(mdl)     (0)
#define  md2ReadLock(mdl)      mdxReadLock((mdl)->db)
#define  md2WriteLock(mdl)     mdxWriteLock((mdl)->db) 
#define  md2CommitLock(mdl)    mdxCommitLock((mdl)->db)  
#define  md2AbortLock(mdl)     mdxAbortLock((mdl)->db)  
#endif
/*
 *  the print routines
 */
md2File * md2xPrintRecord(  mdRecord *mdr );
md2File * md2xPrintAttributes(  mdRecord *mdr );
md2File * md2xPrintTags(  MDL * mdl , mdRecord *mdr ) ;
md2File * md2xPrintXTags(  MDL * mdl , mdRecord *mdr ) ;
md2File * md2xPrintConst( ) ;
void   md2PrintRecord( FILE *f , mdRecord *mdr );
char * md2PrintID( md_id_t id );
char * md2PrintFhandle( mdFhandle m );
char * md2PrintTypes( md_type_t t );
char * md2PrintPermission( md_permission id );
char * md2PrintUnixAttrS( md_unix *a );
#ifdef oldPrintRoutines
void   md2PrintHeader( FILE *f , md_head *head );
void   md2PrintDirInode( FILE *f , md_dir_inode *dirInode );
void   md2PrintDirHash( FILE *f , md_dir_hash  *dirHash ) ;
void   md2PrintDirData( FILE *f , md_dir_data  *dirData );
void   md2PrintFileInode( FILE *f , md_file_inode *fileInode );
void   md2PrintFileData( FILE *f , md_file_data  *dirData );
void   md2DumpMemory( FILE *out , unsigned char *p , long address , long rest );
void   md2PrintUnixAttr( FILE *f , md_unix *attr );
#endif
int    md2PrintHashEntries( MDL * mdl , md_id_t dirID , FILE *f );

/*
 * here we go
 */
int    md2GetRootStatistics( MDL * mdl , md_statistics *st );
int    md2SetRootStatistics( MDL * mdl , md_statistics *st );
int    md2GetRootConfig( MDL * mdl , md_id_t *configID );
int    md2SetRootConfig( MDL * mdl , md_id_t *configID );
int    md2GetDbId( MDL * mdl , long *dbID );
void   md2ScanId( char *s , md_id_t *id );
void   md2ScanIdLevel( char *s , md_id_t *id , int *level );
void   md2ScanPermission( char *s , md_permission *id );
int    md2GetRootDir( MDL *mdl , md_id_t *fsMountID ) ;
int    md2GetNewRecord( MDL * mdl , mdRecord *mdr );
int    md2MakeNewDirectory( MDL * mdl , md_id_t dir , md_id_t *id );
int    md2NewDirectoryData( MDL * mdl , mdRecord *mdr ) ;
int    md2GetDirectoryInfos( MDL * mdl , mdRecord *dir ,
                             mdRecord *hash , char *name );
int    md2HushFunc0( char *name , int size );
int    md2AddToDirectory( MDL * mdl , md_id_t dirID , char *name , md_id_t id );
int    md2MakeDirectory( MDL * mdl , md_id_t dirID , char *name , md_id_t *resID );
int    md2PrintDirectory( MDL * mdl , md_id_t dirID ,int mode , FILE *f );
int    md2CountDirectoryEntries( MDL * mdl , md_id_t dir );
int    md2FindEntryByID( MDL * mdl ,md_id_t parentID ,md_id_t childID ,md_dir_item *item);
int    md2UpdateDirTime( MDL * mdl , md_id_t dirID );
int    mdFindDirectoryEntry( MDL * mdl , md_id_t dirID , char *name , md_dir_item *item );
int    mdFindDirectoryEntryByID( MDL * mdl , md_id_t id ,  md_dir_item *item );
int    mdUpdateDirectoryEntry( MDL * mdl , md_id_t dirID , char *name , md_dir_item *item);
int    md2ExtLookup( MDL * mdl , md_id_t dirID , char *name ,
                     md_id_t *resID , md_dir_item *item         );
int    md2RemoveFromDirectory( MDL * mdl , md_id_t dirID , char *name , md_id_t *resID);
int    md2DeleteDirectory( MDL * mdl , md_id_t dirID );
int    md2RemoveDirectory( MDL * mdl , md_id_t dirID , char *name );
int    md2RemoveFile( MDL * mdl , md_id_t dirID , md_permission perm, char *name );
int    md2MakeNewFile( MDL * mdl , md_id_t dir , md_id_t *id );
int    md2MakeFile( MDL * mdl , md_id_t dirID , char *name , md_id_t *resID );
int    md2MakeFilePerm( MDL * mdl , md_id_t dirID , md_permission perm ,  char *name ,
                                    md_id_t *resID , md_permission *resPerm );
int    md2DeleteFile( MDL * mdl , md_id_t id );
int    md2WriteData( MDL * mdl , md_id_t id , int mode , char *b , long off , long size );
int    md2WriteDataLow( MDL * mdl , mdRecord *base , int mode , char *b , long off , long size );
int    md2WriteDataLowLarge( MDL * mdl , mdRecord *base , int mode , char *b , long off , long size );
int    md2WriteDataLowSmall( MDL * mdl , mdRecord *base , int mode , char *b , long off , long size );
int    md2WriteDataLowUpgrade( MDL * mdl , mdRecord *base , int mode , mdRecord *rec );
int    md2WriteDataPerm( MDL * mdl , md_id_t id  , md_permission p  , char *b , long off , long size );
int    md2ReadData( MDL * mdl , md_id_t id , int mode , char *b , long off , long size );
int    md2ReadDataLow( MDL * mdl , mdRecord *base , int mode , char *b , long off , long size );
int    md2ReadDataLowLarge( MDL * mdl , mdRecord *base , int mode , char *b , long off , long size );
int    md2ReadDataLowSmall( MDL * mdl , mdRecord *base ,mdRecord *block , int mode , char *b , long off , long size );
int    md2ReadDataPerm( MDL * mdl , md_id_t id, md_permission p  ,
                                    md_id_t mountID,
                                    char *b , long off , long size );
int    md2GetAttribute( MDL * mdl , md_id_t id , int mode , md_attr *attr);
int    md2GetExtAttribute( MDL * mdl , md_id_t id , int mode , md_attr *attr , mdRecord *rec);
int    md2GetExtAttributePerm( MDL * mdl , md_id_t id , md_permission perm ,
                               md_attr *attr , mdRecord *rec );
int    md2SetAttribute( MDL * mdl , md_id_t id , int mode , md_attr *attr);
int    md2TruncateZero( MDL * mdl , md_id_t id , int level  );
int    md2GetUnixAttr( MDL * mdl , md_id_t id , int mode , md_unix *uattr);
int    md2ModifyUnixAttr( MDL * mdl , md_id_t id , int mode , md_unix *uattr);
int    md2ModifyUnixAttrs( MDL * mdl , md_id_t id , int mode , md_unix *uattr);
int    md2ModUnixAttr( MDL * mdl , md_id_t id , int mode , md_unix *uattr);
void   md2SetMostAttr(  md_unix *uattr , md_unix *attr );
void   md2SetMostAttrs(  md_unix *uattr , md_unix *attr );
int    md2ChangeParent( MDL * mdl , md_id_t id , md_id_t parent  );
#ifdef modPermissionFunction
int    md2ModPermission( MDL * mdl , md_id_t dirID , char *n , md_permission perm );
#endif
int    md2ExtLookupPerm( MDL * mdl , md_id_t dirID , md_permission *perm , 
                      char *name , md_id_t *resID , md_dir_item *item         );
int    md2MakeNewLink( MDL * mdl , md_id_t dir , md_id_t *id );
int    md2WriteLink( MDL * mdl , md_id_t id , int level , char *path );
int    md2MakeLink( MDL * mdl , md_id_t dirID , char *name ,
                    int level , char *path ,  md_id_t *resID );
int    md2ReadLink( MDL * mdl , md_id_t linkID , int level , char *path );
int    md2AddToDirectoryOnly( MDL * mdl , md_id_t dirID , char *name , md_id_t id );
int    md2ExtRemoveFromDirectory( MDL * mdl , md_id_t dirID , char *name , md_id_t *resID ,
                                  md_dir_item *item );
int    md2ModWhatever( MDL * mdl , md_id_t dirID , char *name , modDirItemFunc func , void * data );
int    md2ForceSize( MDL *mdl , md_id_t id , int level , md_long size ) ;
int    md2ForceSizePerm( MDL *mdl , md_id_t id , md_permission perm  , md_long size )
;
/*
 *   directory interface
 */
int    md2_dir_open( MDL *mdl , md_id_t id , md_dir_cookie *cookie ) ;
int    md2_dir_next( MDL *mdl , md_dir_item *item , md_dir_cookie *cookie ) ;
int    md2_dir_set( MDL *mdl , md_dir_cookie *cookie ) ;
int    md2_dir_close( MDL *mdl ) ;
int    md2DirClose( MDL * mdl );
int    md2DirNext( MDL * mdl , md_cookie *cky , md_dir_item *item);
int    md2DirOpen( MDL * mdl , md_id_t id , md_cookie *cky );
/*
 * dir_item modification function
 */
int    md2ModPermPtr( MDL *mdl, md_dir_item *item , void *data );
int    md2ModExpirePtr( MDL *mdl, md_dir_item *item , void *data );
int    md2SetEntryOnlyPtr( MDL *mdl, md_dir_item *item , void *data );

#define  md2ModPermission(m,dir,name,perm)  md2ModWhatever(m,dir,name,md2ModPermPtr,(void*)&perm)
#define  md2ModExpire(m,dir,name,perm)      md2ModWhatever(m,dir,name,md2ModExpirePtr,(void*)&perm)
#define  md2SetEntryOnly(m,dir,name)        md2ModWhatever(m,dir,name,md2SetEntryOnlyPtr,NULL)

/*
 * the tag facilities
 */
int md2AddTagToDirectory( MDL * mdl , md_id_t dirID , mdRecord *tag );
int md2AddNewTagInh( MDL * mdl , md_id_t dirID , md_id_t parent , 
                     char *name , mdRecord *tag );
int md2GetTopTag( MDL *mdl , md_id_t id , mdRecord *res );
int md2WriteTag( MDL * mdl , md_id_t tagID , char *ptr ,md_long offset , md_long size);
int md2WriteTagLow( MDL * mdl ,mdRecord *tag , char *ptr ,md_long offset ,md_long size);
void md2PrintTagInode( FILE *f , md_tag_inode  *tagInode );
int md2RemoveTags( MDL * mdl , md_id_t dirID  );
int md2GetTagUnixAttr( MDL * mdl , md_id_t tagID  , md_unix *uattr);
int md2FindTag( MDL * mdl , md_id_t dirID , char *name , mdRecord *resTag  );
int md2DuplicateTags( MDL * mdl , md_id_t fromID , md_id_t toID );
int md2ReadTag( MDL * mdl , md_id_t tagID , char *ptr ,md_long offset ,md_long size);
int md2ReadTagLow( MDL * mdl ,mdRecord *tag , char *ptr ,md_long offset ,md_long size);
int md2AddNewTag( MDL * mdl , md_id_t dirID , char *name , mdRecord *tag );
int md2RemoveTag( MDL * mdl , md_id_t tagID  );
int md2GetTagName( char * string , char *tagName );
#define md2IsTagName(s)   md2GetTagName(s,NULL) 
int md2TagUnChain( MDL *mdl , mdRecord *tag );
int md2TagPropagateLow( MDL * mdl , md_id_t tagID  );
int md2TagUnChainLow( MDL *mdl , mdRecord *tag );
 
/*
 *  backward compatibility functions
 */
#define md2Lookup(mdl,dirID,name,resID)         md2ExtLookup(mdl,dirID,name,resID,NULL)
#define md2RemoveFromDirectory(m,dir,name,rid)  md2ExtRemoveFromDirectory(m,dir,name,rid,NULL)
/*
 * recursive filter stuff
 */
int  mdDirectoryTree( MDL * m,md_id_t dir,void *g,md_dir_item *i);
int  mdDirectoryWalk( MDL * m,md_id_t dir,md_filter_func f , void *glbl );

#define mdDirectoryScan(m,dir)   mdDirectoryTree(m,dir,NULL,NULL)

#endif
