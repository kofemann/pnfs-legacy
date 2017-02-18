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

#include "md2types.h"
#include "md2fs.h"


char *mdFs_Ffgets(char *s, int n, md_fs_format *f)
{
  int   c , i ;
  char *x ;
  
   for( i=0 , x = s , n-- ; i<n ; i++ , x++ ){
   
      c = mdFs_Ffgetc( f ) ;
	  if( c == EOF ){ *x = '\0' ; return i==0?NULL:s; }
	  *x = c ;
	  if( c == '\n' )break ;
	  
   }
   x++ ;
   *x = '\0' ;
   return s ;
}
int mdFs_Ffgetc( md_fs_format *f )
{
  int rc ;
  
   if( f->offset >= f->size ){
      if( f->size < f->maxSize )return EOF ;
      rc = mdFs_Read( &f->block , f->buffer , f-> maxSize );
      if( rc <= 0 )return rc ;
	  f -> size   = rc ;
      f -> offset = 0 ;
   }
   return f -> buffer[f->offset++] ;
}
int mdFs_Ffputs(const char *s, md_fs_format *f)
{
  for( ; *s != '\0' ; s++ )mdFs_Ffputc( (int)*s , f );
  return 0 ;
}
int mdFs_Ffputc( int c , md_fs_format *f )
{
  int rc ;
  
   f -> buffer[f->offset++] = c ;
   if( f->offset >= f->maxSize ){
      rc = mdFs_Write( &f->block , f->buffer , f-> maxSize );
      if( rc < 0 )return rc ;
      f -> offset = 0 ;
   }
   return 0 ;
}
int mdFs_FOpen( MDL * mdl , md_fs_format *f , md_id_t id , int level , char *mode )
{
  int rc ;
  
  f-> maxSize = MD_FS_BUFFER_SIZE ;
  f-> size    = 0 ;
  f-> offset  = 0 ;
  if( rc = mdFs_Open( mdl , &f->block , id , level , mode ) )return rc ;
  if(  f->block.mode  == mdFsREAD ){
     /*
      *   read ahead
      */
     rc = mdFs_Read( &f->block , f->buffer , f-> maxSize );
     if( rc < 0 )return rc ;
     f -> size = rc ;
   }else if( f->block.mode  == mdFsWRITE ){
   }
  
  return 0 ;
}
int mdFs_Open( MDL * mdl , md_fs_block *p , md_id_t id , int level , char *mode )
{
  md_attr attr ;
  int     rc ;
  
  p->mdl    = mdl ;
  p->level  = level ;
  p->id     = id ;

  if( ! strcmp( mode , "r" ) ){

     if( rc = md2GetAttribute( p->mdl , p->id , p->level , &attr) )return rc ;
     if( ( attr.unixAttr.mst_mode & 0170000 ) != 0100000 )return -10 ;
     p->size   = attr.entries ;
     p->mode   = mdFsREAD ;
     p->offset = 0 ;
  }else if( ! strcmp( mode , "w" ) ){
     if( rc = md2TruncateZero( p->mdl , p->id , p->level ))return rc ;
     p->size   = 0 ;
     p->mode   = mdFsWRITE ;
     p->offset = 0 ;
  }else return -1 ;
  return 0 ;
}
int mdFs_Write( md_fs_block *p , char *buffer , long size )
{
  int rc ;
  
  rc = md2WriteData( p->mdl , p->id , p->level , buffer , p->offset , size );
  if( rc < 0 )return rc ;
  p->offset += size ;
  
  return size ;

}
int mdFs_Read( md_fs_block *p , char *buffer , long size )
{
  int rc ;
  
  size = md_min( p->size - p->offset , size ) ;
  if( size <= 0 )return 0 ;
  rc = md2ReadData( p->mdl , p->id , p->level , buffer , p->offset , size );
  /*fprintf(stderr, " offset %d size %d\n" , p->offset , size ) ;*/
  if( rc < 0 )return rc ;
  p->offset += size ;
  return size ;
}
/*
 * ===========================================================================================
 *        the directory simulation part
 */
int md2ObjectToId( MDL *mdl , md_id_t id , char *str , md_id_t *resID )
{
 int rc ;
 md_id_t current ;
 char stringx[1024] , *string , *s;
  string = stringx ;
  strncpy( string , str , 1023 ) ;
  if(  strlen( string ) < 1 ){
     *resID = id ;
	 return 0 ;
  }
  if( *string == '/' ){  
      if( rc = md2GetRootDir( mdl , &current ) )return rc ;
      /*printf(" Root : %s\n",mdStringID(current));*/
      string++ ;
  }else{
      current = id ;
  }
  for( ; ; ){
     if( ( s = strchr( string , '/' ) ) == NULL ){
             /*printf( " Lookup : %s %s\n",mdStringID(current),string);*/
             if( rc = md2Lookup( mdl , current , string , resID ) )return rc ;
             /*printf( " Result : %s\n", mdStringID(*resID));*/
             return 0 ;
	 }else{
             *s = '\0' ;
             /*printf( " Lookup : %s %s\n",mdStringID(current),string);*/
             if( rc = md2Lookup( mdl , current , string , &id ) )return rc ;
             /*printf( " Result : %s\n", mdStringID(id));*/
             current = id ;
             string = s + 1 ;
	 }
  
  }
  
}
/*
 * ===========================================================================================
 *        the hostinfo part
 */
void md_string_to_permission( md_permission *ip , char *s )
{
  unsigned char b[sizeof(md_permission)] , x ;  
  int count , i , c , m ;
  
  memset( b , 0 , sizeof(md_permission) ) ;
  count = strlen( s ) ;
  
  for( i = count-1 , m = sizeof(md_permission)*2-1 ; i >= 0 ; i-- , m--){
      c =  ( ( s[i] >= 'a' ) && ( s[i] <= 'f' ) ) ? ( s[i] - 'a' + 10 ) :
	       ( ( s[i] >= 'A' ) && ( s[i] <= 'F' ) ) ? ( s[i] - 'A' + 10 ) :
	       ( ( s[i] >= '0' ) && ( s[i] <= '9' ) ) ? ( s[i] - '0' ) : 0 ;
	  x = ( m%2 ?  ( c & 0xF ) : ( ( c & 0xF ) << 4 )   ) ;
	  b[m/2] |= x ;
	  
  }
  memcpy( (char*)ip , b , sizeof(md_permission) ) ;
}
char * md_ip_to_dotted(  md_ip *ip )
{
  static char s[32] ;
  unsigned char *b ;
  b = (unsigned char *)ip ;
  
  sprintf(s,"%d.%d.%d.%d" ,b[0],b[1],b[2],b[3]) ;
  return s ;
  
}
void md_dotted_to_ip( md_ip *ip , char *string )
{
   unsigned long i1, i2, i3, i4, l ;
   
   sscanf( string, "%d.%d.%d.%d" , &i1,&i2,&i3,&i4 ) ;
   l = ( i1 << 24 ) | ( ( i2 & 0xFF ) << 16 ) | ( ( i3 & 0xFF ) << 8 ) | ( i4 & 0xFF ) ;
   memcpy( ip , (char *)&l , sizeof( md_ip ) ) ;
   return ;
}
int md2_PrintHostInfo( FILE * f , md_host_info *info )
{
 int i ;
   fprintf(f," ----------- Host Info ----------------------\n" ) ;
   fprintf(f,"  MD  Version       %d\n" , info->md_version ) ;
   fprintf(f,"  Generic Name      %s.%s (%s)\n" ,
             info->generic.name,info->domain,md_ip_to_dotted(&info->generic.ip) ) ;
   fprintf(f,"  System            %s  %s  %s\n" ,
             info->hardware,info->system,info->version ) ;
   for( i = 0 ; info->mountpoints[i].mountpoint[0] != '\0' ; i++ )
       fprintf(f,"       Mountpoint %d  %s  %s\n" , i , 
	            mdStringPermission( info->mountpoints[i].permission ) ,
				info->mountpoints[i].mountpoint     ) ;
				
   for( i = 0 ; info->interfaces[i].name[0] != '\0' ; i++ )
       fprintf(f,"       Interface  %d  %d %s(%s) %s\n" , i , 
	             info->interfaces[i].priority,
	             info->interfaces[i].name,md_ip_to_dotted(&info->interfaces[i].ip),
	             info->interfaces[i].ifType     );
				
   return 0 ;
}
int md2_GetHostInfo( md_fs_format *f , md_host_info *info )
{
    char string[256] , *ptr[32] ;
	int  rc , i , mpCount , ifCount;
	mpCount = 0 ;
	ifCount = 0 ;
    md_zero_struct( info );
/*
 * First Line :    <genericName>:<domain>:<ipNumber>:<hardware>:<operatingSys>:<osVersion>:
 *
 * Mountpoints :   mp:<mountPoint>:<mask>:
 *
 * Interfaces  :   if:<priority>:<ifName>:<hostIfName>:<ifIpNumber>:
 *
 * Example :
 *                 watphrakeo:desy.de:131.169.1.222:sun4m:SunOS:2.4.3:
 *                 mp:/generic:000020021:
 *                 mp:/security:000400021:
 *                 if:0:et0:watphrakeo:131.169.1.222:
 *                 if:1:ipg0:watphrakeo-f:131.169.30.22:
 *
 */
    info->md_version = 2 ;
	for( i = 0 ; mdFs_Ffgets( string , 256 , f ) != NULL ; ){
	   if( *string == '#' )continue ;
       rc = md2_GetConfLine( string , ptr , 32 );
	   if( i == 0 ){
	      if( rc < 6 )break ;
		  strncpy( info->generic.name , ptr[0] , MD_MAX_HN_SIZE-1 ) ;
		  strncpy( info->domain       , ptr[1] , MD_MAX_DOMAIN_SIZE-1 ) ;
		  md_dotted_to_ip( &info->generic.ip   , ptr[2] ) ;
		  strncpy( info->hardware   , ptr[3] , MD_MAX_HW_SIZE-1 ) ;
		  strncpy( info->system     , ptr[4] , MD_MAX_OS_SIZE-1 ) ;
		  strncpy( info->version    , ptr[5] , MD_MAX_VER_SIZE-1 ) ;
	   }else{
	      if( ! strcmp( ptr[0] , "mp" ) ){
             if( rc < 3 )continue ;  
			 strncpy( info->mountpoints[mpCount].mountpoint, ptr[1],MD_MAX_MP_SIZE-1) ;
             md_string_to_permission( &info->mountpoints[mpCount].permission , ptr[2] );
			 mpCount++;
		  }else if( ! strcmp( ptr[0] , "if" ) ){
             if( rc < 5 )continue ;
			 sscanf( ptr[1], "%d",&info->interfaces[ifCount].priority ) ;
			 strncpy( info->interfaces[ifCount].ifType, ptr[2],MD_MAX_IF_SIZE-1) ;
			 strncpy( info->interfaces[ifCount].name, ptr[3],MD_MAX_HN_SIZE-1) ;
			 md_dotted_to_ip( &info->interfaces[ifCount].ip   , ptr[4] ) ;
		     ifCount++;
		  }
	   }
	   i++ ;
	}
	return i == 0 ? -1 : 0 ;
}
int md2_GetConfLine( char *string , char *ptr[] , int ptrSize )
{
   int p ;
   char *s , *start ;
   
	for( s = string , start = s , p = 0; 1 ; s++ ){
		if( *s == '\0' )break ;
		if( *s == ':'  ){
		   *s = '\0' ;		   
		   ptr[p++] = start ;
		   if( p >= ptrSize )return p ;
		   start  = s+1 ;
	    } 
	
	}
    return p ;
}
