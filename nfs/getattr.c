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
#include <sys/types.h>
#include <sys/stat.h>


main( int argc , char *argv[] )
{
  struct stat attr;
  int rc ;

  if( argc != 2 )exit(1);

  rc = lstat( argv[1] , &attr ) ;
  if( rc < 0 ){
    fprintf(stderr," lstat : %d\n",errno);
    exit(1);
  }
  printf( " inode=%d;dev=%d;path=%s\n",attr.st_ino,attr.st_dev,argv[1]);
  

}
