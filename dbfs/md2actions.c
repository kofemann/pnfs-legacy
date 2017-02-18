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
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "md2types.h"
#include "md2fs.h"
#include "md2log.h"

static int      initDone  = { 0 } ;
static md_id_t  delDirID ;
static char    *trash_dir = NULL ;

int md2GetTagContent( MDL * mdl , md_e_dir_entry *obj , 
                      char *name , char *ptr , int size );


int md2ExtHandlerINIT( MDL *mdl )
{

  trash_dir = getenv( "trash" ) ;

  initDone = 1 ;
  return 0 ;
}
int md2ExtHandlerRENAME( MDL *mdl , md_e_dir_entry *from , md_e_dir_entry *to )
{
 int rc1 , rc2  ;
 char dir1[128] , dir2[128] ;
 md_record rec ;
 
  if( ! initDone )md2ExtHandlerINIT( mdl ) ;
  rc1 = md2GetTagContent( mdl , from , "sGroup" , dir1 , sizeof( dir1 ) ) ;
  md2pPrintf(md2pMaxLevel,"%s - %s %s\n",
                db_stringHeader(NULL),mdStringID(from->dirId ) , from->objEntry.name);
  rc2 = md2GetTagContent( mdl , to   , "sGroup" , dir2 , sizeof( dir2 ) ) ;
  md2pPrintf(md2pMaxLevel,"%s - %s %s\n",
                db_stringHeader(NULL),mdStringID(to->dirId ) , to->objEntry.name);
#ifdef MV_DENIED
  if( mdIsEqualID( from -> dirId , to -> dirId ) )return 0 ;
  rec.head.ID = from -> objEntry.ID ;
  if( md2GetRecord( mdl , &rec , 0 ) )return MDEnotAllowed ;
  if( ! mdIsType( rec.head.type , mdtInode )  )return MDEnotAllowed ;
  if( ! mdIsType( rec.head.type , mdtDirectory )  )return 0 ;
  md2pPrintf(md2pMaxLevel,"%s - Sorry, couldn't find database in %s\n",
                db_stringHeader(NULL),"dd");
  return -1 ;
#endif
#ifdef MV_ALLOWED
  return 0 ;
#else
  if( ( rc1 < 0 ) && ( rc2 < 0 ) )return 0 ;
  if( ( rc1 == rc2 ) && ! memcmp( dir1 , dir2 , rc1 ) )return 0 ;
  
  return -1 ;
#endif
}
int md2GetTagContent( MDL *mdl , md_e_dir_entry *obj , char *name , char *ptr , int size )
{
  int     rc ;
  md_record tag ;
  
   if( mdIsNullID( obj -> dirId ) )return -1 ;
   if( rc = md2FindTag( mdl , obj -> dirId , name , &tag  ) )return rc ;
   return md2ReadTagLow( mdl , &tag , ptr , 0, size ) ;

}
int md2ExtHandlerREMOVE( MDL *mdl , md_e_dir_entry *obj )
{
   int          rc , i , trash , level ;
   md_fs_block  f ;
   md_id_t      stub ;
   char         fileID[32] , trashFile[1024] , buffer[1024] , trashTemp[1024] ;
   
  if( ! initDone )md2ExtHandlerINIT( mdl ) ;
  if( trash_dir == NULL )return 0 ; 
  
  /*f( mdIsNullID( delDirID ) )return 0 ;*/
  /*
   * find the stub file
   */
  if( ( strlen(obj->objEntry.name)>2 ) &&
      ( obj->objEntry.name[0] == '.' ) &&
      ( obj->objEntry.name[1] == '(' )    )return 0 ;
  
  if( md2ExtLookupPerm( mdl , obj->dirId ,&(obj->dirPermission),
                        obj->objEntry.name , &stub , NULL ) )return 0 ;
                        
  strcpy( fileID , mdStringID( stub ) ) ;

  level = 0 ;
  {
      sprintf( trashFile, "%s/%d/%s", trash_dir, level, fileID ) ;
      printf( "Creating %s\n", trashFile ) ;
      trash = open( trashFile, O_TRUNC|O_CREAT|O_WRONLY, 0600 );
      if (trash > 0) {
          printf( "Creating %s done\n" , trashFile ) ;
          close( trash );
      }
  }
  
  for( level = 1 ; level < 8 ; level ++ ){
      printf( "Opening level %d\n" , level ) ;
      if( mdFs_Open( mdl , &f , stub , level , "r" ) )return 0 ;
      printf( "Opening level %d done\n" , level ) ;
      if (f.size <= 0 && level != 2) continue; 


      sprintf( trashFile , "%s/%d/%s"  , trash_dir , level , fileID ) ; 
      sprintf( trashTemp , "%s/%d/.%s" , trash_dir , level , fileID ) ; 

      printf( "Opening %s\n" , trashTemp ) ;
      trash = open( trashTemp , O_TRUNC|O_CREAT|O_WRONLY,0600);
      if( trash < 0 )continue ;
      printf( "Opening Done %s\n" , trashTemp ) ;

      while( ( rc = mdFs_Read( &f , buffer ,  1024  ) ) > 0  ){
          printf( "Writing %d\n" , rc ) ;
          write( trash , buffer , rc ) ;
      }

      close( trash );
      rename( trashTemp , trashFile ) ;
      printf( "Renaming %s -> %s\n" , trashTemp , trashFile ) ;
      
  }
  return 0 ;
}
