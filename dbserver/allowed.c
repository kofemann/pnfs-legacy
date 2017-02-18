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
#include "md2types.h"
#include "allowed.h"

int _mdBasicWriteAllowed( md_unix * attr , md_auth * auth ){
   int i ;
   if( __mdBasicWriteAllowed(attr,auth) )return 1 ;
   for( i = 0 ; ( i < auth->gidsLen ) && 
                ( auth->gids[i] != attr->mst_gid ); i++ ) ;

   return ( i < auth->gidsLen ) && ((attr)->mst_mode&S_IWGRP);
}
int _mdBasicReadAllowed( md_unix * attr , md_auth * auth ){
   int i ;
   if( __mdBasicReadAllowed(attr,auth) )return 1 ;
   for( i = 0 ; ( i < auth->gidsLen ) && 
                ( auth->gids[i] != attr->mst_gid ); i++ ) ;

   return ( i < auth->gidsLen ) && ((attr)->mst_mode&S_IRGRP);
}
