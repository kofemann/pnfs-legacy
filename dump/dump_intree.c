/**
 *  dump_intree: check whether a tag inode or directory inode is
 *  within a specified subtree.
 */
#include <string.h>

#include "dump_cache.h"

#include "dump_intree.h"


/**
 *  NB. we don't have a static Null initialiser; but, by happy
 *  coincidence, the default initialiser for static is equivalent.
 */
static md_id_t subtree_root_id;
static md_id_t tree_root_id;


/**
 *  PUBLIC API
 */

void insubtree_register_root( md_id_t root_id)
{
  memcpy( &tree_root_id, &root_id, sizeof( md_id_t));
}

void insubtree_register_sub_root( md_id_t root_id)
{
  memcpy( &subtree_root_id, &root_id, sizeof( md_id_t));
}

md_id_t insubtree_get_sub_root()
{
  return subtree_root_id;
}



/**
 *  Is this tag within the (sub-)tree?
 *
 *  Return 1 if within subtree, 0 otherwise.
 */
int insubtree_tag_within_subtree( md_id_t tag)
{
  md_id_t dir;

  if( mdIsNullID( tag))
    fprintf( stderr, "ASSERT insubtree_tag_within_subtree() is given Null tag ID\n");


  //  If subtree isn't set, we're dumping the whole tree.
  if( mdIsNullID( subtree_root_id))
    return 1;

  // Find tag's correponding dir-inode and walk up the directory inodes.
  for( dir = cache_get_tag_dir( tag);
       !mdIsEqualID( dir, tree_root_id);
       dir = cache_get_dir_parent( dir)) {

    /**
     *  If this dir is the subtree root, the tag's parent is within the
     *  subtree.
     */
    if( mdIsEqualID( dir, subtree_root_id))
      return 1;

    /**
     * NullID should only be returned when querying the DB and
     * something serious happened: a connection or database corruption
     * problem.
     */
    if( mdIsNullID( dir))
      return 1;  // TODO: is this the safe option?

    switch( cache_is_dir_known_subtree( dir)) {
    case 1:  // Known to be within the subtree
      return 1;

    case 0: // Known to be outside the subtree
      return 0;

    case -1:  // Unknown, so iterate up.
      break;      
    }
  }

  // Reached the root of the whole PNFS tree.  
  return 0;
}


