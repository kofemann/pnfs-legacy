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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <signal.h>
#include <errno.h>
#include <sys/param.h>

#define MOVERSIZE  (1024)
static char mover[MOVERSIZE] ;
static FILE * o = stderr ;
static statusSignal = 0 ;
int scanDir( char *fromdir , char * todir  ){
 DIR           * dir ;
 struct dirent * ent ;
 struct stat     info ;
 char  from_file[1024]  , to_file[1024] , magic_file[1024] ;
 int   from_fd , to_fd , rc , wc ;
 
 if( ( dir = opendir( fromdir ) ) == NULL )return -2 ;

    while( ( ent = readdir( dir ) ) != NULL ){
      if( ( ! strcmp(ent->d_name,"..") ) ||
          ( ! strcmp(ent->d_name,".") )     )continue ;
          
      sprintf( from_file , "%s/%s"                  , fromdir , ent->d_name ) ;
      sprintf(   to_file , "%s/%s"                  , todir , ent->d_name ) ;
      sprintf( magic_file ,"%s/.(fset)(%s)(io)(on)" , todir , ent->d_name ) ;
      if( lstat( from_file , &info ) ){
        if(o)fprintf(o," Can't stat file %s\n" , from_file ) ;
        continue ;
      }
   
      if( ! S_ISREG( info.st_mode ) ){
        if(o)fprintf(o," Not a file : %s\n" , from_file ) ;
        continue ;
      }
      
      if( ( time(NULL) - info.st_mtime ) < 10  ){
        if(o)fprintf(o," too young : %s\n" , from_file ) ;
        continue ;
      }
      if( info.st_size == 0  ){
        if(o)fprintf(o," Zero size file : %s\n" , from_file ) ;
        unlink( from_file ) ;
        continue ;
      }
      if( ( from_fd = open( from_file , O_RDONLY ) ) < 0 ){
        if(o)fprintf(o,"touch %s failed\n" , from_file ) ;
        continue ;   
      }
      if( ( to_fd = open( to_file , O_WRONLY | O_CREAT | O_TRUNC , 0600 ) ) < 0 ){
         if(o)fprintf(o,"create %s failed\n" , to_file ) ;
         close( from_fd );
         continue ;
      }
      close( to_fd );
      if( ( to_fd = open( magic_file , O_WRONLY | O_CREAT , 0600 ) ) < 0 ){
         if(o)fprintf(o,"touch %s failed\n" , magic_file ) ;
         close( from_fd );
         unlink( to_file ) ;
         continue ;
      }
      close( to_fd );
      if( ( to_fd = open( to_file , O_WRONLY ) ) < 0 ){
         if(o)fprintf(o,"open %s failed\n" , to_file ) ;
         close( from_fd );
         unlink( to_file ) ;
         continue ;
      }
      while( ( ( rc = read( from_fd , mover , MOVERSIZE ) ) > 0 ) &&
             ( ( wc = write( to_fd , mover , rc ) ) == rc ) ) ;
             
      close( from_fd ) ;
      close( to_fd ) ;
      if( rc == 0 ){
        unlink( from_file ) ;
      }else{
         if(o){
           fprintf(o,"io %s-> %s failed\n" , to_file , from_file ) ;
           fprintf(o," rc : %d ; wc : %d ; errno : %d\n" , rc , wc , errno ) ;
         }
         unlink( to_file ) ;
      }
      if( statusSignal )break ;  

   }
 closedir( dir ) ;

 return 0 ;
}
static void mover_intr_handler( int sig )
{
  statusSignal = sig ;
  return ; 
}


main( int argc , char *argv[] ){
 char fromdir[1024] ,  todir[1024]  ;
 int i ;
 struct sigaction newSigAction ;
 
 if( argc != 3 ){
     printf( " USAGE : %s <fromdir> <todir>\n",argv[0] );
     exit(1);
 }
#ifndef STAY
 if( fork() != 0 )exit(0) ;
 setsid() ;
 for( i = 0 ; i < NOFILE ; i++ )close(i) ;
 o = 0 ;
#endif
    
     newSigAction.sa_handler = mover_intr_handler ;
     newSigAction.sa_flags   = 0 ;
     sigemptyset(&newSigAction.sa_mask);
     sigaction( SIGTERM , &newSigAction , NULL ) ;
     sigaction( SIGINT  , &newSigAction , NULL ) ;

 sprintf( fromdir , "%s/1" , argv[1] );
 sprintf( todir   , "%s/1" , argv[2] );
 
while(1){ 
   scanDir( fromdir , todir ) ; 
   if( statusSignal )break ;
   sleep(5) ;
   if( statusSignal )break ;
}
 

 exit(0); 
}
  
