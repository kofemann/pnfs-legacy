#ifndef __DUMP_TAG_H__
#define __DUMP_TAG_H__

#include <stdio.h> // Needed for FILE* in md2types.h

#include "md2types.h" // TODO: should this be something else?

/**
 *  Use the TAG_TYPE_NAME() macro rather than the direct variable
 *  _tag_type_name to prevent over- or under-flow problems with the
 *  array.
 */

#define TAG_TYPE_NAME(A) (((A) < PRIMARY || (A) > INHERITED) ? "Out-of-bound" : _tag_type_name[A])


typedef enum {
  PRIMARY=1,
  PSEUDO_PRIMARY,
  INVALID,
  INHERITED,
} tag_type_t;


int tag_identify_type( md_record *rec, tag_type_t *type_p);

extern const char *_tag_type_name[];

#endif
