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
#ifndef MD2_SCAN__H_
#define MD2_SCAN__H_

#define  MDS_INVALID (300)
#define  MDS_START   (301)
#define  MDS_TOKEN   (302)
#define  MDS_ARGS    (303)
#define  MDS_NAME    (304)
#define  MDS_END     (305)

#define  MAX_OBJECT_NAME  (32)
#define  MAX_OBJECT_TYPE  (64)
#define  MAX_OBJECT_ARGS  (256)

typedef struct {
  int numType , argc ;
  char *name ;
  char type[MAX_OBJECT_TYPE] ;
  char args[MAX_OBJECT_ARGS] ;
  char *argv[32] ;
} md_object ;

#define md2ObjectType(o)   ((o)->numType)
#define md2ObjectName(o)   ((o)->name)

#define MDO_UNKNOWN       (-1)
#define MDO_TAG           (1)
#define MDO_REQ_ID        (2)
#define MDO_REQ_PATH      (3)
#define MDO_REQ_JUMP      (4)
#define MDO_REQ_ACCESS    (5)
#define MDO_REQ_SHOWID    (6)
#define MDO_REQ_LSTAGS    (7)
#define MDO_REQ_FORCEIO   (8)
#define MDO_REQ_FORCEIO2  (9)
#define MDO_REQ_CONFIG    (10)
#define MDO_REQ_FSET      (11)
#define MDO_REQ_CONST     (12)
#define MDO_REQ_NAME      (13)
#define MDO_REQ_LEVEL     (14)
#define MDO_REQ_DIGITS    (15)
#define MDO_REQ_PARENT    (16)
#define MDO_REQ_NAMEOF    (17)
#define MDO_REQ_SET       (18)
#define MDO_REQ_SET_SIZE  (19)
#define MDO_REQ_GET       (20)
#define MDO_REQ_GET_COUNTERS (21)
#define MDO_REQ_GET_DATABASE (22)
#define MDO_REQ_PSET      (23)
#define MDO_REQ_GETATTR   (24)
#define MDO_REQ_LSXTAGS   (25)
#define MDO_REQ_LIST      (26)
#define MDO_REQ_IGNORE    (27)

int md2scanObjectString( char *string , md_object *obj );
md_object * md2IsObject( char * string  );

#endif
