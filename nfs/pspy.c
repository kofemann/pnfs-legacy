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
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include "shmcom.h"
#include "md2log.h"

#define spy_head    "Spy"

main( int argc , char *argv[] )
{
  char      *shmKey , *pnfsSetup , *logFile;
  key_t     key ;
  int       rc ;
  char      *error ;
  
   if( argc < 2  ){
      pnfsSetup = "/usr/etc/pnfsSetup" ;
   }else{
      pnfsSetup = argv[1] ;
   }

  if( spy_addToEnv( pnfsSetup ) ){
     fprintf(stderr," Sorry, couldn't add %s to environment\n",pnfsSetup);
     exit(2);
  }

  if( ( logFile = getenv( "pspyLog" ) ) == NULL ){
     fprintf(stderr," Sorry, couldn't find pspyLog in %s\n",pnfsSetup);
     md2pOpen( "/dev/null" , 0 ) ;
     md2pNewLevel( 0 ) ; 
  }else{
     md2pOpen( logFile , 0 ) ;
     md2pNewLevel( md2pMODINFO ) ; /* only interested in modifications */
     md2pNewLevel( md2pMOREINFO ) ; 
  }
  md2pPrintf(md2pMaxLevel,"%s - start %s\n", spy_head() , "pspy" );

  if( ( shmKey = getenv( "shmkey" ) ) == NULL )shmKey="1122" ;

  sscanf( shmKey , "%x" , &key ) ;
  if( ! ( scl   = sclClientOpen(  key , 8*1024 , &rc ) ) ){
      error = sclError( "sclClientOpen" , rc ) ;
      md2pPrintf(md2pMaxLevel,"%s - sclClientOpen : %s\n", spy_head , error );
      exit(2) ;
  }

}
int spy_addToEnv( char *name )
{
  char string[1024] , *env ;
  FILE *f ;
  int len ;
  
   if( (  ( f = fopen( name , "r" ) ) == NULL ) ){
     fprintf(stderr," Sorry, %s not found : %d\n" , name , errno ) ;
     return -1 ;
   }else{
      while( fgets( string , 1024 , f ) != NULL ){
         if( ( *string == '#' ) ||
             ( ( len = strlen( string ) ) < 2 ) )continue ;
         string[len-1] = '\0' ;
         if( ( env = malloc( len + 16 ) ) == NULL ){
            fprintf(stderr," Malloc failed(exit)\n") ;
            exit(1) ;
         }
         strcpy( env , string ) ;
         putenv( env ) ;
      }
   }
   fclose(f) ;
   return 0;
}
