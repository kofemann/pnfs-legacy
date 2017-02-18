
#include <sys/time.h>

#include "dump_compat.h"

/**
 *  Various functions that may be needed for compatibility.
 */

#ifdef __sun
void timeradd( struct timeval *a, struct timeval *b, struct timeval *res)
{
  res->tv_sec = a->tv_sec + b->tv_sec;

  if( a->tv_usec + b->tv_usec < 1000000)
    res->tv_usec = a->tv_usec + b->tv_usec;
  else {
    res->tv_usec = a->tv_usec + b->tv_usec - 1000000;
    res->tv_sec++;
  }
}
#endif


#ifdef __sun
void timersub(struct timeval *a, struct timeval *b, struct timeval *res)
{
  res->tv_sec = a->tv_sec - b->tv_sec;

  if( b->tv_usec <= a->tv_usec) {
    res->tv_usec = a->tv_usec - b->tv_usec;
  } else {
    res->tv_usec = 1000000 + a->tv_usec - b->tv_usec;
    res->tv_sec--;
  }
}
#endif
