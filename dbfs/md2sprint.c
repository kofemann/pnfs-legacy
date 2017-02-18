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
/*
 */
#include "md2log.h"
#include "md2types.h"
#include "md2scan.h"

void md2sPrintRecord( md2sp *f , mdRecord *mdr );
void md2sPrintAttributes( md2sp *f , mdRecord *mdr );
void md2sPrintDirData( md2File *f , md_dir_data  *dirData );
void md2sPrintFileData( md2File *f , md_file_data  *dirData );
void md2sPrintDirInode( md2File *f , md_dir_inode *dirInode );
void md2sPrintFileInode( md2File *f , md_file_inode *fileInode );
void md2sPrintUnixAttr( md2File *f , md_unix *attr );
void md2sPrintHeader( md2File *f , md_head *head );
void md2sPrintDirHash( md2File *f , md_dir_hash  *dirHash ) ;
void md2sPrintTagInode( md2File *f , md_tag_inode  *tagInode );
void md2sPrintTags(  MDL * mdl , md2File *f , mdRecord *dir );
void md2sPrintXTags(  MDL * mdl , md2File *f , mdRecord *dir );
void md2sPrintConst(  );
void md2sDumpMemory( md2File *out , unsigned char *p , long address , long rest );

void md2sPrintf( md2sp *f , char *format , ... );

 
void md2sPrintf( md2sp *f , char *format , ... ) 
{
 va_list   args;
 int l ;

 if( f->cursor ){   
   va_start(args, format);
   vsprintf( f->cursor , format, args) ;
   va_end(args);

   l = strlen( f->cursor ) ;
   f->cursor += l ;
   f->rest   -= l   ;
   f->size   += l ;
 }
 if( f->stream ){
   va_start(args, format);
   vfprintf( f->stream , format, args) ;
   va_end(args);
 } 
   return  ;

}

void md2PrintRecord( FILE *s , mdRecord *mdr )
{
  md2File  f ;
  
  memset( (char *)&f , 0 , sizeof( f ) ) ;
  f.stream = s ;
  md2sPrintRecord( &f , mdr ) ;
  return ;
  
}
void md2PrintUnixAttr( FILE *s , md_unix *attr )
{
  md2File  f ;
  
  memset( (char *)&f , 0 , sizeof( f ) ) ;
  f.stream = s ;
  md2sPrintUnixAttr( &f , attr ) ;
  return ;
  
}
void md2DumpMemory( FILE *s , unsigned char *p , long address , long rest )
{
  md2File  f ;
  
  memset( (char *)&f , 0 , sizeof( f ) ) ;
  f.stream = s ;
  md2sDumpMemory( &f ,  p ,   address , rest );
  return ;
  
}
static md2File  defMd2File ;
static char md2FileBuffer[8*1024] ;

md2File * md2xPrintConst(  )
{
    defMd2File.stream = NULL ;
    defMd2File.base = defMd2File.cursor = md2FileBuffer ;
    defMd2File.size = 0 ;
    defMd2File.rest = sizeof(md2FileBuffer)  ;
    md2sPrintConst( &defMd2File ) ;
    
    return &defMd2File;
}
md2File * md2xPrintRecord(  mdRecord *mdr )
{
    defMd2File.stream = NULL ;
    defMd2File.base = defMd2File.cursor = md2FileBuffer ;
    defMd2File.size = 0 ;
    defMd2File.rest = sizeof(md2FileBuffer)  ;
    md2sPrintRecord( &defMd2File , mdr ) ;
    
    return &defMd2File;
}
md2File * md2xPrintAttributes(  mdRecord *mdr )
{
    defMd2File.stream = NULL ;
    defMd2File.base = defMd2File.cursor = md2FileBuffer ;
    defMd2File.size = 0 ;
    defMd2File.rest = sizeof(md2FileBuffer)  ;
    md2sPrintAttributes( &defMd2File , mdr ) ;
    
    return &defMd2File;
}
md2File * md2xPrintTags(  MDL * mdl , mdRecord *mdr )
{
    defMd2File.stream = NULL ;
    defMd2File.base = defMd2File.cursor = md2FileBuffer ;
    defMd2File.size = 0 ;
    defMd2File.rest = sizeof(md2FileBuffer)  ;
    md2sPrintTags( mdl , &defMd2File , mdr ) ;
    
    return &defMd2File;
}
md2File * md2xPrintXTags(  MDL * mdl , mdRecord *mdr )
{
    defMd2File.stream = NULL ;
    defMd2File.base = defMd2File.cursor = md2FileBuffer ;
    defMd2File.size = 0 ;
    defMd2File.rest = sizeof(md2FileBuffer)  ;
    md2sPrintXTags( mdl , &defMd2File , mdr ) ;
    
    return &defMd2File;
}
void md2sPrintConst(  md2File *f )
{
  md2sPrintf(f,"MD2_P_VERSION=%x\n",MD2_P_VERSION); 
  md2sPrintf(f,"MD2_RECORD_LENGTH=%d\n",MD2_RECORD_LENGTH); 
  md2sPrintf(f,"MD_MAX_NAME_LENGTH=%d\n",MD_MAX_NAME_LENGTH); 
  md2sPrintf(f,"MD_HASH_SIZE=%d\n",MD_HASH_SIZE); 
  md2sPrintf(f,"MAX_BODY_SIZE=%d\n",MAX_BODY_SIZE); 
  md2sPrintf(f,"MAX_TAG_NAME_SIZE=%d\n",MAX_TAG_NAME_SIZE); 
  md2sPrintf(f,"TAG_DATA_SIZE=%d\n",TAG_DATA_SIZE); 
  md2sPrintf(f,"HASH_HANDLES=%d\n",HASH_HANDLES); 
  md2sPrintf(f,"HASH_POINTERS=%d\n",HASH_POINTERS); 
  md2sPrintf(f,"DATA_POINTERS=%d\n",DATA_POINTERS); 
  md2sPrintf(f,"DIR_ITEMS=%d\n",DIR_ITEMS); 
  md2sPrintf(f,"DATA_UNITS=%d\n",DATA_UNITS); 

}
void md2sPrintTags(  MDL * mdl , md2File *f , mdRecord *dir )
{
 mdRecord tag  ;
 int rc ;
 char *h ;
   if( !  ( mdIsType( dir->head.type , mdtInode )  &&
            mdIsType( dir->head.type , mdtDirectory )  ) ){
            
        md2sPrintf(f," Fatal Problems reading tags\n" ) ;
        return ;
   }
   
 if(md2ReadLock( mdl ))return  ;
 
    for( tag.head.ID = dir -> body.dirInode.attr.tag  ;
         ! mdIsNullID( tag.head.ID ) ;
         tag.head.ID = tag.head.nextID           ){
#ifdef fullTagSyntax        
        if( rc  =  md2GetRecord( mdl , &tag , 0 ) )goto abort ;
        h = md2PrintUnixAttrS( &tag.body.tagInode.attr.unixAttr );
        md2sPrintf(f,"%s %2x %s\n", h , 
                   tag.body.tagInode.attr.flags ,
                   tag.body.tagInode.name  ) ;
#else
        if( rc  =  md2GetRecord( mdl , &tag , 0 ) )goto abort ;
        md2sPrintf(f,".(tag)(%s)\n", tag.body.tagInode.name  ) ;
#endif
    }

 if(md2CommitLock( mdl ))return  ;
 return  ;
 
abort :

 md2AbortLock( mdl ) ;
 return  ;  
   
}
void md2sPrintXTags(  MDL * mdl , md2File *f , mdRecord *dir )
{
 mdRecord tag  ;
 int rc ;
 char *h ;
   if( !  ( mdIsType( dir->head.type , mdtInode )  &&
            mdIsType( dir->head.type , mdtDirectory )  ) ){
            
        md2sPrintf(f," Fatal Problems reading tags\n" ) ;
        return ;
   }
   
 if(md2ReadLock( mdl ))return  ;
 
    for( tag.head.ID = dir -> body.dirInode.attr.tag  ;
         ! mdIsNullID( tag.head.ID ) ;
         tag.head.ID = tag.head.nextID           ){
         
        if( rc  =  md2GetRecord( mdl , &tag , 0 ) )goto abort ;
        md2sPrintf(f,"%s %s\n",
                   mdStringID(tag.head.ID) , 
                   tag.body.tagInode.name  ) ;

    }

 if(md2CommitLock( mdl ))return  ;
 return  ;
 
abort :

 md2AbortLock( mdl ) ;
 return  ;  
   
}
void md2sPrintAttributes( md2File *f , mdRecord *mdr )
{
 md_unix *attr ;
 
 if( mdIsType( mdr->head.type , mdtInode ) ){
   
   if( mdIsType( mdr->head.type , mdtDirectory ) ){
      attr = &( mdr -> body.dirInode.attr.unixAttr ) ;
      md2sPrintf(f,"%ho:%d:%d:%x:%x:%x\n" , 
                 attr->mst_mode,
                 attr->mst_uid,
                 attr->mst_gid,
                 attr->mst_atime,
                 attr->mst_mtime,
                 attr->mst_ctime ) ;
   }else if( mdIsType( mdr->head.type , mdtRegular   ) ){
      int i ;
      for( i  = 0 ; i < 8 ; i++ ){
         attr = &( mdr -> body.fileInode.attr[i].unixAttr ) ;
         md2sPrintf(f,"%ho:%d:%d:%x:%x:%x\n" , 
                    attr->mst_mode,
                    attr->mst_uid,
                    attr->mst_gid,
                    attr->mst_atime,
                    attr->mst_mtime,
                    attr->mst_ctime ) ;
      }
   }else if( mdIsType( mdr->head.type , mdtLink   ) ){
      int i ;
      for( i  = 0 ; i < 8 ; i++ ){
         attr = &( mdr -> body.fileInode.attr[i].unixAttr ) ;
         md2sPrintf(f,"%ho:%d:%d:%x:%x:%x\n" , 
                    attr->mst_mode,
                    attr->mst_uid,
                    attr->mst_gid,
                    attr->mst_atime,
                    attr->mst_mtime,
                    attr->mst_ctime ) ;
      }
   }else{
      md2sPrintf( f , "0\n" ) ;
      return ;
   }

       
 }else{
    md2sPrintf( f , "0\n" ) ;
 } 
 return ;
}
void md2sPrintRecord( md2File *f , mdRecord *mdr )
{
 
md2sPrintf(f," ------------------------------------------------------\n" ) ;

md2sPrintHeader( f , &mdr->head );

if( mdIsType( mdr->head.type , mdtInode ) ){

   if( mdIsType( mdr->head.type , mdtDirectory ) ){
       md2sPrintf(f," Type          : %s\n" , "Directory ( Inode )" ) ;
       md2sPrintDirInode( f , &mdr -> body.dirInode ) ;
   }else if( mdIsType( mdr->head.type , mdtRegular ) ){
       md2sPrintf(f," Type          : %s\n" , "Regular ( Inode )" ) ;
       md2sPrintFileInode( f , &mdr -> body.fileInode ) ;
   }else if( mdIsType( mdr->head.type , mdtLink ) ){
       md2sPrintf(f," Type          : %s\n" , "Link ( Inode )" ) ;
       md2sPrintFileInode( f , &mdr -> body.fileInode ) ;
   }else if( mdIsType( mdr->head.type , mdtTag ) ){
       md2sPrintf(f," Type          : %s\n" , "Tag ( Inode )" ) ;
       md2sPrintTagInode( f , &mdr -> body.tagInode ) ;
   }
   
}else if(   mdIsType( mdr->head.type , mdtRoot ) ){
   md2sPrintf(f," Next Free ID  : %s\n", mdStringID(mdr -> body.root.nextFreeID) ) ;
   md2sPrintf(f," Database ID   : %d\n", mdr -> body.root.DB ) ;
   md2sPrintf(f," Config ID     : %s\n", mdStringID(mdr -> body.root.configID) ) ;
   md2sPrintf(f," Stat Dirs     : %d\n", mdr -> body.root.statistics.dirObjects ) ;
   md2sPrintf(f," Stat Files    : %d\n", mdr -> body.root.statistics.fileObjects ) ;
   return ;
}else if(   mdIsType( mdr->head.type , mdtData ) ){
   if( mdIsType( mdr->head.type , mdtDirectory ) ){
       md2sPrintf(f," Type          : %s\n" , "Directory ( Data )" ) ;
       md2sPrintDirData( f , &mdr -> body.dirData ) ;
   }else if( mdIsType( mdr->head.type , mdtRegular ) ){
       md2sPrintf(f," Type          : %s\n" , "Regular ( Data )" ) ;
       md2sPrintFileData( f , &mdr -> body.fileData ) ;
   }
}else if(   mdIsType( mdr->head.type , mdtHash ) ){
   if( mdIsType( mdr->head.type , mdtDirectory ) ){
       md2sPrintf(f," Type          : %s\n" , "Directory ( Hash )" ) ;
       md2sPrintDirHash( f , &mdr -> body.dirHash ) ;
   }else if( mdIsType( mdr->head.type , mdtRegular ) ){
       md2sPrintf(f," Type          : %s\n" , "Regular ( Hash )" ) ;
       md2sPrintDirHash( f , &mdr -> body.dirHash ) ;
   }
}
return ;
} 
void md2sPrintDirData( md2File *f , md_dir_data  *dirData )
{
  int i ;
   md2sPrintf(f," Max Directory Entries : %d\n" , dirData -> dirHead.maxEntries ) ;
   md2sPrintf(f,"     Directory Entries : %d\n" , dirData -> dirHead.entries ) ;
   for( i = 0 ; i < dirData -> dirHead.entries ; i++ )
      md2sPrintf( f ," %s %s %s\n",
               mdStringID(dirData -> dirItem[i].ID),
               mdStringPermission(dirData -> dirItem[i].perm ),
               dirData -> dirItem[i].name  ) ;
   
   return ;
}
void md2sPrintFileData( md2File *f , md_file_data  *dirData )
{
  md2sDumpMemory( f , (unsigned char *)(dirData->data) , 0 , DATA_UNITS );
  return ;
}
void md2sPrintDirHash( md2File *f , md_dir_hash  *dirHash ) 
{
  int i ;
  
   md2sPrintf(f," Hash Pointer Entries  : %d\n" , dirHash -> hashHead.entries ) ;
   for( i = 0 ; i < dirHash -> hashHead.entries ; i++ ){
      md2sPrintf( f," %s",mdStringID(dirHash -> hashPointer[i]) ) ;
      if( ((i+1) % 4) == 0  )md2sPrintf(f,"\n"); 
   }
   if( ( i % 4) != 0  )md2sPrintf(f,"\n"); 
   return ;
}
void md2sPrintDirInode( md2File *f , md_dir_inode *dirInode )
{
   int i ;
   
   md2sPrintUnixAttr( f , &dirInode -> attr.unixAttr );
   md2sPrintf(f," Tag                : %s\n" , mdStringID( dirInode -> attr.tag ) );
   md2sPrintf(f," Group              : %d\n" , dirInode -> attr.group ) ;
   md2sPrintf(f," Entries            : %d\n" , dirInode -> attr.entries ) ;
   md2sPrintf(f," Hash Function      : %d\n" , dirInode -> hashInfo.function ) ;
   md2sPrintf(f," Hash Size          : %d\n" , dirInode -> hashInfo.size ) ;
   md2sPrintf(f," Hash EntriesPerRow : %d\n" , dirInode -> hashInfo.entriesPerRow ) ;
   md2sPrintf(f," Hash Rows          : %d\n" , dirInode -> hashInfo.rows ) ;
   for( i = 0 ; i < dirInode -> hashInfo.rows ; i++ ){
      md2sPrintf( f," %s",mdStringID(dirInode -> hashHandle[i]) ) ;
      if( ((i+1) % 4) == 0  )md2sPrintf(f,"\n"); 
   }
  md2sPrintf(f,"\n");
  return ;
}
void md2sPrintFileInode( md2File *f , md_file_inode *fileInode )
{
 int i ;  
   md2sPrintf(f," Info bytesPerBlock : %d\n" , fileInode -> fileInfo.bytesPerBlock ) ;
   md2sPrintf(f," Info blocksPerhash : %d\n" , fileInode -> fileInfo.blocksPerHash ) ;
   md2sPrintUnixAttr( f , &fileInode -> attr[0].unixAttr );
   for( i = 0 ; i < 8 ; i++ ){
     md2sPrintf(f," Entries(%d)       : %d\n" , i,fileInode -> attr[i].entries ) ;
     md2sPrintf(f," Chain(%d)         : %s\n" , i,mdStringID(fileInode -> attr[i].chain)  ) ;
     md2sPrintf(f," Group(%d)         : %d\n" , i,fileInode -> attr[i].group  ) ;
   }
  md2sPrintf(f,"\n");
  return ;
}
void md2sPrintUnixAttr( md2File *f , md_unix *attr )
{
   md2sPrintf(f," mst_dev        : %d\n" , attr->mst_dev ) ;
   md2sPrintf(f," mst_ino        : %d\n" , attr->mst_ino ) ;
   md2sPrintf(f," mst_mode       : %ho\n" , attr->mst_mode ) ;
   md2sPrintf(f," mst_nlink      : %d\n" , attr->mst_nlink ) ;
   md2sPrintf(f," mst_uid        : %d\n" , attr->mst_uid ) ;
   md2sPrintf(f," mst_gid        : %d\n" , attr->mst_gid ) ;
   md2sPrintf(f," mst_rdev       : %d\n" , attr->mst_rdev ) ;
   md2sPrintf(f," mst_size       : %d\n" , attr->mst_size) ;
   md2sPrintf(f," mst_atime      : %s" , ctime((time_t*)&attr->mst_atime) ) ;
   md2sPrintf(f," mst_mtime      : %s" , ctime((time_t*)&attr->mst_mtime) ) ;
   md2sPrintf(f," mst_ctime      : %s" , ctime((time_t*)&attr->mst_ctime) ) ;
   md2sPrintf(f," mst_blksize    : %d\n" , attr->mst_blksize ) ;
   md2sPrintf(f," mst_blocks     : %d\n" , attr->mst_blocks ) ;
   return;
}
void md2sPrintHeader( md2File *f , md_head *head )
{
   md2sPrintf(f," ID            : %s\n" , mdStringID(head->ID) ) ;
   md2sPrintf(f," Type          : %s\n" , md2PrintTypes( head->type ) ) ;
   md2sPrintf(f," next ID       : %s\n" , mdStringID(head->nextID) ) ;
   md2sPrintf(f," base ID       : %s\n" , mdStringID(head->baseID) ) ;
   md2sPrintf(f," parent ID     : %s\n" , mdStringID(head->parentID) ) ;
   md2sPrintf(f," creation time : %s"   , ctime((time_t*)&head->cTime) ) ;
   md2sPrintf(f," modif. time   : %s"   , ctime((time_t*)&head->mTime) ) ;
   return;
}
void md2sPrintTagInode( md2File *f , md_tag_inode  *tagInode )
{

   md2sPrintf(f," Name     : %s\n" ,tagInode -> name );
   md2sPrintf(f," Flags    : ");
   if( tagInode -> attr.flags & mdTagInvalid )
      md2sPrintf(f," invalid" ) ;
   if( tagInode -> attr.flags & mdTagInherited )
      md2sPrintf(f," inherited" ) ;
   md2sPrintf(f,"\n" ) ;
   md2sPrintf(f," Size     : %d\n" ,tagInode -> attr.entries );
   md2sPrintUnixAttr(  f , &tagInode -> attr.unixAttr );
   md2sDumpMemory(f , (unsigned char *)(tagInode->data) ,
                 0L , tagInode -> attr.entries ) ;
                 
   return ;

}
void md2sDumpMemory( md2File *out , unsigned char *p , long address , long rest )
{
   unsigned char *m ;
   int i , r ;
   
   for(  ; rest > 0 ;  ){
      md2sPrintf(out," %8.8X ",address) ;
      address += 16 ;
      m = p ;
      r = rest ;
      for( i = 0 ; ( r > 0 ) && ( i < 16 )  ; r-- , i++ , m++)
         md2sPrintf(out," %2.2x", *m ) ;
      for( ; i < 16 ;  i++ )md2sPrintf(out,"   " ) ;
      md2sPrintf(out," *");
      for( i = 0 ; (rest > 0) && ( i < 16 ); rest-- , i++ , p++){
         if(isprint(*p))md2sPrintf(out,"%c", *p ) ;
         else md2sPrintf(out,".");
      }
      for( ; i < 16 ;  i++ )md2sPrintf(out," " ) ;
      md2sPrintf(out,"*\n" ) ;
   }
   return ;
}
