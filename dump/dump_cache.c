/**
 *  Support routines falling into broadly two categories:
 *
 *    o  cache some limited client-side database information.
 *
 *    o  query this cache for specific queries.
 *
 *  The cached information is chosen so it can support the queries
 *  within dump_insubtree under normal circumstances.  These queries
 *  include:
 *
 *    o walking up the directory tree (discovering if a directory is
 *      within a directory subtree.
 *
 *    o mapping from tag inode ID to directory inode ID.
 *
 *    o walking up the tag inode tree to find the primary or
 *      pseudo-primary tag for a given inherited tag.
 *
 *  The cache query functions include a fall-back where it will ask
 *  the database.  This should allow the query routines to always
 *  produce the correct values independent of the caching policy.
 *
 *  The cache using a simple fixed-size hash-map where we hash an
 *  object's ID to a fixed static array.  The array has size 256
 *  buckets which, on a 32-bit machine, will result in 1kiB storage
 *  (in BSS) and twice that on 64-bit machines.  This can be increased
 *  from unsigned char to unsigned short (65535 buckets, resulting in
 *  256kiB usage), if bucket collisions proves to be a problem.
 */

#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "md2types.h" // needed for md2PrintID

#include "dump_utils.h"
#include "dump_wrap.h"

#include "dump_cache.h"


typedef enum {
  TAG_DATA=1,
  DIR_DATA,
} stored_type_t;

typedef struct {
  md_id_t dir;
  tag_type_t type;
  const char *data;
  size_t data_size;
  const char *name;
} tag_info_t;


typedef struct {
  md_id_t *tags;
  int tag_count;
} dir_info_t;


typedef struct _stored_info {
  stored_type_t type;

  md_id_t id;     // ID of this object
  md_id_t parent; // ID of this object's parent

  union {
    dir_info_t dir_info;
    tag_info_t tag_info;
  } data;

  struct _stored_info *next;
} stored_info_t;


typedef unsigned char hash_value_t;


// We use a fixed size hash table for simplicity.
static stored_info_t *hash_entries[256];

static int cache_hits, cache_misses, disable_cache;

static SCL *scl;

static hash_value_t calc_hash( md_id_t id);
static void store_info( stored_info_t *info, md_id_t id, md_id_t parent);
static void remove_info( md_id_t id);
static int find_info( md_id_t id, stored_info_t **info_p);
static int get_tag_type( md_id_t tag, tag_type_t *type_p);
static int get_tag_data( md_id_t tag, char **data_storage_p, size_t *data_length_p);
static int get_tag_id_by_name( const char *name, md_id_t dir_id, md_id_t *tag_id_p);
static void hash_long( hash_value_t *hash_p, unsigned long value);
static void hash_short( hash_value_t *hash_p, unsigned short value);

/**
 *  Public interface for cache.
 */


/**
 *  Register the database access handle.
 */
void cache_register_scl( SCL *new_scl)
{
  scl = new_scl;
}

void cache_disable()
{
  disable_cache=1;
}


/**
 *  Register information about a new directory
 */
void cache_dir_enter( md_id_t this_dir, md_id_t parent_dir)
{
  stored_info_t *new_dir;

  if( disable_cache)
    return;

  new_dir = malloc( sizeof( stored_info_t));

  new_dir->type = DIR_DATA;

  memset( &new_dir->data.dir_info, 0, sizeof( dir_info_t));

  store_info( new_dir, this_dir, parent_dir);
}


/**
 *  Register information about a new tag
 */
void cache_store_tag( md_id_t dir, md_id_t this_tag, md_id_t parent_tag, tag_type_t type,
		      const char *tag_name, const char *tag_data, size_t tag_data_size)
{
  dir_info_t *dir_info;
  stored_info_t *new_tag, *dir_item;
  char buffer[25];

  if( disable_cache)
    return;

  if( !find_info( dir, &dir_item)) {
    fprintf( stderr, "Failed to find corresponding dir %s for tag %s\n",
	     mdStringID( dir), mdStringID_r( this_tag, buffer));
    return;
  }
  dir_info = &dir_item->data.dir_info;

  // Store tag item
  new_tag = malloc( sizeof( stored_info_t));
  new_tag->type = TAG_DATA;
  memcpy( &new_tag->data.tag_info.dir, &dir, sizeof( md_id_t));
  new_tag->data.tag_info.type = type;
  new_tag->data.tag_info.data = tag_data ? malloc( tag_data_size+1) : NULL;
  new_tag->data.tag_info.name = strdup( tag_name);

  if( new_tag->data.tag_info.data) {
    memcpy( (char *)new_tag->data.tag_info.data, tag_data, tag_data_size+1);
    new_tag->data.tag_info.data_size = tag_data_size;
  } else {
    new_tag->data.tag_info.data_size = 0;    
  }

  store_info( new_tag, this_tag, parent_tag);

  // Update corresponding dir item
  dir_info->tag_count++;
  dir_info->tags = realloc( dir_info->tags, dir_info->tag_count * sizeof( md_id_t));
  dir_info->tags [dir_info->tag_count-1] = this_tag;
}


/**
 *  Free up space now that we're leaving a subdirectory.
 */
void cache_dir_leave( md_id_t id)
{
  if( disable_cache)
    return;

  remove_info( id);
}



/**
 *  Emit some statistics to the given output stream.
 */
void cache_emit_stats( FILE *fp)
{
  if( disable_cache)
    return;

  fprintf( fp, "Cache queries: %u (hits: %u, misses: %u)\n", cache_hits + cache_hits, cache_hits, cache_misses);
}



/**
 *  Look up the parent tag inode ID of the given tag inode ID.  If the
 *  value cannot be found from the cache, this routine will query the
 *  database.  If the database also fails, then the Null ID is
 *  returned.
 */
md_id_t cache_get_tag_parent( md_id_t tag) 
{
  int rc;
  stored_info_t *info;
  md_record this_tag_rec;
  md_id_t null_id;

  if( !disable_cache) {

    if( find_info( tag, &info)) {
      if( info->type == TAG_DATA) {
	cache_hits++;
	return info->parent;
      } else
	fprintf( stderr, "cache_get_tag_parent: found cached tag ID as directory cached entry.\n");
    }

    cache_misses++;
  }
  
  // Lookup from database.  
  rc = myGetRecord( scl, tag, &this_tag_rec);

  if( rc != 0) {
    fprintf( stderr, "Failed to obtain tag inode for cache_get_tag_parent().\n");
    fprintf( stderr, "\ttag: %s\n", md2PrintID( tag));

     // a db access problem
    mdSetNullID( null_id);
    return null_id;
  }

  return this_tag_rec.head.parentID;
}


/**
 *  Look up a tag by name for a given directory.  The cache is
 *  referred to first; should that fail, database lookups are used.
 *
 *  If tag_id_p is not NULL, the data it points to is updated with the
 *  tag's ID, if a matching tag is found.
 *
 *  Returns 1 if the tag was found, 0 otherwise.
 */
int cache_get_tag_id( md_id_t dir_id, const char *name, md_id_t *tag_id_p)
{
  md_record dir_rec, tag_rec;
  md_id_t this_tag_id;

  /* Try the cache first */
  if( !disable_cache) {
    if( get_tag_id_by_name( name, dir_id, tag_id_p)) {
      cache_hits++;
      return 1;
    }

    cache_misses++;
  }

  /* Cache lookup failed, try to look up from database */

  /* Lookup the directory inode record */
  if( myGetRecord( scl, dir_id, &dir_rec) != 0) {
    fprintf( stderr, "Cannot obtain root record\n");
    return 0;
  }

  if( !mdIsType(dir_rec.head.type, mdtDirectory | mdtInode)) {
    fprintf( stderr, "Non directory id used in cache_get_tag_id() call\n");
    fprintf( stderr, "\tid: %s\n", md2PrintID( dir_id));
    return 0;
  }

  /* Iterate over all tags for this directory. */
  for( this_tag_id = dir_rec.body.dirInode.attr.tag;
       !mdIsNullID( this_tag_id);
       this_tag_id = tag_rec.head.nextID) {

    if( myGetRecord( scl, this_tag_id, &tag_rec) != 0) {
      fprintf( stderr, "Failed to obtain record for a tag\n");
      fprintf( stderr, "\ttag id: %s\n", md2PrintID( this_tag_id));
      fprintf( stderr, "\tdirectory inode: %s\n", md2PrintID( dir_id));

      /**
       * If there was a problem with this tag, we would like to
       * continue; however, without the md_record, we can't.
       */
      return 0;
    }

    if( !strcmp( tag_rec.body.tagInode.name, name)) {
      if( tag_id_p)
	memcpy( tag_id_p, &this_tag_id, sizeof( md_id_t));

      return 1;
    }
  }

  return 0;
}




/**
 *  Look up the parent tag inode ID of the given tag inode ID from the
 *  cache.  If the value cannot be found from the cache then the Null
 *  ID is returned.
 */
md_id_t cache_get_cached_tag_parent( md_id_t tag)
{
  stored_info_t *info;
  md_id_t null_id;

  if( find_info( tag, &info)) {
    if( info->type == TAG_DATA) {
      cache_hits++;
      return info->parent;
    } else
      fprintf( stderr, "cache_get_cached_tag_parent: found cached tag ID as directory cached entry.\n");
  }

  cache_misses++;
 
  mdSetNullID( null_id);
  return null_id;
}




/**
 *  Query to discover the corresponding directory given a tag inode
 *  ID.  If the answer is not known from the cache, the routine will
 *  query the database.  If there is a problem querying the database
 *  then the Null ID is returned.
 */
md_id_t cache_get_tag_dir( md_id_t tag)
{
  int rc;
  stored_info_t *info;
  md_record this_tag_rec;
  md_id_t null_id;

  if( !disable_cache) {
    if( find_info( tag, &info)) {
      if( info->type == TAG_DATA) {
	cache_hits++;
	return info->data.tag_info.dir;
      } else
	fprintf( stderr, "cache_get_tag_dir: found cached tag ID as directory cached entry.\n");
    }

    cache_misses++;
  }

  /* We don't have it in cache, must look it up in the database */

  /* If we have not database handle registered, fail */
  if( !scl)
    goto error_exit;

  // Lookup from database.  
  rc = myGetRecord( scl, tag, &this_tag_rec);

  if( rc != 0) {
    fprintf( stderr, "Failed to obtain tag inode for cache_get_tag_dir().\n");
    fprintf( stderr, "\ttag: %s\n", md2PrintID( tag));
    goto error_exit;
  }

  return this_tag_rec.head.baseID;

 error_exit:
  mdSetNullID( null_id);
  return null_id;  
}


/**
 *  Query to obtain the parent directory inode ID of the given
 *  directory inode ID.  If the answer is not obtainable from the
 *  cache then the routine will query the database.  Should the
 *  database be unable to provide an answer then the Null ID is
 *  returned.
 */
md_id_t cache_get_dir_parent( md_id_t dir)
{
  md_record this_dir_rec;
  stored_info_t *info;
  int rc;
  md_id_t null_id;

  if( !disable_cache) {
    if( find_info( dir, &info)) {
      if( info->type == DIR_DATA) {
	cache_hits++;
	return info->parent;
      }

      fprintf( stderr, "cache_get_dir_parent: found cached directory ID as tag.\n");
    }

    cache_misses++;
  }

  /* If we have not database handle registered, fail */
  if( !scl)
    goto error_exit;

  // Lookup from database.  
  rc = myGetRecord( scl, dir, &this_dir_rec);

  if( rc != 0) {
    fprintf( stderr, "Failed to obtain directory inode for cache_get_dir_parent().\n");
    fprintf( stderr, "\tdirectory: %s\n", md2PrintID( dir));

    // problem accessing database entry.
    goto error_exit;
  }

  return this_dir_rec.head.parentID;

 error_exit:
  mdSetNullID( null_id);
  return null_id;
}


/**
 *  Check whether we know this directory to be within the search
 *  subtree.  Returns 1 if known to be within the subtree, 0 if
 *  known to be outside the subtree, -1 if unknown.
 *
 *  We currently do not cache entries outwith the walk sub-tree, so
 *  this routine currently will never return 0.
 */
int cache_is_dir_known_subtree( md_id_t dir)
{
  stored_info_t *info;

  if( find_info( dir, &info)) {
    if( info->type == DIR_DATA)
      return 1;

    fprintf( stderr, "cache_get_dir_parent: found cached dir ID as tag.\n");
  }

  return -1;
}


/**
 *  Given a tag's inode ID, discover the tag's corresponding value.
 *  This involves ascending the parent hierarchy to find either a
 *  primary or pseudo-primary tag and updating the data-storage,
 *  data-size and attributes so it contains that (pseudo-)primary
 *  tag's information.
 *
 *  The tag data is stored in memory pointed to by data_storage.  At
 *  most, data_storage_size bytes are used.  If data_length is not
 *  NULL then it is updated to the length of the data.
 *
 *  As much as possible is done using the cache; however, should
 *  entries be not stored within a cache, the routine will silently
 *  switch to using the database.
 *
 *  Returns 1 on success, 0 on failure.
 */
int cache_find_tag_value( md_id_t tag, char *data_storage, size_t data_storage_size,
			  size_t *data_length_p)
{
  int have_obtained_rec;
  md_id_t current;
  md_record rec;
  md_tag_inode *tinode = &rec.body.tagInode;
  tag_type_t type;
  size_t fetched_data_size=0, max_size;
  char *fetched_data=NULL;

  for( current = tag; !mdIsNullID( current); current = cache_get_tag_parent( current)) {
    have_obtained_rec = 0;

    // If we cannot find the tag in the cache, look it up from the database.
    if( !get_tag_type( current, &type)) {
      if( myGetRecord( scl, current, &rec) != 0) {
	fprintf( stderr, "Database lookup failure for tag\n");
	fprintf( stderr, "\ttop-most tag: %s\n", md2PrintID( tag));
	fprintf( stderr, "\tcurrent tag: %s\n", md2PrintID( current));
	return 0;  // FAIL
      }

      if( !tag_identify_type( &rec, &type)) {
	fprintf( stderr, "Database lookup revealed something not a tag inode\n");
	fprintf( stderr, "\ttop-most tag: %s\n", md2PrintID( tag));
	fprintf( stderr, "\tcurrent tag: %s\n", md2PrintID( current));
	return 0;  // FAIL
      }

      have_obtained_rec = 1;
    }

    switch( type) {

    case PRIMARY:
    case PSEUDO_PRIMARY:

      // If we haven't already obtained the db record...
      if( !have_obtained_rec) {

	// Try to fetch the data from the cache.
	get_tag_data( current, &fetched_data, &fetched_data_size);

	/**
	 * If we failed to retrieve a cached data (or the data is NULL),
	 * fetch the data from the database.
	 */
	if( !fetched_data) {
	  if( myGetRecord( scl, current, &rec) != 0) {
	    fprintf( stderr, "Database lookup failure for tag\n");
	    fprintf( stderr, "\ttop-most tag: %s\n", md2PrintID( tag));
	    fprintf( stderr, "\tcurrent tag: %s\n", md2PrintID( current));
	    return 0;  // FAIL
	  }

	  have_obtained_rec = 1;
	}
      }

      if( have_obtained_rec) {
	fetched_data = tinode->data;
	fetched_data_size = tinode->attr.entries;
      }

      if( data_storage) {
	max_size = fetched_data_size < (data_storage_size-1) ? fetched_data_size : data_storage_size-1;
	memcpy( data_storage, fetched_data, max_size);
	data_storage [max_size] = '\0';

	data_storage [data_storage_size-1] = '\0'; // Just to be sure.
      }

      if( data_length_p)
	*data_length_p = fetched_data_size;

      return 1;  // SUCCESS

    case INVALID:
      fprintf( stderr, "Tag type is invalid, which is not supported.\n");
      fprintf( stderr, "\ttop-most tag: %s\n", md2PrintID( tag));
      fprintf( stderr, "\tcurrent tag: %s\n", md2PrintID( current));
      break;

    case INHERITED:
      // Simply iterate up the hierarchy.
      break;
    }
  }

  fprintf( stderr, "Broken tag hierarchy: unable to find primary parent tag\n");
  fprintf( stderr, "\ttop-most tag: %s\n", md2PrintID( tag));
  return 0; // FAIL.
}





/**
 *  Private functions.
 */


/**
 *  Store some information against an (directory or tag) inode ID: no
 *  checking is done for duplicate entries.
 */
void store_info( stored_info_t *info, md_id_t id, md_id_t parent)
{
  hash_value_t hash;

  memcpy( &info->id, &id, sizeof( md_id_t));
  memcpy( &info->parent, &parent, sizeof( md_id_t));
  
  hash = calc_hash( id);

  // Add to head of LL.
  info->next = hash_entries[hash];
  hash_entries[hash] = info;
}




/**
 *  Lookup some stored information based on the supplied ID.  If
 *  info_p is not NULL, then the location pointed to is updated so it
 *  points to the record.
 *
 *  Returns 1 if item was found, 0 if not.
 */
int find_info( md_id_t id, stored_info_t **info_p)
{
  hash_value_t hash;

  stored_info_t *this;

  hash = calc_hash( id);

  // Walk LL for item
  for( this = hash_entries[hash]; this; this = this->next) {
    if( mdIsEqualID( id, this->id)) {
      if( info_p)
	*info_p = this;

      return 1;
    }
  }

  return 0; // not found.
}


/**
 *  Lookup the type of tag of the given tag inode ID.  If the type
 *  cannot be discovered from cached information then type_p is left
 *  unaltered.
 *
 *  Returns 1 if the tag inode ID is found in cache, 0 otherwise.
 */
int get_tag_type( md_id_t tag, tag_type_t *type_p)
{
  stored_info_t *info;

  if( !disable_cache) {
    if( find_info( tag, &info)) {
      if( info->type == TAG_DATA) {
	cache_hits++;

	if( type_p)
	  *type_p = info->data.tag_info.type;
	
	return 1;
      } else
	fprintf( stderr, "get_tag_type: found cached tag ID as directory cached entry.\n");
    }  

    cache_misses++;
  }

  return 0;
}




/**
 *  Remove an item from the cache.  If the item is information about a
 *  tag inode then it is simply removed from the hash.  If the item is
 *  information about a directory inode then any corresponding tag
 *  inode information is also removed; this is achieved by calling
 *  remove_info() from within remove_info().
 */
void remove_info( md_id_t id)
{
  int i;
  hash_value_t hash;
  stored_info_t *previous, *current;

  hash = calc_hash( id);

  // Walk LL looking for item
  for( previous=NULL, current = hash_entries[hash];
       current;
       previous = current, current=current->next) {

    // If found it...
    if( mdIsEqualID( id, current->id))
      break;
  }

  if( current) {

    //  Delink from LL.
    if( previous)
      previous->next = current->next;
    else
      hash_entries[hash] = current->next;

    //  Cache entry type specific handling.
    switch( current->type) {
    case DIR_DATA:
      for( i = 0; i < current->data.dir_info.tag_count; i++)
	remove_info( current->data.dir_info.tags[i]);

      free( current->data.dir_info.tags);
      break;

    case TAG_DATA:
      free( (void *) current->data.tag_info.data);
      free( (void *) current->data.tag_info.name);
      break;
    }

    free( current);

  } else {
    fprintf( stderr, "Failed to find ID %s when removing cached item\n", mdStringID( id));
  }
}



/**
 *  Try to obtain a tag's ID by name.
 *
 *  Return 1 on success, 0 on failure.
 */
int get_tag_id_by_name( const char *name, md_id_t dir_id, md_id_t *tag_id_p)
{
  stored_info_t *dir_info, *tag_info;
  int i;

  /* Lookup cached directory entry */
  if( !find_info( dir_id, &dir_info))
    return 0; // FAILED

  if( dir_info->type != DIR_DATA) {
    fprintf( stderr, "ASSERT: dir_info.type != DIR_DATA\n");
    return 0; // FAILED
  }

  /* Scan through the directory's tags, looking for a match */
  for( i = 0; i < dir_info->data.dir_info.tag_count; i++) {

    if( !find_info( dir_info->data.dir_info.tags [i], &tag_info)) {
      fprintf( stderr, "ASSERT: recorded tag %u in dir %s does not exist in cache\n",
	       i, md2PrintID( dir_id));
      continue;
    }

    if( tag_info->type != TAG_DATA) {
      fprintf( stderr, "ASSERT: tag_info.type != TAG_DATA for tag %u in dir %s\n",
	       i, md2PrintID( dir_id));
      continue;
    }

    if( !strcmp( tag_info->data.tag_info.name, name)) {

      // We've found it!
      if( tag_id_p)
	memcpy( tag_id_p, &tag_info->id, sizeof( md_id_t));
      
      return 1; // SUCCESS
    }
  }
  
  return 0; // FAILED
}



/**
 *  Try to look up a tag's data from the cache.
 *
 *  return 1 on success, 0 on failure.
 */
int get_tag_data( md_id_t tag, char **data_storage_p, size_t *data_length_p)
{
  stored_info_t *info;

  if( find_info( tag, &info)) {
    if( info->type == TAG_DATA) {
      cache_hits++;

      if( data_storage_p)
	/* TODO: not have to drop the const */
	*data_storage_p = (char *) info->data.tag_info.data; 

      if( data_length_p)
	*data_length_p = info->data.tag_info.data_size;

      return 1;
    }

    fprintf( stderr, "get_tag_type: found cached tag as directory cached entry.\n");
  }  

  cache_misses++;

  return 0;
}


#define BITS_IN_BYTE 8

/**
 *  Calculate a hash value for given (inode) ID The current routine
 *  should be independent of the number of bytes used in hash_value_t.
 *  However, if hash_value_t is more than a single byte, it may prove
 *  better to byte-swap the database part of the ID, so avoiding some
 *  bucket collisions.
 */
hash_value_t calc_hash( md_id_t id)
{
  hash_value_t hash_value = 0;

  hash_long( &hash_value, id.low);
  hash_long( &hash_value, id.high);

  hash_short( &hash_value, id.ext);
  hash_short( &hash_value, id.db);

  return hash_value;
}


void hash_long( hash_value_t *hash_p, unsigned long value)
{
  unsigned long mask = (1 << sizeof( hash_value_t) * BITS_IN_BYTE) -1;
  hash_value_t hash=0;
  int i;

  for( i = 0; i < sizeof(unsigned long)/sizeof( hash_value_t); i++) {
    hash ^= value & mask;
    value >>= sizeof( hash_value_t)*BITS_IN_BYTE;
  }

  *hash_p ^= hash;
}

void hash_short( hash_value_t *hash_p, unsigned short value)
{
  unsigned short mask = (1 << sizeof( hash_value_t) * BITS_IN_BYTE) -1;
  hash_value_t hash=0;
  int i;

  for( i = 0; i < sizeof(unsigned short)/sizeof( hash_value_t); i++) {
    hash ^= value & mask;
    value >>= sizeof( hash_value_t)*BITS_IN_BYTE;
  }
  
  *hash_p ^= hash;
}

