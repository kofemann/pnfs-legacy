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
#include <sys/param.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <dirent.h>
#include <strings.h>
#include <errno.h>
#include <syslog.h>

struct {

 int items[4] ;

} x ;
main( int argc , char *argv[] )
{
  char *path ;
  DIR	*dirp;
  struct dirent *dp;
  struct stat   s;
  
   if( argc < 2 )goto Usage ;

   
   if( ! strcmp( argv[1] , "dir" ) ){
      if( argc != 3 )goto Usage ;
      path = argv[2] ;
      if( (dirp = opendir(path) ) == NULL ){
        fprintf(stderr," Can't opendir %s\n" , path ) ;
        exit(2) ; 
      }
      while( ( dp = readdir(dirp) ) != NULL ){
          printf( "fn=%s ; fnl=%d ; fno=%d ; rl=%d ; off=0x%x\n",
                   dp->d_name,dp->d_namlen,dp->d_fileno,dp->d_reclen,
                   dp->d_off);
      }
      closedir(dirp);
      exit(0);

   }else if( ! strcmp( argv[1] , "waste" ) ){
        printf( " Sizeof(struct stat) = %d\n" , sizeof(struct stat ) ) ;
        printf( " Sizeof %d\n" , sizeof( x.items ) ) ;
        printf( " Sizeof %d\n" , sizeof( x.items )/sizeof(x.items[0]) ) ;
   }else if( ! strcmp( argv[1] , "stat" ) ){
      if( argc != 3 )goto Usage ;
      path = argv[2] ;
      if( lstat( path , &s ) < 0 ){
        fprintf(stderr," Can't lstat %s\n" , path ) ;
        exit(3) ; 
      }
      printf( " st_dev        : %d\n" , s.st_dev ) ;
      printf( " st_ino        : %d\n" , s.st_ino ) ;
      printf( " st_mode       : %o\n" , s.st_mode ) ;
      printf( " st_nlink      : %d\n" , s.st_nlink ) ;
      printf( " st_uid        : %d\n" , s.st_uid ) ;
      printf( " st_gid        : %d\n" , s.st_gid ) ;
      printf( " st_rdev       : %d\n" , s.st_rdev ) ;
      printf( " st_size       : %d\n" , s.st_size ) ;
      printf( " st_atime      : %s" , ctime(&s.st_atime) ) ;
      printf( " st_mtime      : %s" , ctime(&s.st_mtime) ) ;
      printf( " st_ctime      : %s" , ctime(&s.st_ctime) ) ;
      printf( " st_blksize    : %d\n" , s.st_blksize ) ;
      printf( " st_blocks     : %d\n" , s.st_blocks ) ;
      exit(0);
   }else{
     fprintf(stderr," What : %s\n" , argv[1] ) ;
     goto Usage ;
   }
   exit(0) ;
   
 Usage :  
      fprintf(stderr," USAGE : %s dir <entry>\n" , argv[0] ) ;
      fprintf(stderr," USAGE : %s stat <entry>\n" , argv[0] ) ;
      exit(1);

}
