/**
 *  Implement a Directory Iterator using PNFS' ReadDir support
 */
#include <stdlib.h>
#include <string.h>  // needed by setMaxAuth()

#include "dump_di_dbscan.h"
#include "dump_wrap.h"


#define EXPECTED_DINODE_ENTRIES_PER_ROW (long)77
#define EXPECTED_DINODE_SIZE            (long)128
#define EXPECTED_DINODE_ROWS            (long)2


static void dbscan_init( SCL *scl, di_state_t *state, md_id_t dir_id,
			  int read_ahead);
static int dbscan_test( SCL *scl, di_state_t *state, md_id_t dir_id,
			 md_dir_item **dir_item_p, int *err_counter_p);
static void dbscan_incr( SCL *scl, di_state_t *state, md_id_t dir_id);
static void dbscan_clean( di_state_t *state);



/**
 *  Public function to alter a directory iterator function-set so it
 *  will use the ReadDir methods
 */
void dump_di_dbscan( di_fn_t *fn_set)
{
  fn_set->init = dbscan_init;
  fn_set->test = dbscan_test;
  fn_set->incr = dbscan_incr;
  fn_set->clean = dbscan_clean;
}


/**
 *  Initialise our access
 */
void dbscan_init( SCL *scl, di_state_t *state, md_id_t dir_id,
		  int read_ahead)
{
  md_dir_inode *dir_inode;
  md_head *dir_head;
  long i;

  state->dbscan.error = 0;
  state->dbscan.end_of_list = 0;
  state->dbscan.hash_handle = NULL;
  state->dbscan.max_hash_handle = 0;
  state->dbscan.hh_idx = 0;
  state->dbscan.de_idx = 0;

  if( myGetRecord( scl, dir_id,  &state->dbscan.dir_inode) < 0) {
    fprintf( stderr, "Skipping directory: failed to fetch dir_inode\n");
    fprintf( stderr, "\tdir inode: %s\n", md2PrintID( dir_id));
    goto err_exit;
  }

  dir_head = &state->dbscan.dir_inode.head;
  if( !mdIsType( dir_head->type, mdtDirectory | mdtInode)) {
    fprintf( stderr, "Skipping directory: dir_inode has wrong type\n");
    fprintf( stderr, "\tdir inode: %s\n", md2PrintID( dir_id));
    fprintf( stderr, "\ttype: %s\n", md2PrintTypes( dir_head->type));
    goto err_exit;
  }


  dir_inode = &state->dbscan.dir_inode.body.dirInode;
  if( dir_inode->hashInfo.entriesPerRow != EXPECTED_DINODE_ENTRIES_PER_ROW ||
      dir_inode->hashInfo.size != EXPECTED_DINODE_SIZE ||
      dir_inode->hashInfo.rows != EXPECTED_DINODE_ROWS) {
    fprintf( stderr, "WARNING: directory inode has wrong values in header\n");
    fprintf( stderr, "\tdir inode: %s\n", md2PrintID( dir_id));
    if( dir_inode->hashInfo.entriesPerRow != EXPECTED_DINODE_ENTRIES_PER_ROW)
      fprintf( stderr, "\tentriesPerRow: %lu != %lu\n",
	       dir_inode->hashInfo.entriesPerRow,
	       EXPECTED_DINODE_ENTRIES_PER_ROW);
    if( dir_inode->hashInfo.size != EXPECTED_DINODE_SIZE)
      fprintf( stderr, "\tsize: %lu != %lu\n",
	       dir_inode->hashInfo.size,
	       EXPECTED_DINODE_SIZE);
    if( dir_inode->hashInfo.rows != EXPECTED_DINODE_ROWS)
      fprintf( stderr, "\trows: %lu != %lu\n",
	       dir_inode->hashInfo.rows,
	       EXPECTED_DINODE_ROWS);
  }


  // Alloc more memory, if necessary
  if( dir_inode->hashInfo.rows > state->dbscan.max_hash_handle) {
    state->dbscan.max_hash_handle = dir_inode->hashInfo.rows;
    state->dbscan.hash_handle = realloc( state->dbscan.hash_handle,
					 dir_inode->hashInfo.rows * sizeof(md_record));

    if( !state->dbscan.hash_handle) {
      fprintf( stderr, "Out of memory: aborting directory list\n");
      goto err_exit;
    }
  }


  // Attempt to load each of the hash rows.
  for( i = 0; i < dir_inode->hashInfo.rows; i++) {

    if( myGetRecord( scl, dir_inode->hashHandle [i],  &state->dbscan.hash_handle[i]) < 0) {
      fprintf( stderr, "Skipping directory: failed to fetch hash handle %lu (of %lu)\n",
	       i+1, dir_inode->hashInfo.rows);
      fprintf( stderr, "\tdir inode: %s\n", md2PrintID( dir_id));
      fprintf( stderr, "\thash node: %s\n", md2PrintID( dir_inode->hashHandle [i]));
      goto err_exit;
    }

    if( !mdIsType( state->dbscan.hash_handle [i].head.type, mdtHash)) {
      fprintf( stderr, "Skipping directory: hash handle has wrong type\n");
      fprintf( stderr, "\tdir inode: %s\n", md2PrintID( dir_id));
      fprintf( stderr, "\thash node: %s\n", md2PrintID( dir_inode->hashHandle [i]));
      fprintf( stderr, "\ttype: %s\n", md2PrintTypes( state->dbscan.hash_handle [i].head.type));
      goto err_exit;
    }    
  }


  /* Initialise pointers to look at hash-1, hash-idx-1, LL-idx-1 */

  /* Call fn that finds first non-empty hash entry, including current posn */

  return;

 err_exit:
  state->dbscan.error = 1;
}


/**
 *  Return 1 if there is at least one more directory item to read, 0
 *  otherwise.
 *
 *  If 1 is returned, *dir_item_p should point to the md_dir_item
 *  structure for the next directory item.
 */
int dbscan_test( SCL *scl, di_state_t *state, md_id_t dir_id,
		 md_dir_item **dir_item_p, int *err_counter_p)
{

  if( state->dbscan.end_of_list || state->dbscan.error) {
    dbscan_clean( state);
    return 0;
  }

  /* update dir_item_p */

  // Normally return 1
  return 0;
}


/**
 *  Called after dir_item has been processed.
 */
void dbscan_incr( SCL *scl, di_state_t *state, md_id_t dir_id)
{
  /* Call fn that finds first non-empty hash entry, excluding current posn */
}


/**
 *  Make be called explicitly to clean the state.  Should be safe
 *  against multiple calls.
 */
void dbscan_clean( di_state_t *state)
{
  if( state->dbscan.hash_handle) {
    free( state->dbscan.hash_handle);
    state->dbscan.hash_handle = NULL;
  }

  state->dbscan.max_hash_handle = 0;
}




/**
 *  Scan, starting at the current point, for a non-empty inode entry.
 *
 *  If try_current is 1 then the current position is considered,
 *  otherwise the current entry is ignored and the next non-empty
 *  inode entry is selected.
 *
 *  If no (further) entries are found then state->dbscan.end_of_list
 *  is set.
 */
void scan_for_dir_entry( di_state_t *state, int try_current)
{


}
