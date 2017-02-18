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
#ifdef SYSTEM7
#include "unistd.h"
#endif
#include "dbglue.h"
#include "md2types.h"
#include "md2fs.h"

static int ssplit( char *string , char *aargv[] ) ;
int tagmain( MDL * mdl , int argc , char *argv[] );
void usage();
typedef struct k_struct { long high ; long low ; } the_key ;

#define BUFFER_SIZE   (4*1024)
char buffer[BUFFER_SIZE] ;

#define sscanID(str,id)  md2ScanId(str,&(id))

int copytopnfs( MDL * mdl , char *name ,  md_id_t id , int level);
int copyfrompnfs( MDL * mdl , char *name ,  md_id_t id , int level);
main( int argc , char *argv[] )
{
char string[1024] ;
char *aargv[32] ;
int aargc , i ,  rc , mode , z , exitrc ;
long off , size ;
MDL *mdl ;
md_id_t id , resID , dirID ;

mdRecord mdr ;

mdl = NULL ;
exitrc = 0 ;
if( argc > 1 ){
  if( strcmp( argv[1] , "none" ) ){
    mdl = md2OpenReadWrite( argv[1] , &rc , 0 ) ;
    if( ! mdl ){ 
       fprintf( stderr," Can't open %s : %d\n" , argv[1] , rc );
       exit(1);
    }
  }else{
    mdl = NULL ;
  }

}

for(i=0;i<BUFFER_SIZE;i++)buffer[i] = i ;

for( z = 0 ; ; z++ ){
   if( argc > 2 ){
     
     if( z ){
       if( mdl )md2Close( mdl ) ;
       exit(exitrc) ;
     }
     for(i=2;i<argc;i++)aargv[i-2] = argv[i] ;
     aargc = argc -2 ;
     
   }else{
     fgets( string , sizeof(string) , stdin ) ;
     i = strlen(string) ;
     if( ( i > 0 ) && ( string[i-1] == '\n' ) )string[i-1] = '\0' ;
     aargc = ssplit( string , aargv ) ;
     if( aargc < 1 )continue ;
   }
   if( !strcmp( aargv[0] , "say" ) ){
/*                    -------------------          */
      for(i=0;i<aargc;i++)printf( "%d : %s\n" , i , aargv[i] ) ;
   }else if( !strcmp( aargv[0] , "help" ) ){
/*                    -------------------          */
       usage();
   }else if( !strcmp( aargv[0] , "tag" ) ){
       if( ! mdl ){ printf( " DB not open\n" );continue;}
       rc = tagmain( mdl , aargc-1 , aargv+1 ) ;
       if(rc)exitrc = 1 ;
   }else if( !strcmp( aargv[0] , "nextid" ) ){
/*                    -------------------          */
      if( aargc < 1 ){ usage() ; continue ; }
      if( ! mdl ){ printf( " DB not open\n" );continue;}
      rc = md2GetNextId( mdl , &id ) ;
      if(rc){printf(" md2GetNetID : %d\n",rc);continue;}
      printf( " New ID : %s\n" , mdStringID( id ) ) ;
   }else if( !strcmp( aargv[0] , "countentries" ) ){
/*                    ----------------------------          */
      if( aargc < 2 ){ usage() ; continue ; }
      if( ! mdl ){ printf( " DB not open\n" );continue;}
      md2ScanId( aargv[1] , &dirID ) ;
      rc = md2CountDirectoryEntries( mdl , dirID );
      if(rc<0){printf(" md2PrintDirectory : %d\n",rc);continue;}
      printf( " Directory Entries : %d\n" , rc ) ;
   }else if( !strcmp( aargv[0] , "getid" ) ){
/*                    ----------------------------          */ 
      if( aargc < 2 ){ usage() ; continue ; }
      if( ! mdl ){ printf( " DB not open\n" );continue;}
      rc = md2ObjectToId( mdl , dirID , aargv[1] , &dirID );
      if(rc<0){printf(" md2ObjectToId : %d\n",rc);continue;}
	  printf( "%s %s\n",mdStringID(dirID),aargv[1]);
   }else if( !strcmp( aargv[0] , "setconfig" ) ){
/*                    ----------------------------          */ 
      if( aargc < 2 ){ usage() ; continue ; }
      if( ! mdl ){ printf( " DB not open\n" );continue;}
      md2ScanId( aargv[1] , &dirID ) ;
      rc = md2SetRootConfig( mdl , &dirID );
      if(rc<0){printf(" md2SetRootConfig : %d\n",rc);continue;}
   }else if( !strcmp( aargv[0] , "getconfig" ) ){
/*                    ----------------------------          */ 
      if( aargc < 1 ){ usage() ; continue ; }
      if( ! mdl ){ printf( " DB not open\n" );continue;}
      rc = md2GetRootConfig( mdl , &dirID  );
      if(rc<0){printf(" md2GetRootConfig : %d\n",rc);continue;}
      printf( "%s\n",mdStringID(dirID));
   }else if( !strcmp( aargv[0] , "hashcount" ) ){
/*                    ----------------------------          */ 
      if( aargc < 2 ){ usage() ; continue ; }
      if( ! mdl ){ printf( " DB not open\n" );continue;}
      md2ScanId( aargv[1] , &dirID ) ;
      rc = md2PrintHashEntries( mdl , dirID , stdout );
      if(rc<0){printf(" md2PrintHashEntries : %d\n",rc);continue;}
   }else if( !strcmp( aargv[0] , "ls" ) ){
/*                    ----------------------------          */
      if( aargc < 2 ){ usage() ; continue ; }
      if( ! mdl ){ printf( " DB not open\n" );continue;}
      md2ScanId( aargv[1] , &dirID ) ;
      rc = md2PrintDirectory( mdl , dirID , 0,stdout );
      if(rc){printf(" md2PrintDirectory : %d\n",rc);continue;}
   }else if( !strcmp( aargv[0] , "ls-l" ) ){
/*                    ----------------------------          */
      if( aargc < 2 ){ usage() ; continue ; }
      if( ! mdl ){ printf( " DB not open\n" );continue;}
      md2ScanId( aargv[1] , &dirID ) ;
      rc = md2PrintDirectory( mdl , dirID , 1,stdout );
      if(rc){printf(" md2PrintDirectory : %d\n",rc);continue;}
   }else if( !strcmp( aargv[0] , "getdbid" ) ){
/*                    ----------------------------          */
      long dbid ;
      if( ! mdl ){ printf( " DB not open\n" );continue;}
      rc = md2GetDbId( mdl , &dbid );
      if(rc){printf(" md2GetDbId : %d\n",rc);continue;}
      else printf( " dbid %d\n",dbid ) ;
   }else if( !strcmp( aargv[0] , "set" ) ){
/*                    ----------------------------          */
      int level ;
      md_unix attr ;
      
      if( aargc < 5 ){ usage() ; continue ; }
      if( ! mdl ){ printf( " DB not open\n" );continue;}
      sscanID( aargv[1] , dirID ) ;
      sscanf( aargv[2] , "%d" , &level ) ;
      attr.mst_uid = md_no_uid ;
      attr.mst_gid = md_no_gid ;
      attr.mst_mode = md_no_mode ;
      attr.mst_mtime = md_no_time ;
      attr.mst_atime = md_no_time ;
      attr.mst_size  = md_no_size ;
      if( ! strcmp( aargv[3] , "mode" ) ){
         sscanf( aargv[4] , "%ho" , &attr.mst_mode ) ;
      }if( ! strcmp( aargv[3] , "uid" ) ){
         sscanf( aargv[4] , "%d" , &attr.mst_uid ) ;
      }if( ! strcmp( aargv[3] , "gid" ) ){
         sscanf( aargv[4] , "%d" , &attr.mst_gid ) ;
      }if( ! strcmp( aargv[3] , "time" ) ){
         sscanf( aargv[4] , "%x" , &attr.mst_atime ) ;
         sscanf( aargv[4] , "%x" , &attr.mst_mtime ) ;
      }
      printf( " %s-%d : mode %ho , uid %d , gid %d , time %x\n" ,
              mdStringID( dirID ) , level ,
              attr.mst_mode,attr.mst_uid,attr.mst_gid,attr.mst_mtime);
      rc = md2ModUnixAttr( mdl , dirID ,  level , &attr) ;
      if(rc){printf(" md2ModUnixAttr : %d\n",rc);continue;}
   }else if( !strcmp( aargv[0] , "diropen" ) ){
/*                    ----------------------------          */
      md_cookie cookie ;
      if( aargc < 3 ){ usage() ; continue ; } 
      if( ! mdl ){ printf( " DB not open\n" );continue;}
      sscanID( aargv[1] , dirID ) ;
      sscanf( aargv[2] , "%x" , &cookie ) ;
      rc=md2DirOpen( mdl , dirID , &cookie ) ;
      if(rc){printf(" md2DirOpen : %d\n",rc);continue;}
      printf( " Cookie %x\n" , cookie ) ;
   }else if( !strcmp( aargv[0] , "dirnext" ) ){
/*                    ----------------------------          */
      md_cookie cookie ;
      md_dir_item item ;
      if( aargc < 2 ){ usage() ; continue ; }
      if( ! mdl ){ printf( " DB not open\n" );continue;}
      sscanf( aargv[1] , "%x" , &cookie ) ;
      rc=md2DirNext( mdl ,  &cookie , &item ) ;
      if(rc){printf(" md2DirOpen : %d\n",rc);continue;}
      printf( " File   %s  %s\n" , mdStringID( item.ID ) , item.name  ) ;
      printf( " Cookie %x\n" , cookie ) ;
   }else if( !strcmp( aargv[0] , "rmdir" ) ){
/*                    ----------------------------          */
      if( aargc < 3 ){ usage() ; continue ; }
      if( ! mdl ){ printf( " DB not open\n" );continue;}
      sscanID( aargv[1] , dirID ) ;
      rc=md2RemoveDirectory( mdl , dirID , aargv[2] ) ;
      if(rc){printf(" md2RemoveDirectory : %d\n",rc);continue;}
   }else if( !strcmp( aargv[0] , "next" ) ){
/*                    ----------------------------          */
      md_dir_cookie cookie ;
      md_dir_item   item ;
      rc = md2_dir_next( mdl , &item , &cookie ) ;
      if(rc){printf(" md2_dir_next : %d\n",rc);continue;}
      printf( " %s %s\n",mdStringID(item.ID),item.name);
      printf( " Cookie : %d %d\n",cookie.hash,cookie.position);
   }else if( !strcmp( aargv[0] , "directory" ) ){
/*                    ----------------------------          */
      md_dir_cookie cookie ;
      if( aargc < 4 ){ usage() ; continue ; }
      if( ! mdl ){ printf( " DB not open\n" );continue;}
      sscanID( aargv[1] , dirID ) ;
      sscanf( aargv[2] , "%d" , &cookie.hash ) ; 
      sscanf( aargv[3] , "%d" , &cookie.position ) ; 

      rc=md2_dir_open( mdl , dirID , &cookie ) ;
      if(rc){printf(" md2_dir_open : %d\n",rc);continue;}
      printf( " Hash    : %d\n" , cookie.hash ) ;
      printf( " Position: %d\n" , cookie.position ) ;
      printf( " Eof     : %d\n" , mdl->dir.eof ) ;
      printf( " Valid   : %d\n" , mdl->dir.valid ) ;
      /* (void)md2_dir_close( mdl ) ;*/
   }else if( !strcmp( aargv[0] , "permission" ) ){
/*                    ----------------------------          */
      md_permission perm ;
      if( aargc < 4 ){ usage() ; continue ; }
      if( ! mdl ){ printf( " DB not open\n" );continue;}
      sscanID( aargv[1] , dirID ) ;
	  memset( (char *)&perm , 0 , sizeof( perm ) ) ;
	  sscanf( aargv[3] , "%x" , &perm.low ) ;
      rc=md2ModPermission( mdl , dirID , aargv[2] , perm) ;
      if(rc){printf(" md2ModPermission : %d\n",rc);continue;}
   }else if( !strcmp( aargv[0] , "rm" ) ){
/*                    ----------------------------          */
      md_permission perm ;
      memset( (char*)&perm , 0 , sizeof( perm ) ) ;
      if( aargc < 3 ){ usage() ; continue ; }
      if( ! mdl ){ printf( " DB not open\n" );continue;}
      sscanID( aargv[1] , dirID ) ;
      rc=md2RemoveFile( mdl , dirID , perm , aargv[2] ) ;
      if(rc){printf(" md2RemoveDirectory : %d\n",rc);continue;}
   }else if( !strcmp( aargv[0] , "read" ) ){
/*                    ----------------------------          */
      if( aargc < 4 ){ usage() ; continue ; }
      if( ! mdl ){ printf( " DB not open\n" );continue;}
      sscanID( aargv[1] , dirID ) ;
      sscanf( aargv[2] , "%x" , &off ) ; 
      sscanf( aargv[3] , "%x" , &size ) ;
      if( aargc > 4 ){
          sscanf( aargv[4] , "%d" , &mode ) ;
      }else{
          mode = 0 ;
      }
      if(size>BUFFER_SIZE){ usage() ; continue ; }
      rc = md2ReadData( mdl , dirID , mode , buffer , off , size );
      if(rc<0){printf(" md2ReadData : %d\n",rc);continue;}
      printf( " Bytes Read : %d\n" , rc ) ;
      md2DumpMemory( stdout , buffer , off , rc );
   }else if( !strcmp( aargv[0] , "write" ) ){
/*                    ----------------------------          */
      if( aargc < 4 ){ usage() ; continue ; }
      if( ! mdl ){ printf( " DB not open\n" );continue;}
      sscanID( aargv[1] , dirID ) ;
      sscanf( aargv[2] , "%x" , &off ) ; 
      sscanf( aargv[3] , "%x" , &size ) ;
      if( aargc > 4 ){
          sscanf( aargv[4] , "%d" , &mode ) ;
      }else{
          mode = 0 ;
      }
      if(size>BUFFER_SIZE){ usage() ; continue ; }
      for(i=0;i<size;i++)buffer[i] = i ;
      rc = md2WriteData( mdl , dirID , mode , buffer , off , size );
      if(rc){printf(" md2WriteData : %d\n",rc);continue;}
   }else if( !strcmp( aargv[0] , "lookup" ) ){
/*                    ----------------------------          */
      if( aargc < 3 ){ usage() ; continue ; }
      if( ! mdl ){ printf( " DB not open\n" );continue;}
      sscanID( aargv[1] , dirID ) ;
      rc = md2Lookup( mdl , dirID , aargv[2] , &resID );
      if(rc){printf(" md2Lookup : %d\n",rc);continue;}
      printf( "%s %s\n" , mdStringID( resID ) , aargv[2]) ;
      
   }else if( !strcmp( aargv[0] , "hostinfo" ) ){
/*                    ----------------------------          */
      md_fs_format f ;
	  md_host_info  hi ;
      if( aargc < 2 ){ usage() ; continue ; }
      if( ! mdl ){ printf( " DB not open\n" );continue;}
      sscanID( aargv[1] , dirID ) ;
	  rc = mdFs_FOpen( mdl , &f , dirID , 0 , "r" );
      if(rc){printf(" mdFs_FOpen : %d\n",rc);continue;}
	  rc=md2_GetHostInfo( &f , &hi );
      if(rc){printf(" md2_GetHostInfo : %d\n",rc);continue;}
	  rc=md2_PrintHostInfo( stdout , &hi );

   }else if( !strcmp( aargv[0] , "topnfs" ) ){
/*                    ----------------------------          */
      int level ;
      if( aargc < 3 ){ usage() ; continue ; /* topnfs <file> <id> [<level>]*/}
      if( ! mdl ){ printf( " DB not open\n" );continue;}
      sscanID( aargv[2] , dirID ) ;
      if( aargc > 3 )sscanf( aargv[3] , "%d" , &level ) ;
      else level = 0 ;
      rc = copytopnfs( mdl , aargv[1]  ,  dirID , level );
      if(rc){printf(" copytopnfs : %d\n",rc);continue;}
   }else if( !strcmp( aargv[0] , "frompnfs" ) ){
/*                    ----------------------------          */
      int level ;
      if( aargc < 3 ){ usage() ; continue ; /* frompnfs <file> <id> [<level>]*/}
      if( ! mdl ){ printf( " DB not open\n" );continue;}
      sscanID( aargv[2] , dirID ) ;
      if( aargc > 3 )sscanf( aargv[3] , "%d" , &level ) ;
      else level = 0 ;
      rc = copyfrompnfs( mdl , aargv[1]  ,  dirID , level);
      if(rc){printf(" copyfrompnfs : %d\n",rc);continue;}
      
   }else if( !strcmp( aargv[0] , "deletedirectory" ) ){
/*                    ----------------------------          */
      if( aargc < 2 ){ usage() ; continue ; }
      if( ! mdl ){ printf( " DB not open\n" );continue;}
      sscanID( aargv[1] , dirID ) ;
      rc = md2DeleteDirectory( mdl , dirID );
      if(rc){printf(" md2DeleteDirectory : %d\n",rc);continue;}
      
   }else if( !strcmp( aargv[0] , "removefromdirectory" ) ){
/*                    ----------------------------          */
      if( aargc < 3 ){ usage() ; continue ; }
      if( ! mdl ){ printf( " DB not open\n" );continue;}
      sscanID( aargv[1] , dirID ) ;
      rc = md2RemoveFromDirectory( mdl , dirID , aargv[2] , &resID );
      if(rc){printf(" md2RemoveFromDirectory : %d\n",rc);continue;}
      printf( " Removed Directory ID : %s\n" , mdStringID( resID ) ) ;

   }else if( !strcmp( aargv[0] , "propagate" ) ){
      if( aargc < 2 ){ usage() ; continue ; }
      md2ScanId( aargv[1] , &dirID ) ;
      rc = md2TagPropagateLow( mdl , dirID );
      if(rc){printf(" md2TagPropagateLow : %d\n",rc);continue;}
   }else if( !strcmp( aargv[0] , "scantest" ) ){
      if( aargc < 2 ){ usage() ; continue ; }
      md2ScanId( aargv[1] , &dirID ) ;
      printf( "%s\n" , mdStringID( dirID )) ;
   }else if( !strcmp( aargv[0] , "mkdir" ) ){
/*                    ----------------------------          */
      if( aargc < 3 ){ usage() ; continue ; }
      if( ! mdl ){ printf( " DB not open\n" );continue;}
      md2ScanId( aargv[1] , &dirID ) ;
      rc = md2MakeDirectory( mdl , dirID , aargv[2] , &resID );
      if(rc)exitrc = 1 ;
      if(rc){fprintf(stderr," md2MakeDirectory : %d errno : %d\n",rc,errno);continue;}
      printf( "%s %s\n" , mdStringID( resID ) , aargv[2]) ;
   }else if( !strcmp( aargv[0] , "mkfile" ) ){
/*                    ----------------------------          */
      if( aargc < 3 ){ usage() ; continue ; }
      if( ! mdl ){ printf( " DB not open\n" );continue;}
      md2ScanId( aargv[1] , &dirID ) ;
      rc = md2MakeFile( mdl , dirID , aargv[2] , &resID );
      if(rc){printf(" md2MakeFile : %d\n",rc);continue;}
      printf( "%s %s\n" , mdStringID( resID ),  aargv[2]) ;
   }else if( !strcmp( aargv[0] , "addtodirectory" ) ){
/*                    ----------------------------          */
      if( aargc < 4 ){ usage() ; continue ; }
      if( ! mdl ){ printf( " DB not open\n" );continue;}
      md2ScanId( aargv[1] , &dirID ) ;
      md2ScanId( aargv[3] , &id ) ;
      rc = md2AddToDirectoryOnly( mdl , dirID , aargv[2] , id );
      if(rc){printf(" md2AddToDirectoryOnly : %d\n",rc);continue;}
      printf( "%s %s\n" , mdStringID( id ),  aargv[2]) ;
   }else if( !strcmp( aargv[0] , "scandirectory" ) ){
/*                    ----------------------------          */
      if( aargc < 2 ){ usage() ; continue ; }
      if( ! mdl ){ printf( " DB not open\n" );continue;}
      md2ScanId( aargv[1] , &dirID ) ;
      rc = mdDirectoryScan( mdl , dirID );
      if(rc){printf(" mdDirectoryScan : %d\n",rc);continue;}
   }else if( !strcmp( aargv[0] , "newdirectory" ) ){
/*                    ----------------------------          */
      if( aargc < 2 ){ usage() ; continue ; }
      if( ! mdl ){ printf( " DB not open\n" );continue;}
      md2ScanId( aargv[1] , &id ) ;
      rc = md2MakeNewDirectory( mdl , id , &resID );
      if(rc){printf(" md2MakeNewDirectory : %d\n",rc);continue;}
      printf( " New Directory ID : %s\n" , mdStringID( resID ) ) ;
   }else if( !strcmp( aargv[0] , "show" ) ){
/*                    -------------------          */
      if( aargc < 2 ){ usage() ; continue ; }
      if( ! mdl ){ printf( " DB not open\n" );continue;}
      md2ScanId( aargv[1] , &mdr.head.ID ) ;
      rc = md2GetRecord( mdl , &mdr , 0 ) ;
      if(rc){printf(" md2GetRecord : %d\n",rc);continue;}
      md2PrintRecord( stdout , &mdr ) ;

   }else if( !strcmp( aargv[0] , "open" ) ){
/*                    -------------------          */
      if( aargc < 2 ){ usage() ; continue ; }
      if( mdl ){ printf( " DB still open\n" );continue;}
      mdl = md2OpenReadWrite( aargv[1] , &rc, 0 ) ;
      if( ! mdl ){ printf( " Can't open %s : %d(%d)\n" , aargv[1] , rc ,errno);continue;}
   }else if( !strcmp( aargv[0] , "create" ) ){
/*                    -------------------          */
      if( aargc < 3 ){ usage() ; continue ; }
      if( mdl ){ printf( " DB still open\n" );continue;}
      sscanf( aargv[2] , "%d" , &rc ) ;
      rc = md2Create( aargv[1] , rc  ) ;
      if(rc)exitrc = 1 ;
      if( rc ){ printf( " Can't create %s : %d\n" , aargv[1] , rc );continue;}
   }else if( !strcmp( aargv[0] , "close" ) ){
/*                    -------------------          */
      if( ! mdl ){ printf( " DB not open\n" );continue;}
      md2Close( mdl ) ;
      mdl = NULL ;
   }else if(!strcmp( aargv[0] , "exit" ) ){
/*                    -------------------          */
     if( mdl )md2Close( mdl ) ;
     mdl = NULL ;
     break ;
   }else{
      printf( " Unknown %s\n" , aargv[0] ) ;
      usage() ;
   }

  }
  if( mdl )md2Close( mdl ) ;
  exit(0);

}
void usage(){
printf( " Commands :\n");
printf( "    help\n");
printf( "    open   <dataBase>\n");
printf( "    create <dataBase> <dbID>\n");
printf( "    close\n");
printf( "    getdbid\n");
printf( "    exit\n");
printf( "    mkdir    <dirID> <name>\n");
printf( "    permission  <dirID> <name> <perm>\n");
printf( "    lookup   <dirID> <name>\n");
printf( "    ls       <dirID>\n");
printf( "    ls-l     <dirID>\n");
printf( "    show     <id>\n");
printf( "    getid    <absolutePath>\n");
printf( "    topnfs   <name> <id>\n");
printf( "    frompnfs <name> <id>\n");
printf( "    hostinfo <id>\n");
printf( "    getconfig \n");
printf( "    setconfig <id>\n");
printf( "    -----------------------------------------\n");
printf( "    nextid\n");
printf( "    addtodirectory      <dirID> <name> <id>\n");
printf( "    newdirectory        <dirID>\n");
printf( "    removefromdirectory <dirID> <name>\n");
printf( "    deletedirectory     <dirID>\n");
printf( "    countentries        <dirID>\n");
printf( "    scandirectory       <dirID>\n");
printf( "    propagate           <tagID>\n");
}

int copytopnfs( MDL * mdl , char *name ,  md_id_t id , int level )
{
  int rc , fh;
  md_fs_block p ;
  
  if( rc = mdFs_Open( mdl , &p , id , level , "w" )  )return rc ;
  if( ( fh = open( name , O_RDONLY ) ) < 0 ){
     fprintf(stderr," Can't open %s : %d\n" , name , errno ) ;
     return -200 ;
  }
  while( ( rc = read( fh , buffer , 1024 ) ) > 0 ){	
    if( ( rc = mdFs_Write( &p , buffer , rc ) ) < 0 )break ;  
  }
  if(rc)fprintf(stderr," Something returned with %d\n",rc );
  close(fh);
  return 0 ;
}
int copyfrompnfs( MDL * mdl , char *name ,  md_id_t id , int level )
{
  int rc , fh;
  md_fs_block p ;
  
  if( rc = mdFs_Open( mdl , &p , id , level , "r" )  )return rc ;
#ifdef SYSTEM7
  if( ( fh = open( name , O_WRONLY | O_CREAT | O_TRUNC  ) ) < 0 ){
#else
  if( ( fh = open( name , O_WRONLY | O_CREAT | O_TRUNC  , 0600 ) ) < 0 ){
#endif
     fprintf(stderr," Can't open %s : %d\n" , name , errno ) ;
     return -200 ;
  }
  while( ( rc = mdFs_Read( &p , buffer , 1024 ) ) > 0 ){	
    if( ( rc = write( fh , buffer , rc ) ) < 0 )break ;  
  }

  if(rc)fprintf(stderr," Something returned with %d\n",rc );
  close(fh);
  return 0 ;
}

#define X_UNKNOWN   (0)
#define X_BLANK     (1)
#define X_CHAR      (2)

static int ssplit( char *string , char *aargv[] )
{
  int stat , count ;
  char *x ;
  
  stat  = X_UNKNOWN ;
  count = 0 ;
  for( x = string ; *x != '\0' ; x++ ){
    switch( stat ){
      case X_UNKNOWN :
         if( ( *x == ' ' ) || ( *x == '\t' ) ){
             stat = X_BLANK ;
         }else{
             stat = X_CHAR ;
             aargv[count++] = x ;
         }
      break ;
      case X_BLANK :
         if( ( *x == ' ' ) || ( *x == '\t' ) ){
             stat = X_BLANK ;
         }else{
             stat = X_CHAR ;
             aargv[count++] = x ;
         }
      break ;
      case X_CHAR :
         if( ( *x == ' ' ) || ( *x == '\t' ) ){
             stat = X_BLANK ;
             *x = '\0' ;
         }else{
             stat = X_CHAR ;
         }
      break ;
    
    }
  }
  return count ;

}
