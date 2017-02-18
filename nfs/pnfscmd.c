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
#include <utime.h>
#include <errno.h>

main( argc , argv )
int argc ;
char *argv[] ;
{
   struct utimbuf times;
   int rc ;

   if( argc < 4 ){
     fprintf(stderr," USAGE : %s path command argument\n" , argv[0] ) ;
     exit(1);
   }
   sscanf( argv[2] , "%d" , &times.actime );
   sscanf( argv[3] , "%x" , &times.modtime );
   rc = utime( argv[1] , &times);
   if(rc){
     fprintf(stderr," Problem : errno = %d\n" , errno ) ;
     exit(1) ;
   }
   exit(0);
}
