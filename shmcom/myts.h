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
 * Some time measurement structures
 */
/*
 * Structure returned by gettimeofday(2) system call,
 * and used in other calls.
 */
#ifndef MYTS_H
#ifdef testOnly

#include <Events.h>

#define NO_DELAY   0

#define EWOULDBLOCK   (-5555) 

struct timeval {
        long    tv_sec;         /* seconds */
        long    tv_usec;        /* and microseconds */
};
struct tz { long xxx ; } ;
/*
 *  the FD simulation.
 */
#define FD_ZERO(value)         {*(value)=0;}
#define FD_SET(pos,value)      {*(value) |= (1L << (pos) ) ; }
#define FD_CLR(pos,value)      {*(value) &= ( ~( 1L << (pos) ) ) ; }
#define FD_ISSET(pos,value)    (*(value)&(1L<<(pos)))

#define FD_SETSIZE    16

typedef unsigned long fdset ;
typedef unsigned long fd_set ;
#define gettimeofday(x,y) {(x)->tv_usec=(1000000*(TickCount()%60))/60;\
                           (x)->tv_sec=TickCount()/60;\
						   (y)->xxx = 0;}
#define select(h,w,r,x,t)    (1)
#endif
#endif

#include <sys/time.h>

#define mUT   (1000000)
typedef struct timeval ts ;
#define tsGet(t) { struct timezone tempTZ ; gettimeofday(t,&tempTZ) ; }
#define tsZero(t) {(t)->tv_sec=0;(t)->tv_usec=0;}
#define tsAdd(t,d) {(t)->tv_sec+=(d)->tv_sec;\
                    (t)->tv_usec+=(d)->tv_usec;  \
					if((t)->tv_usec>=mUT){  \
					(t)->tv_sec++;(t)->tv_usec-=mUT;}}
#define tsSub(t,d) {(t)->tv_sec-=(d)->tv_sec;\
                    (t)->tv_usec-=(d)->tv_usec;  \
		    if((t)->tv_usec<0){  \
		    (t)->tv_sec--;(t)->tv_usec=mUT+(t)->tv_usec;}}
#define tsSay2(string,t) printf( "%s %d:%d:%d\n",string,\
        (t)->tv_sec,(t)->tv_usec/1000,(t)->tv_usec%1000)
#define tsSay(string,t) printf( "%s %d:%d\n",string,\
        (t)->tv_sec,(t)->tv_usec/1000)
#define tsMsecs(t) ((t)->tv_sec*1000+(t)->tv_usec/1000)
#define tsSec(t) (((float)((t)->tv_sec))+((float)((t)->tv_usec))/1000000.)
#define tsSecs(t) ((t)->tv_sec)
