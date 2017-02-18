
#ifndef __DUMP_SERIALISER_H__
#define __DUMP_SERIALISER_H__

/**
 *  Ensure off_t is sufficient for 64-bit files.
 */

#include <time.h>     // NEEDED FOR md2ptypes.h
#include <stdio.h>    // NEEDED FOR md2types.h

#include "md2types.h" // NEEDED FOR sclib.h, also includes md2ptypes.h
#include "shmcom.h"   // NEEDED FOR sclib.h

#include "parse_lvl2.h"


#define FLAGS_REQ_DEFAULT   0x00
#define FLAGS_REQ_LEVEL2    0x01

/**
 *  The various return-codes for the call-backs.  The semantics are:
 *    ser_ok    .. continue with walk
 *    ser_error .. terminate walk quickly
 *    ser_skip  .. skip over any child items
 *                 (directory->tags and dir-entries; file->levels)
 */
typedef enum {
  ser_error=-1,
  ser_ok=0,
  ser_skip
} ser_return_t;


/**
 *  Information about a plugin that can dump PNFS data.  Each
 *  serialiser must complete one of these structures and register it.
 *  Registration is handled in ...
 *
 *  Call-back function may be NULL.  If not NULL, they should be a
 *  function that returns an integer: 0 => OK, any other return code
 *  will cause serialisation to stop immediately.
 */

typedef struct {

  const char *help;

  int flags;

  /**
   *  These mark the beginning and end of serialisation, independent
   *  of any content.  These are guarenteed to be called precisely
   *  once, unless the program crashes.
   *
   *  Between begin_serialisation/end_serialisation there is precisely
   *  one begin_dump/end_dump pair.
   */
  ser_return_t (*begin_serialisation)( const char *cmdline,
				       int argc, char *argv[]);

  ser_return_t (*end_serialisation)();


  /**
   *  Call-back for the first PNFS directory: the root directory.
   *  These will be called once (unless the PNFS root ID is invalid or
   *  there is some database problem).
   *
   *  Between begin_dump and end_dump there is precisely one
   *  begin_dir/end_dir call corresponding to the root directory.
   */
  ser_return_t (*begin_dump)( md_id_t root_id);
  
  ser_return_t (*end_dump)( md_id_t root_id);


  /**
   *  Call-back for beginning and end of a directory.  The
   *  parent_dir_id is the ID of the parent directory, except for the
   *  root directory, where parent_dir_id == id.
   *
   *  Between begin_dir and end_dir there are zero or more tags
   *  followed by zero or more directory items.  A directory item is
   *  either a begin_dir/end_dir pair (a sub-directory), a
   *  begin_file/end_file pair (a file), or a new_link.
   */
  ser_return_t (*begin_dir)( md_id_t parent_dir_id,
			     md_id_t id,
			     const char *name,
			     md_attr *attr);
  
  ser_return_t (*end_dir)( md_id_t parent_dir_id,
			   md_id_t id,
			   const char *name);


  /**
   *  Support for tags
   *
   *  We support four types of tags: primary, pseudo-primary, invalid
   *  and inherited.
   */


  /**
   *  A primary tag is the top-most element in a chain of tags.
   */
  ser_return_t (*new_primary_tag)( md_id_t parent_dir_id,
				   md_id_t id,
				   const char *name,
				   md_attr *attr,
				   const char *data,
				   size_t data_length);

  /**
   *  A pseudo-primary tag assigns a new value to a primary tag.  It
   *  retains the link to the next highest tag in the tag hierachy.
   *  If delete, the directory will inherit the next "highest"
   *  pseudo-primary tag or, if no more are present in the hierarchy,
   *  the corresponding primary tag.
   */
  ser_return_t (*new_pseudo_primary_tag)( md_id_t parent_dir_id,
					  md_id_t parent_tag_id,
					  md_id_t id,
					  const char *name,
					  md_attr *attr,
					  const char *data,
					  size_t data_length);

  /**
   *  An invalid tag breaks the tag chain but retains the link to the
   *  parent tag item.  If the invalid tag is deleted, then the
   *  original tag value is recovered.
   *
   *  TODO: verify that this is the actual behaviour.
   */
  ser_return_t (*new_invalid_tag)( md_id_t parent_dir_id,
				   md_id_t parent_tag_id,
				   md_id_t id,
				   const char *name);

  /**
   *  The inherited tag takes all values from it's corresponding
   *  parent primary or pseudo-primary tag.
   */
  ser_return_t (*new_inherited_tag)( md_id_t parent_dir_id,
				     md_id_t parent_tag_id,
				     md_id_t id,
				     const char *name);

  /**
   *  Called during the beginning and end of processing a file.  The
   *  attributes are those of level 0.
   *
   *  Between begin_file and end_file there may be zero or more
   *  new_checksum calls and zero or more new_level calls.
   */
  ser_return_t (*begin_file)( md_id_t parent_dir_id,
			      md_id_t id,
			      const char *name,
			      filesize_t size,
			      lvl2_info_t *lvl2_info,
			      md_attr *attr);

  ser_return_t (*end_file)( md_id_t parent_dir_id,
			    md_id_t id,
			    const char *name);

  /**
   *  Contains information about different levels.  Opaque heuristics
   *  is used to determine whether a level is defined.  The data will
   *  disappear after the call so make a copy if it is needed after
   *  the call-back returns.
   */
  ser_return_t (*new_level)( md_id_t file_id, int level,
			     md_attr *attr, const char *data,
			     size_t size);

  /**
   *  A (symbolic) link.  The data is (should be) a valid filename.
   */
  ser_return_t (*new_link)( md_id_t parent, md_id_t link_id, const char *name,
			    md_attr *attr, const char *data, size_t size);
 
} dump_serialiser_t;


#define CALL_SERIALISER(A,B,C) (A->B ? A->B C : ser_ok)
#define SERIALISER_HAS_FN(A,B) (A->B)

#endif
