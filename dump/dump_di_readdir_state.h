#ifndef __DUMP_DI_READDIR_STATE_H__
#define __DUMP_DI_READDIR_STATE_H__

/**
 *  The state when using the ReadDir method.
 */
typedef struct {

  int expect_final_read;

  long cookie;
  int count;
  int current;
  int read_ahead;

  reqExtItem     *extItem;

} dump_di_readdir_state_t;


#endif
