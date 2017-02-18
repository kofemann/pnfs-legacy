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
#include <fcntl.h>

#include "md2types.h"
#include "md2fs.h"

main( int argc ,char *argv[] )
{

  int rc  ;
  mdRecord r ;
  MDL *mdlIn , *mdlOut ;
  char *infile , *outfile ;

  infile = argv[1] ;
  outfile = argv[2] ;

  mdlIn  = NULL ;
  mdlOut = NULL ;
  if( argc < 3 ){
    fprintf(stderr," USAGE : %s baseDB slaveDB\n",argv[0]);
    exit(1) ;
  }

   rc = 0 ;
   if( ! ( mdlIn = md2OpenReadOnly( infile , &rc ) ) ){
       fprintf( stderr," Can't open %s : %d\n" , infile , rc );
       exit(1);
   }
   if( ! ( mdlOut = md2OpenReadOnly( outfile , &rc ) ) ){
       fprintf( stderr," Can't open %s : %d\n" , outfile , rc );
       md2Close( mdlIn ) ;
       exit(1);
   }
   for( md2GetFirstKey( mdlIn , &r.head.ID ) ;
        ! mdIsNullID( r.head.ID ) ;
        md2GetNextKey( mdlIn , r.head.ID , &r.head.ID ) ){


      if( rc = md2GetRecord( mdlIn , &r , 0 ) ){
         fprintf(stderr,"not found in slave %s\n",mdStringID(r.head.ID));
      }
   }


   if( mdlIn )md2Close( mdlIn ) ;
   if( mdlOut )md2Close( mdlOut ) ;
   exit(0);
}
