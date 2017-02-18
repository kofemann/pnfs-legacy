#ifndef __DUMP_CACHE__
#define __DUMP_CACHE__

/* Needed for the (FILE *) */
#include <stdio.h>

/* Needed for md2ptypes.h */
#include <time.h>

/* Needed for md_id_t */
#include "md2ptypes.h"

/* Needed for SCL */
#include "shmcom.h"

#include "dump_tag.h"

void cache_register_scl( SCL *new_scl);
void cache_disable();

void cache_dir_enter( md_id_t this_dir, md_id_t parent_dir);

void cache_store_tag( md_id_t dir, md_id_t this_tag, md_id_t parent_tag, tag_type_t type,
		      const char *tag_name, const char *tag_data, size_t tag_data_size);

void cache_dir_leave( md_id_t id);
void cache_emit_stats( FILE *fp);

md_id_t cache_get_tag_parent( md_id_t tag);
md_id_t cache_get_tag_dir( md_id_t tag);
md_id_t cache_get_dir_parent( md_id_t dir);
int cache_get_tag_id( md_id_t dir_id, const char *name, md_id_t *tag_id_p);
int cache_find_tag_value( md_id_t tag, char *data_storage, size_t data_storage_size,
			  size_t *data_length_p);


/**
 *  Routines that query the cache without any database access.
 */
md_id_t cache_get_cached_tag_parent( md_id_t tag);
int cache_is_dir_known_subtree( md_id_t dir);

#endif
