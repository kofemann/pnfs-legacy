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
#include <stdarg.h>
#include <string.h>
#include <time.h>
#if ! defined(darwin) && ! defined(solaris)
#include <memory.h>
#include <malloc.h>
#endif

#include "dbgprint.h"

static FILE * fileStream = { NULL } ;

int dbgprintOpen( char * filename ){
   dbgprintClose() ;
   
   if( filename == NULL ){
      fileStream = stdout ;
   }else{
      fileStream = fopen( filename , "w" ) ;
   }
   return fileStream == NULL ? -1 : 0 ;
}
int dbgprintRewind(){
   if( fileStream != NULL )rewind( fileStream ) ;
   return 0 ;
}
int dbgprintClose(){
  if( ( fileStream != NULL   ) &&
      ( fileStream != stdout ) &&
      ( fileStream != stderr )     )
       fclose( fileStream ) ;
  
  fileStream = NULL ;
  return 0 ;

}
int dbgprintln( char *format , ... ) 
{
 va_list   args;    
    char   string[4*1024] ;
  time_t   now ;
  
   if( fileStream == NULL )return -1 ;
   time( &now ) ;
   sprintf( string , "%8.8X " , now ) ;
   va_start(args, format);
   vsprintf( string+9 , format, args) ;
   va_end(args);

   fputs( string , fileStream ) ;
   fputc( '\n' , fileStream ) ;
   fflush( fileStream ) ;
   return 0 ;

}
