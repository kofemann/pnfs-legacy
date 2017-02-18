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
#include "md2log.h"

static md2pBase *md2pStdout = { NULL } ;

#ifdef MAIN_TEST
main()
{
/*
  md2WGC * def = md2rpWorldGateClass( 0x83A901de , 30302 ); 
  if( ! def ){ printf( "cant open worldgaten\n"); exit(1);}
  
*/    
md2pOpen( "waste" , 0 );
md2pPrintf( md2pW , " this is an ordanary line %d\n" , 555 ) ;
md2pClose();

}
#endif
int md2p_open()
{
   if( md2pStdout -> out = fopen( md2pStdout -> file , "a" )  ){
#ifdef solaris
      setvbuf(md2pStdout -> out,NULL,_IOLBF,0 );
#else
      setlinebuf(md2pStdout -> out);
#endif
      return 0 ;
   }
   return -1 ;
}
int md2pOpen( char *filename , long flags )
{
   if( md2pStdout )md2pClose() ;

   md2pStdout = (md2pBase*)malloc(sizeof(md2pBase)) ;
   if( ! md2pStdout )return -1 ;
   memset( (void*)md2pStdout , 0 , sizeof(md2pBase) ) ;  
   strcpy( md2pStdout -> file , filename );
   return 0 ;
   
}
int md2pPrintf( int prio , char *format , ... ) 
{
 va_list   args;
    char   string[4*1024] ;

   va_start(args, format);
   vsprintf( string , format, args) ;
   va_end(args);

   return md2pPutf( prio , string ) ;

}
int md2pNewLevel( int level )
{
   if( ! md2pStdout )return -1 ;
   if( level < 0 ){
     md2pStdout -> level ++ ; 
   }else{
     md2pStdout -> level = level ; 
   }
   if( md2pStdout -> level > md2pMaxLevel ){
     md2pStdout -> level = 0 ; 
   }
   return md2pStdout -> level ;
}
#define _MAX_LINE_LEN  (1024)
#define LINE_BUFFER_ON
#ifdef LINE_BUFFER_ON
int md2pPutf( int prio , char *string ) 
{
  static char line[_MAX_LINE_LEN] ;
  static int counter = { 0 } ;
  static int rest = { _MAX_LINE_LEN } ;
  int f , trigger ;
  
   if( ! md2pStdout )return -1 ;
   if( md2pLevelOf(prio) < md2pStdout -> level )return -2 ;
   
   if( ( strlen( string ) + 1 ) >= rest ){
      if( md2p_open() )return -3 ;
      fputs( line , md2pStdout -> out ) ;
      if(md2worldOk(prio))md2sendWorld( line ) ;
      fclose( md2pStdout -> out ) ;
      rest    = _MAX_LINE_LEN ;
      counter = 0 ;
   }
   trigger = 0 ;
   for( f=0 ; ( line[counter] = string[f] ) !=  '\0' ; f++, counter++ , rest-- )
      if( line[counter] == '\n' )trigger = 1 ;
      
   if( ! trigger ) return 0 ;
   
   if( md2p_open() )return -3 ;
   fputs( line , md2pStdout -> out ) ;
   if(md2worldOk(prio))md2sendWorld( line ) ;
   fclose( md2pStdout -> out ) ;
   rest    = _MAX_LINE_LEN ;
   counter = 0 ;
   
   md2pStdout -> outCounter ++ ;
   
   return 0 ;
}
#else
int md2pPutf( int prio , char *string ) 
{
   if( ! md2pStdout )return -1 ;
   if( md2pLevelOf(prio) < md2pStdout -> level )return -2 ;
   
   if( md2p_open() )return -3 ;
   fputs( string , md2pStdout -> out ) ;
   if(md2worldOk(prio))md2sendWorld( line ) ;
   fclose( md2pStdout -> out ) ;
   
   md2pStdout -> outCounter ++ ;
   
   return 0 ;
}
#endif
int md2pClose()
{
   if( ! md2pStdout )return -1 ;
   /* . do whatever is neccessary to close whatever is open */

   free( (void*)md2pStdout ) ;
   md2pStdout = NULL ;
   return 0 ;
}
#ifdef WORLD_LOG
md2WGC * md2rpWorldGateClass( unsigned long ip ,int port )
{
   md2WGC * world ;
   
   if( ( world = (md2WGC *)malloc(sizeof(md2WGC))) == NULL )return NULL ;
   
   memset( (char*)world , 0 , sizeof(md2WGC) ) ;
   
   if( (world->fd = socket( AF_INET , SOCK_DGRAM , 0 ))<0)goto problem ;
/* 0x83A901de  30302 */
   world->defLogIP.sin_port        = htons( (short)port ) ;
   world->defLogIP.sin_addr.s_addr = htonl( ip ) ;
   world->defLogIP.sin_family      = AF_INET ; 
   
   (void)md2rpSendMessageLow( world , "urgent:Hallo" , 0 ) ;
   
   return world ;
   
problem:
   free( ( char *)world ) ;
   return NULL ;   

}
#else
md2WGC * md2rpWorldGateClass( unsigned long ip ,int port )
{
    return NULL ;
}
#endif
int md2rpSendMessageLow( md2WGC *world , char *message , int len )
{
  int rc ;
  
  if( ( world == NULL ) || ( message == NULL ) )return 0 ;
  
  rc = sendto( world->fd , message , len ? len : strlen(message)+1 , 0 ,
              (struct sockaddr *)&(world->defLogIP),sizeof(world->defLogIP));
              
  return rc ;
}
/* static stuff */
static md2WGC  *  defWorldConnection = { NULL } ;
int md2openWorld()
{
   if( defWorldConnection == NULL ){
      defWorldConnection = md2rpWorldGateClass( 0x83A901de , 30302 );   
   }
   return defWorldConnection ? 0 : -1 ;
}
int md2sendWorld( char * message )
{
   md2openWorld() ;
   return md2rpSendMessageLow( defWorldConnection , message , 0 ) ;
}
