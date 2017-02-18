/**
 *  Definitions common between different directory
 *  iterators.
 */
#ifndef __DUMP_DI_H__
#define __DUMP_DI_H__


/**
 *  These are all needed to satisfy usage of SCL, md_id_t and
 *  reqExtItem.
 */
#include <time.h>
#include <stdio.h>
#include "md2types.h"
#include "sdef.h"
#include "shmcom.h"
#include "sclib.h"

/**
 *  Include definitions of the different DI implementation's state
 */
#include "dump_di_readdir_state.h"
#include "dump_di_dbscan_state.h"


/**
 *  The combined state.  This should be allocated and have extent for
 *  the duration of the DI run.
 */
typedef union {
  dump_di_readdir_state_t readdir;
  dump_di_dbscan_state_t  dbscan;
} di_state_t;



/**
 *   The macro that expands to our directory iterator.
 *
 *   The canonical usage:
 *
 *     di_fn_t fns;
 *     di_state_t state;
 *     md_dir_item *dir_item;
 *
 *     dump_di_readdir( &fns);
 *
 *     DI_ITERATE( fns, SCL, state, dir_id, 10, dir_item,
 *                 err_count) {
 *
 *         // process dir_item
 *
 *         if( problem) {
 *             DI_FINALLY( fns, state);
 *             break;
 *         }
 *     }
 */
#define DI_ITERATE(DI,SCL,DI_STATE,DIR_ID,READ_AHEAD,			\
		   DIR_ITEM, ERR_COUNTER)				\
  for( (DI).init((SCL), &(DI_STATE), (DIR_ID), (READ_AHEAD));		\
       (DI).test((SCL), &(DI_STATE), (DIR_ID), &(DIR_ITEM),		\
		 &(ERR_COUNTER));					\
       (DI).incr((SCL), &(DI_STATE), (DIR_ID)))

#define DI_FINALLY(DI,DI_STATE) (DI).clean(&(DI_STATE))





/**
 *  The function-set to implement the Directory Iterator.
 */
typedef struct {
  void (*init)( SCL *scl, di_state_t *state, md_id_t dir_id, int read_ahead);
  int (*test)( SCL *scl, di_state_t *state, md_id_t dir_id,
	       md_dir_item **dir_item_p, int *err_counter_p);
  void (*incr)( SCL *scl, di_state_t *state, md_id_t dir_id);
  void (*clean)( di_state_t *state);
} di_fn_t;


/**
 *  Read in the function prototypes.  This is to save dump_main
 *  calling them explicitly.
 */
#include "dump_di_readdir.h"
#include "dump_di_dbscan.h"

#endif
