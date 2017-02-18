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
#ifndef MD2LOG__H_
#define MD2LOG__H_
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#if ! defined(solaris) && ! defined(darwin)
#include <memory.h>
#include <malloc.h>
#endif
#ifndef NO_NETWORK
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>
#endif

typedef struct {

   FILE *out ;
   int   outCounter ;
   int   level ;
   char  file[1024] ;
}md2pBase ;
#ifdef NO_NETWORK
typedef  void * md2WGC ;
#else
typedef struct md2pWorldGate {
   int fd;
   struct sockaddr_in  defLogIP ;
}  md2WGC;
#endif
md2WGC * md2rpWorldGateClass( unsigned long ip ,int port );
int md2rpSendMessageLow( md2WGC *world , char *message , int len );
int md2openWorld();
int md2sendWorld( char * message );

#define md2pMOREINFO  (3)
#define md2pINFO      (4)
#define md2pMODINFO   (5)
#define md2pIncLevel  (-1)
#define md2pMaxLevel  (9)
#define md2pLevelMask (0xFF)
#define md2pW         (0x100)

#define md2worldOk(x)     ((x)&md2pW)
#define md2pLevelOf(x)    ((x)&md2pLevelMask)

int md2pOpen( char *filename , long flags ) ;
int md2pPrintf( int prio , char *format , ... ) ;
int md2pPutf( int prio , char *string ) ;
int md2pClose();
int md2pNewLevel( int level );


#endif
