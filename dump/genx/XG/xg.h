/* $Id: xg.h,v 1.3 2004/03/18 19:17:50 dvd Exp $ */

#ifndef XG_H
#define XG_H

#include <stdarg.h>

#define PIFILE "davidashen-net-xg-file"
#define PIPOS "davidashen-net-xg-pos"

#define ER_IO 0
#define ER_XML 1
#define ER_NOXENT 2
#define ER_NOSID 3
#define ER_GENX 4
#define ER_MALLOC 8

extern void xg_perror(int erno,va_list ap);
extern char *xg_abspath(char *r,char *b); /* r must be long enough 
                                             for absolute path */
#endif
