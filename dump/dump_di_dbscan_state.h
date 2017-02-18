#ifndef __DUMP_DI_DBSCAN_STATE_H__
#define __DUMP_DI_DBSCAN_STATE_H__

/**
 *  The state when using the DB Scan method.
 */
typedef struct {

  /* state goes here */
  md_record dir_inode;

  /* The array of hash-row records */
  md_record *hash_handle;

  /* Current maximum number of records in dir_hash_row */
  int max_hash_handle;

  /* Is 0 if everything is OK, 1 if there's some error requiring no further processing */
  int error;

  /* Is 0 if we have more entries to process; 1 if there are no more entries to process */
  int end_of_list;

  /* Our indexes */
  int hh_idx, de_idx;
  md_record dir_entry;


} dump_di_dbscan_state_t;


#endif
