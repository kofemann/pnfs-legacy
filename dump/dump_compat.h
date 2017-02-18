
#include <sys/time.h>

/**
 *  Various functions that may be needed for compatibility.
 */

#ifdef __sun
void timeradd( struct timeval *a, struct timeval *b, struct timeval *res);
#endif

#ifdef __sun
void timersub(struct timeval *a, struct timeval *b, struct timeval *res);
#endif

