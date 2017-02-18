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
#ifndef __DBGPRINT__H__
#define __DBGPRINT__H__

int dbgprintOpen( char * filename ) ;
int dbgprintRewind() ; 
int dbgprintClose() ;
int dbgprintln( char *format , ... ) ;


#endif
