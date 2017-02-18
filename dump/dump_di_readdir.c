/**
 *  Implement a Directory Iterator using PNFS' ReadDir support
 */
#include <stdlib.h>
#include <string.h>  // needed by setMaxAuth()

#include "dump_di_readdir.h"
#include "dump_wrap.h"


static void readdir_init( SCL *scl, di_state_t *state, md_id_t dir_id,
			  int read_ahead);
static int readdir_test( SCL *scl, di_state_t *state, md_id_t dir_id,
			 md_dir_item **dir_item_p, int *err_counter_p);
static void readdir_incr( SCL *scl, di_state_t *state, md_id_t dir_id);
static void readdir_clean( di_state_t *state);



/**
 *  Public function to alter a directory iterator function-set so it
 *  will use the ReadDir methods
 */
void dump_di_readdir( di_fn_t *fn_set)
{
  fn_set->init = readdir_init;
  fn_set->test = readdir_test;
  fn_set->incr = readdir_incr;
  fn_set->clean = readdir_clean;
}


void readdir_clean( di_state_t *state)
{
  if( state->readdir.extItem) {
    free( state->readdir.extItem);
    state->readdir.extItem = NULL;
  }
}




/**
 *  Initialise the state
 */
void readdir_init( SCL *scl, di_state_t *state, md_id_t dir_id, int read_ahead)
{
  state->readdir.count = 0;
  state->readdir.current = 0;

  state->readdir.expect_final_read=0;
  state->readdir.read_ahead = read_ahead;
  
  state->readdir.extItem = malloc( read_ahead * sizeof( reqExtItem));
}



/**
 *  Test whether we should continue iterating.  Return 0 if there are
 *  no more entries, 1 otherwise.
 *
 *  If there are more entries, the next entry should be loaded into
 *  the variables.
 *
 */
int readdir_test( SCL *scl, di_state_t *state, md_id_t dir_id,
		  md_dir_item **dir_item_p, int *err_counter_p)
{
  mdPermission perm = {0,0};
  md_auth auth;
  long cookie;

  if( !state->readdir.extItem)
    goto should_stop;


  /* If we have run out of data, read in the next batch */
  if( state->readdir.current >= state->readdir.count) {
    setMaxAuth( &auth);

    /**
     *   Obtain the cookie value.  NB count == 0 can only happens for
     *   initial read.
     */
    cookie = state->readdir.count == 0 ? 0 : state->readdir.extItem[state->readdir.count-1].cookie;

  
    state->readdir.count = myReadDirAuth( scl, &auth, dir_id, perm, cookie,
					  state->readdir.read_ahead,
					  state->readdir.extItem);

    // This is really here just to emit a warning if we have a short read
    if( state->readdir.expect_final_read && state->readdir.count > 0) {
      fprintf( stderr, "ASSERT: expected final read is non-zero.\n");
      fprintf( stderr, "\tread %u entries\n", state->readdir.count);
      fprintf( stderr, "\tcookie: 0x%08x\n", (unsigned int) cookie);
      if( state->readdir.count < state->readdir.read_ahead)
	fprintf( stderr, "\t(next read should be final)\n");
      (*err_counter_p)++;
    }

    if( state->readdir.count <= 0) {

      // Emit a error message if there was an error
      if( state->readdir.count < 0) {
	fprintf( stderr, "Aboring readdir list due to myReadDirAuth() returning error\n");
	fprintf( stderr, "\tdir inode: %s\n", md2PrintID( dir_id));
	fprintf( stderr, "\tcookie: 0x%08x\n", (unsigned int) cookie);
	(*err_counter_p)++;
      }

      goto should_stop;
    }

    state->readdir.current = 0;

    // If we read less than read_ahead then the next read should be zero-length
    state->readdir.expect_final_read = (state->readdir.count < state->readdir.read_ahead) ? 1 : 0;
  }

  *dir_item_p = &state->readdir.extItem [state->readdir.current++].item;

  return 1;

 should_stop:
  *dir_item_p = NULL;
  readdir_clean( state);
  return 0;
}


/**
 *  Clean up resources used for the entry.  Update cookie ready for
 *  the next item.
 */
void readdir_incr( SCL *scl, di_state_t *state, md_id_t dir_id)
{
}


