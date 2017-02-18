/**
 *  Simple routines to interrogate a tag's inode record and derive
 *  useful information.
 *
 *  This provides basic support for mapping a tag inode to one of
 *  several tag types: primary, pseudo-primary, etc...
 */

#include "dump_tag.h"

const char *_tag_type_name[] = {
  "unknown",
  "primary",
  "pseudo-primary",
  "invalid",
  "inherited",
};


/**
 *  Given a record, identify which type of tag this is.
 *
 *  Returns 1 on success, 0 on failure.
 */
int tag_identify_type( md_record *tag, tag_type_t *type_p)
{
  md_tag_inode *tinode;
  tag_type_t type;

  if( !mdIsType( tag->head.type, mdtTag | mdtInode))
    return 0;

  tinode = &tag->body.tagInode;

  if( mdIsNullID( tag->head.parentID)) {
    type = PRIMARY;
  } else if( !(tinode->attr.flags & mdTagInherited)) {  
    type = PSEUDO_PRIMARY;
  } else if( tinode->attr.flags & mdTagInvalid) { 
    type = INVALID;
  } else {
    type = INHERITED;
  }

  if( type_p)
    *type_p = type;

  return 1;
}
