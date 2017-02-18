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
#include <unistd.h>
#include <string.h>
#ifdef onIRIX
#include <bstring.h>
#endif
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <termio.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>
#

#define bufLen    (8*1024)


main( int argc , char *argv[] )
/*---------------------------------------*/
{
  int f , ret , addrLen  ;
  struct sockaddr_in   from , me ;
  char buffer[bufLen]  ;
  FILE *stream ;

   if( ( f = socket( AF_INET , SOCK_DGRAM , 0 ) ) < 0 ){
     fprintf( stderr , " Sorry, can't open socket %s\n",errno ) ;
     exit(1) ;
   }
   me.sin_port        = 30302 ;
   me.sin_addr.s_addr = INADDR_ANY ;
   me.sin_family      = AF_INET ; 

   if( ( bind( f , ( struct sockaddr *) &me , sizeof( me ) ) ) < 0 ){
     fprintf( stderr , " Sorry, can't bind socket: %d\n" , errno ) ;
     exit(1) ;
   }

#ifdef  background  
  if( fork() )exit(0);  
#ifdef  irix
  setpgrp() ;
#else
{ int fd ;
  setpgrp(0,getpid()) ;
  if((fd=open("/dev/tty",O_RDWR))>=0){
    ioctl(fd,TIOCNOTTY,NULL);
    close(fd) ;
  }
}
#endif
#endif
   



 while(1){
   time_t now ;

   addrLen = sizeof( from ) ;
   ret = recvfrom( f , buffer , bufLen-1 , 0 ,
                   ( struct sockaddr *)&from , &addrLen ) ;
   if( stream = fopen( "/tmp/world.log" , "a" ) ){ 
     time( &now );
     
     fprintf( stream ,
              " Request From host %u.%u.%u.%u port %d size %d at %s",
              from.sin_addr.s_addr >> 24,
            ( from.sin_addr.s_addr >> 16 ) & 0xFF ,
            ( from.sin_addr.s_addr >>  8 ) & 0xFF ,
            ( from.sin_addr.s_addr       ) & 0xFF ,
            from.sin_port , ret , ctime(&now) ) ;
     if( ret > 0 )buffer[ret] = '\0' ;
     fprintf(stream,"%s\n" , buffer ) ;
     fclose( stream );    
   }
 }  
} 
