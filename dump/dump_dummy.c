/**
 *  Various skeleton and dummy routines to support linkage.
 */

#include "md2log.h"
#include "md2ptypes.h"

#include "dump_dummy.h"

static void md2ScanIdLevel( char *s , md_id_t *id , int *level );


int md2pPrintf( int prio , char *fmt , ... )
{  
  va_list ap;
  FILE *out = stdout;
  int n;

  va_start(ap, fmt);
  n = vfprintf( out, fmt, ap);
  va_end(ap);

  return n;
}

/**
 *  The following functions are taken from dbfs/md2lib.c.  They are
 *  included here because we cannot link against md2lib without
 *  including much baggage.  There are minor corrections to remove
 *  warnings.
 */
void md2ScanId( char *s , md_id_t *id )
{
  md2ScanIdLevel( s , id , NULL );
  return ;
}

void md2ScanIdLevel( char *s , md_id_t *id , int *level )
{
   char tmp[2*sizeof(md_id_t)+2];
   int len = strlen( s ) ;
   
   len = len > 2*sizeof(md_id_t) ?  2*sizeof(md_id_t)  :len ;
   memset( tmp , '0' , 2*sizeof(md_id_t) ) ;
   tmp[2*sizeof(md_id_t)] = '\0' ;
   memcpy( &tmp[2*sizeof(md_id_t)-len] , s , len ) ;
   sscanf( tmp , "%4hx%4hx%8x%8x", &id->db,&id->ext,(unsigned int *)&id->high,(unsigned int *)&id->low);
   if( level ){
     *level    = (*id).low & 0x7 ;
     (*id).low = (*id).low & ~ 0x7 ;  
   }
}

char * md2PrintID( md_id_t id )
{
   static char str[2*sizeof(md_id_t)+1] ;
   sprintf(str,"%4.4X%4.4X%8.8lX%8.8lX",id.db,id.ext,id.high,id.low) ;
   str[2*sizeof(md_id_t)] = '\0' ;
   return str ;
}

