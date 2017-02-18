/**
 *  Main code to support walking a set of PNFS databases.
 */
#include <sys/time.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>


#include "md2types.h" // TODO: should this be something else?
#include "shmcom.h"

// From dbserver
#include "sdef.h"
#include "sclib.h"

// Support for directory iterators
#include "dump_di.h"

#include "dump_wrap.h"
#include "dump_cache.h"
#include "dump_tag.h"
#include "dump_output.h"
#include "dump_intree.h"
#include "dump_compat.h"
#include "dump_version.h"
#include "dump_utils.h"
#include "parse_lvl2.h"



#define N_FILE_LEVELS 8
#define N_LEVELS_VERBOSITY 3

// The level in which some basic file metadata is stored.
#define FILE_LEVEL_BASIC_METADATA 2

/* This is probably good for almost every PNFS instance */
#define DEFAULT_SHMKEY_VALUE 0x1122

#define READ_AHEAD_DEFAULT    4
#define READ_AHEAD_MAX_VALUE  2000
#define READ_AHEAD_MIN_VALUE  1

#define SECONDS_IN_HOUR    3600
#define SECONDS_IN_MINUTE    60

#define FREOPEN64_STR  XSTR(FREOPEN64)

/*  These characters must be escaped when representing the command-line */
#define MUST_ESCAPE_CHARACTERS " $"


/**
 *  We record some statistics about how many entries we have
 *  processed.
 */
typedef struct {
  int nDir;
  int nFile;
  int nUnknown;
  int nSkipped;
  int nError;
} stats_t;



/**
 *  Big, ugly globals
 */
stats_t stats_current, stats_last;
struct timeval started_at, next_periodic_info, periodicity = {10,0};
static int verbosity;
static int should_be_paranoid;
static int read_ahead = READ_AHEAD_DEFAULT;
static lvl2_info_t lvl2_info;
static di_fn_t di_primary;


// Ugly: needed for sig-handler
SCL *scl_global = NULL;

int sclClientClose( SCL *scl ); // TODO: should be in header file.


// Local prototypes.
void handle_sig_shutdown( int signal);
void setup_signals();
void emit_freq( const char *label, int elapsed_time, stats_t *old);
void list_available_serialisers();
int parse_args( int argc, char *argv[], dump_serialiser_t **serialiser_p,
		int *shmkey_p, int *ser_argc_p, char **ser_argv_p[]);
static char *build_cmdline( int argc, char *argv[]);
void print_help();
void emit_final_numbers();
void test_for_periodic_info();
void emit_periodic_info();

int have_file_lvl2( void);
ssize_t get_file_level( SCL *scl, md_id_t id, md_file_inode *finode,
			int level, char **buffer_p);

int acquire_file_lvl2( SCL *scl, md_id_t file_id, md_file_inode *finode,
		       const char *filename);
void flush_file_lvl2( void);


int process_file_levels( SCL *scl, dump_serialiser_t *serialiser,
			  md_record *file_record);


void process( SCL *scl, dump_serialiser_t *serialiser);

int scan_directory( SCL *scl, md_id_t parent, md_id_t id,
		     dump_serialiser_t *serialiser);
int process_dir_item( SCL *scl, dump_serialiser_t *serialiser,
		       md_id_t parent, md_dir_item *item, md_record *record);

int process_dir( SCL *scl, dump_serialiser_t *serialiser, md_id_t parent,
		 md_dir_item *item, md_record *record);
int process_file( SCL *scl, dump_serialiser_t *serialiser, md_id_t parent,
		  md_dir_item *item, md_record *record);
int process_link( SCL *scl, dump_serialiser_t *serialiser, md_id_t parent,
		  md_dir_item *item, md_record *record);

static int scan_all_tags( SCL *scl, dump_serialiser_t *serialiser, md_id_t dir_id,
			  md_dir_inode *dinode);
static int process_tag( SCL *scl, dump_serialiser_t *serialiser, md_record *this_tag,
			md_id_t dir_id);
static int process_primary_tag( dump_serialiser_t *serialiser, md_id_t dir_id,
				md_id_t tag_id, char *tag_name, char *tag_data,
				size_t tag_data_len, md_attr *tag_attr);
static int process_pseudo_primary_tag( dump_serialiser_t *serialiser, md_id_t dir_id,
				       md_id_t parent_tag_id, md_id_t tag_id,
				       const char *tag_name, const char *tag_data,
				       size_t tag_data_len, md_attr *tag_attr);
static int process_invalid_tag( dump_serialiser_t *serialiser, md_id_t dir_id,
				md_id_t parent_tag_id, md_id_t tag_id, const char *tag_name);
static int process_inherited_tag( dump_serialiser_t *serialiser, md_id_t dir_id,
				  md_id_t parent_tag_id, md_id_t tag_id, const char *tag_name);


ssize_t append_line( char *storage, size_t storage_size, const char *text);
inline static void split_time( struct timeval tm, int *h, int *m, double *s);




/**
 *  It starts...
 */
int main( int argc, char *argv[])
{
  int rc, ser_argc, shmkey = DEFAULT_SHMKEY_VALUE;
  char *error, **ser_argv;
  dump_serialiser_t *serialiser;
  ser_return_t ser_rc;
  char *cmdline;
  
  // Our default Directory Iterators
  dump_di_readdir( &di_primary);
  //dump_di_dbscan( &di_primary);

  if( parse_args( argc, argv, &serialiser, &shmkey, &ser_argc, &ser_argv) < 0)
    return 1;

  cmdline = build_cmdline( argc, argv);

  ser_rc = CALL_SERIALISER(serialiser, begin_serialisation, (cmdline, ser_argc, ser_argv));
  
  free( cmdline);

  switch( ser_rc) {

  case ser_error:
    return 1;

  case ser_skip:
    // Do nothing...
    break;

  case ser_ok:
    if (!(scl_global = sclClientOpen(shmkey, 12 * 1024, &rc))) {
      error = sclError("sclClientOpen", rc);
      fprintf(stderr, " Problem : %s(%d)\n", error, errno);
      if( errno > 0)
	perror( "shmget");
      
      goto err_exit;
    }

    gettimeofday( &started_at, NULL);
    timeradd( &started_at, &periodicity, &next_periodic_info);
    cache_register_scl( scl_global);
    
    setup_signals();

    process( scl_global, serialiser);
    
    cache_register_scl( NULL);
    sclClientClose( scl_global);
    break;
  }

  if( CALL_SERIALISER(serialiser, end_serialisation, ()) == ser_error)
    goto err_exit;

  if( verbosity >= 1)
  	emit_final_numbers();

  lvl2_final_flush( &lvl2_info);


  return 0;

 err_exit:
  CALL_SERIALISER(serialiser, end_serialisation, ());
  return 1;
}


/**
 *  Reconstruct the command-line used to invoke pnfsDump.
 */
char *build_cmdline( int argc, char *argv[])
{
  char *cmdline;
  size_t cmdline_len, argv_len;
  int i;
  unsigned char *escaped;

  escaped = malloc( argc);

  if( !escaped) {
    fprintf( stderr, "out of memory.\n");
    return NULL;
  }

  /* The space character between each element */
  cmdline_len = argc;

  /* The space needed by each element */
  for( i = 0; i < argc; i++) {
    argv_len = strlen( argv[i]);

    cmdline_len += argv_len;
    
    if( strcspn( argv[i], MUST_ESCAPE_CHARACTERS) == argv_len) {
      escaped [i] = 0;
    } else {
      escaped [i] = 1;
      /* Add space needed for two single-quote characters */
      cmdline_len += 2;
    }
  }

  cmdline = malloc( cmdline_len+1); // +1 for terminating '\0'

  if( !cmdline) {
    fprintf( stderr, "out of memory.\n");
    free( escaped);
    return NULL;
  }

  cmdline[0] = '\0';

  for( i = 0; i < argc; i++) {
    if( escaped [i])
      strcat( cmdline, "'");

    strcat( cmdline, argv[i]);

    if( escaped [i])
      strcat( cmdline, "'");

    if( i < argc-1)
      strcat( cmdline, " ");
  }

  free( escaped);
  return cmdline;
}



/**
 *  Parse the command-line arguments.
 *
 *  Returns 0 if everything is OK, -1 if the arguments are in some way
 *  wrong.
 */
int parse_args( int argc, char *argv[], dump_serialiser_t **serialiser_p,
		int *shmkey_p, int *ser_argc_p, char **ser_argv_p[])
{
  long delay;
  dump_serialiser_t *ser;
  char *ser_name, *end_of_integer;
  int c, max_retries;
  md_id_t subtree_root;

  /**
   *  Parse optional items
   */
  opterr = 0;
  while( (c = getopt( argc, argv, "+:s:r:o:d:a:m:hvVcp")) != -1) {
    switch( c) {
    case 'r':
      if( dump_parse_hex( optarg, 24) == -1) {
       fprintf( stderr, "Invalid ID %s\n", optarg);
       return -1;
      }
      md2ScanId( optarg, &subtree_root);
      insubtree_register_sub_root( subtree_root);
      break;

    case 's':
      *shmkey_p = dump_parse_hex( optarg, 0);

      if( *shmkey_p == -1) {
	fprintf( stderr, "Invalid hex. number: %s\n", argv[optind]);
	return -1;
      }
      break;

    case 'c':
      cache_disable();
      break;

    case 'm':
      max_retries = atoi( optarg);
      if( max_retries <= 0) {
	fprintf( stderr, "Invalid maximum number of retries.\n");
	return -1;
      }

      myGet_register_max_retries( max_retries);
      break;

    case 'd':
      delay = strtol( optarg, &end_of_integer, 10);

      if( end_of_integer == optarg) {
	fprintf( stderr, "Argument to -d must be an integer\n");
	return -1;
      } else if( *end_of_integer != '\0') {
	fprintf( stderr, "Ignoring trailing characters (\"%s\") in delay option.\n",
		 end_of_integer);
      }

      if( delay < 0) {
	fprintf( stderr, "Delay must be zero or greater\n");
	return -1;
      }

      if ( delay == 0)
	fprintf( stderr, "WARNING: a zero delay may interfere with NFS operations and should not be used on a production system.\n");

      myGet_register_delay( delay);
      break;

    case 'o':
      if( FREOPEN64( optarg, "w", stdout) == NULL) {
	perror( FREOPEN64_STR);
	fprintf( stderr, "Failed to open file \"%s\" for writing\n", optarg);
	return -1;
      }
      break;

    case 'v':
      if( verbosity < N_LEVELS_VERBOSITY)
	verbosity++;
      else
	fprintf( stderr, "Ignoring attempt to exceed maximum verbosity.\n");
      break;

    case 'V':
      fprintf( stderr, "Version: " PNFSDUMP_VERSION_STR "\n");
      return -1;


    case 'p':
      should_be_paranoid ^= 1;
      break;

      
    case 'a':
      read_ahead = strtol( optarg, &end_of_integer, 10);

      if( end_of_integer == optarg) {
	fprintf( stderr, "Argument to -a must be an integer\n");
	return -1;
      } else if( *end_of_integer != '\0') {
	fprintf( stderr, "Ignoring trailing characters (\"%s\") in read-ahead option.\n",
		 end_of_integer);
      }

      if( read_ahead < READ_AHEAD_MIN_VALUE) {
	fprintf( stderr, "Read-ahead must be %u or greater\n",
		 READ_AHEAD_MIN_VALUE);
	return -1;
      }

      if( read_ahead > READ_AHEAD_MAX_VALUE) {
	fprintf( stderr, "Read-ahead must be %u or less\n",
		 READ_AHEAD_MAX_VALUE);
	return -1;
      }
      break;

    case ':':
      fprintf( stderr, "Option -%c requires an argument\n", optopt);
      return -1;

    case 'h':
      print_help( argv[0]);
      return -1;

    case '?':
      fprintf( stderr, "Unknown option: -%c\n", optopt);
      print_help( argv[0]);
      return -1;
    }
  }

  if( argc - optind < 1) {
    print_help( argv[0]);
    return -1;
  }

  ser_name = argv[optind];
  if( !(ser = pnfs_serialisers_find( ser_name))) {
    fprintf( stderr, "Did not find output %s\n", ser_name);
    list_available_serialisers();
    return -1;
  }

  /**
   *  Build return variables.
   */
  *ser_argv_p = &argv[optind];
  *ser_argc_p = argc - (optind);
  *serialiser_p = ser;

  return 0;
}



/**
 *  Display a standard "helpful" message.
 */
void print_help( const char *cmd_name)
{
  fprintf( stderr, "%s [-r <root pnfsId>] [-o <output file>] [-d <delay>] [-a <read-ahead>] [-s <shm key>] [-m <max retries>] [-p] [-c] [-v [-v [-v]]] <output format> [<option #1> ...]\n",
	   cmd_name);
  fprintf( stderr, "\n");
  fprintf( stderr, "The -d option specifies how many milliseconds to delay every 100 database\n");
  fprintf( stderr, "operations.  By default, -d %d is used.\n", MYGET_DEFAULT_DELAY); 
  fprintf( stderr, "\n");
  fprintf( stderr, "By default, pnfsDump will retry indefinitely if a PNFS database operation\n");
  fprintf( stderr, "times out.  The -m option will tell pnfsDump to fail such a database\n");
  fprintf( stderr, "operation after <max retries> attempts.\n");
  fprintf( stderr, "\n");
  list_available_serialisers();
}


void setup_signals()
{
  struct sigaction newAction;

  memset( (char *)&newAction , 0 , sizeof(newAction) ) ;
  newAction.sa_handler = handle_sig_shutdown;

  if( sigaction( SIGTERM, &newAction, NULL) < 0)
    perror( "sigaction");

  if( sigaction( SIGHUP, &newAction, NULL) < 0)
    perror( "sigaction");
  
  if( sigaction( SIGINT, &newAction, NULL) < 0)
    perror( "sigaction");

  if( sigaction( SIGQUIT, &newAction, NULL) < 0)
    perror( "sigaction");

  if( sigaction( SIGUSR1, &newAction, NULL) < 0)
    perror( "sigaction");
}




/**
 *  Top-level processing of tree
 */
void process( SCL *scl, dump_serialiser_t *serialiser)
{
  ser_return_t ser_rc;
  md_id_t root_id, top_id;
  static md_id_t null_id;
  int rc;
  md_record rec;

  rc = myGetRootId( scl , 0, &root_id);

  if( rc != 0) {
    stats_current.nError++;
    return;
  }

  insubtree_register_root( root_id);

  top_id = insubtree_get_sub_root();

  if( mdIsNullID( top_id))
    memcpy( &top_id, &root_id, sizeof( md_id_t));

  // Obtain the dir_inode.
  if( myGetRecord( scl, top_id, &rec) != 0) {
    fprintf( stderr, "Cannot obtain root record\n");
    stats_current.nError++;
    return;
  }

  if( !mdIsType(rec.head.type, mdtDirectory | mdtInode)) {
    fprintf( stderr, "root record not a directory\n");
    stats_current.nError++;
    return;
  }


  cache_dir_enter( top_id, null_id);


  ser_rc = CALL_SERIALISER(serialiser, begin_dump, (top_id));

  if( ser_rc == ser_ok)
    rc = scan_all_tags( scl, serialiser, top_id, &rec.body.dirInode);

  if( ser_rc == ser_ok && rc != -1)
    scan_directory( scl, top_id, top_id, serialiser);

  CALL_SERIALISER(serialiser, end_dump, (top_id));

  cache_dir_leave( top_id);

  // Free up malloc-ed data.
  get_file_level( NULL, top_id, NULL, 0, NULL);
}



int scan_directory( SCL *scl, md_id_t parent, md_id_t dirId,
		    dump_serialiser_t *serialiser)
{
  static unsigned long poll_counter;

  md_dir_item    *dirItem;
  mdRecord       rec;
  di_state_t     state;

  DI_ITERATE( di_primary, scl, state, dirId, read_ahead, dirItem,
	      stats_current.nError) {

    // Send rate output (to stderr) periodically.
    if( poll_counter++ % 20)
      test_for_periodic_info();

    // Err... lets just ignore these ones
    if( !strcmp( dirItem->name, ".") || !strcmp( dirItem->name, ".."))
      continue;

    if( myGetRecord( scl, dirItem->ID,  &rec) != 0) {
      fprintf( stderr, "Failed to obtain inode record; item will be skipped.\n");
      fprintf( stderr, "\titem inode: %s\n", md2PrintID( dirItem->ID));
      fprintf( stderr, "\titem name: %s\n", dirItem->name);
      fprintf( stderr, "\tparent directory inode: %s\n", md2PrintID( dirId));
      stats_current.nSkipped++;
      continue;
    }

    if( process_dir_item( scl, serialiser, parent, dirItem, &rec) < 0) {
      DI_FINALLY( di_primary, state);
      return -1;
    }
  }

  return 0;
}



/**
 *  Process a directory item.
 *
 *  Returns 0 if serialisation should continue, -1 if the
 *  serialisation should terminate.
 */
int process_dir_item( SCL *scl, dump_serialiser_t *serialiser,
		      md_id_t parent, md_dir_item *item, md_record *record)
{
  int rc;

  /**
   *  Type is really a bit-field, with various flags that may be
   *  set.  However, in practise, only a few possible combinations
   *  are encountered.  Therefore, we treat it as a list of
   *  possibles.
   */
  switch( mdShowType(record->head.type)) {

  case mdtInode|mdtDirectory:               // Directory
    rc = process_dir( scl, serialiser, parent, item, record);
    break;


  case mdtForceIO |mdtRegular| mdtInode:    // Wormhole target.
  case mdtInode|mdtRegular:                 // Regular file.
    rc = process_file( scl, serialiser, parent, item, record);
    break;
 

  case mdtLink | mdtInode:                  // Link
    rc = process_link( scl, serialiser, parent, item, record);
    break;


  default:
    printf( "entry %s (%s) with unexpected type 0x%03lx\n",
	    item->name,
	    mdStringID( item->ID),
	    mdShowType( record->head.type));
    rc = stats_current.nUnknown++;
    break;
  }

  return rc;
}



/**
 *  Process a link
 */
int process_link( SCL *scl, dump_serialiser_t *serialiser, md_id_t parent,
		  md_dir_item *item, md_record *record)
{
  md_file_inode *finode = &record->body.fileInode;
  md_attr  *attr;
  ssize_t count;
  char *data;
  ser_return_t rc = ser_ok;

  stats_current.nFile++;

  /**
   *  We extract *only* level zero.  Is this correct?
   */

  attr = &finode->attr [0];

  /* Some consistency checking... */
  if( attr->entries != attr->unixAttr.mst_size) {
    fprintf( stderr, "Inconsistency in size of level 0: %lu != %lu\n",
	     attr->entries, attr->unixAttr.mst_size);
  }    

  count = get_file_level( scl, record->head.ID, finode, 0, &data);

  if( count == attr->entries)
    rc = CALL_SERIALISER(serialiser, new_link, (parent, record->head.ID, item->name,
						attr, data, count));

  return rc != ser_error ? 0 : -1;
}




/**
 *  Process a new directory.
 */
int process_dir( SCL *scl, dump_serialiser_t *serialiser, md_id_t parent,
		 md_dir_item *item, md_record *record)
{
  md_dir_inode *dinode = &record->body.dirInode;
  ser_return_t ser_rc1, ser_rc2;
  int rc = 0;

  stats_current.nDir++;

  cache_dir_enter( item->ID, parent);

  ser_rc1 = CALL_SERIALISER(serialiser, begin_dir, (parent, item->ID, item->name,
						    &dinode->attr));

  switch( ser_rc1) {
    
  case ser_error:
    return -1;

  case ser_skip:
    // Do nothing
    break;

  case ser_ok:
    rc = scan_all_tags( scl, serialiser, record->head.ID, dinode);

    // Walk into the (sub-)directory using recursion.
    if( rc != -1)
      rc = scan_directory( scl, record->head.ID, item->ID, serialiser);
    break;
  }

  cache_dir_leave( item->ID);

  ser_rc2 = CALL_SERIALISER(serialiser, end_dir, (parent, item->ID, item->name));

  if( ser_rc2 == ser_error)
    rc = -1;
    
  return rc;
}


/**
 *  Return 0 if OK, -1 if there's a problem.
 */
int scan_all_tags( SCL *scl, dump_serialiser_t *serialiser, md_id_t dir_id,
		   md_dir_inode *dinode)
{
  md_id_t this_tag_id;
  md_record this_tag_rec;
  int rc;

  /* If the serialiser isn't interested in any tags, don't scan them. */
  if( !SERIALISER_HAS_FN( serialiser, new_primary_tag) &&
      !SERIALISER_HAS_FN( serialiser, new_pseudo_primary_tag) &&
      !SERIALISER_HAS_FN( serialiser, new_invalid_tag) &&
      !SERIALISER_HAS_FN( serialiser, new_inherited_tag))
    return 0;


  /* Iterate over all tags for this directory. */
  for( this_tag_id = dinode->attr.tag;
       !mdIsNullID( this_tag_id);
       this_tag_id = this_tag_rec.head.nextID) {

    rc = myGetRecord( scl, this_tag_id, &this_tag_rec);
  
    if( rc != 0) {
      fprintf( stderr, "Failed to obtain record for a tag\n");
      fprintf( stderr, "\ttag id: %s\n", md2PrintID( this_tag_id));
      fprintf( stderr, "\tdirectory inode: %s\n", md2PrintID( dir_id));

      /**
       * If there was a problem with this tag, we would like to
       * continue; however, without the md_record, we can't.
       */
      break;
    }

    if( process_tag( scl, serialiser, &this_tag_rec, dir_id) == -1)
      return -1;
  }

  return 0;
}


/**
 *  Process a tag.  The tag is, at this stage "raw", representing the
 *  tag on the database.  If we are doing a partial dump then we wish
 *  to promote any tag with inheritance (inherited and pseudo-primary
 *  tags) to be primary tags.
 *
 *  Returns 0 on success, -1 if the serialiser wants to quit.
 */
int process_tag( SCL *scl, dump_serialiser_t *serialiser, md_record *this_tag,
		 md_id_t dir_id)
{
  md_tag_inode *tinode = &this_tag->body.tagInode;
  tag_type_t type;
  char storage[TAG_DATA_SIZE];
  size_t storage_length;
  int is_orphaned;

  
  if( !tag_identify_type( this_tag, &type)) {
    fprintf( stderr, "failed to process tag: %s\n", mdStringID( this_tag->head.ID));
    return 0;
  }

  if( !mdIsNullID( this_tag->head.parentID))
    is_orphaned = !insubtree_tag_within_subtree( this_tag->head.parentID);
  else {
    if( type != PRIMARY)
      fprintf( stderr, "ASSERT failed: tag without parent not PRIMARY.\n");
    is_orphaned = 0;
  }

  switch( type) {

  case PRIMARY:
    return process_primary_tag( serialiser, dir_id, this_tag->head.ID, tinode->name,
				tinode->data, tinode->attr.entries, &tinode->attr);

  case PSEUDO_PRIMARY:
    if( !is_orphaned)
      return process_pseudo_primary_tag( serialiser, dir_id, this_tag->head.parentID,
					 this_tag->head.ID, tinode->name, tinode->data,
					 tinode->attr.entries, &tinode->attr);

    // If we have an orphaned pseudo-primary tag, make it a primary tag.
    return process_primary_tag( serialiser, dir_id, this_tag->head.ID, tinode->name,
				tinode->data, tinode->attr.entries, &tinode->attr);


  case INVALID:
    fprintf( stderr, "Found invalid tag.\n");
    fprintf( stderr, "\tdirectory: %s\n", mdStringID( dir_id));
    fprintf( stderr, "\ttag: %s\n", mdStringID( this_tag->head.ID));
    fprintf( stderr, "\tname: %s\n", tinode->name);

    return process_invalid_tag( serialiser, dir_id, this_tag->head.parentID,
				this_tag->head.ID, tinode->name);


  case INHERITED:
    if( !is_orphaned)
      return process_inherited_tag( serialiser, dir_id, this_tag->head.parentID,
				    this_tag->head.ID, tinode->name);

    /**
     *  Promote inherited tag to primary tag.
     */
    if( cache_find_tag_value( this_tag->head.ID, storage, sizeof( storage),
			      &storage_length))
      return process_primary_tag( serialiser, dir_id, this_tag->head.ID, tinode->name,
				  storage, storage_length, &tinode->attr);

    fprintf( stderr, "Couldn't resolve orphaned, inherited tag.\n");
    fprintf( stderr, "\tdirectory: %s\n", mdStringID( dir_id));
    fprintf( stderr, "\ttag: %s\n", mdStringID( this_tag->head.ID));
    fprintf( stderr, "\tname: %s\n", tinode->name);
    return 0;
  }

  fprintf( stderr, "ASSERT failed: tag type is unknown: %u\n", type);      
  return 0;
}


/**
 *  Process a primary tag.
 *
 *  Returns 0 on success, -1 on failure.
 */
int process_primary_tag( dump_serialiser_t *serialiser, md_id_t dir_id,
			 md_id_t tag_id, char *tag_name, char *tag_data,
			 size_t tag_data_len, md_attr *tag_attr)
{
  ser_return_t ser_rc;
  md_id_t null_id;

  mdSetNullID( null_id);

  cache_store_tag( dir_id, tag_id, null_id, PRIMARY,
		   tag_name, tag_data, tag_data_len);

  ser_rc = CALL_SERIALISER( serialiser, new_primary_tag, (dir_id, tag_id, tag_name,
							  tag_attr, tag_data,
							  tag_data_len));

  return ser_rc != ser_error ? 0 : -1;
}



/**
 *  Process a psudo-primary tag.
 *
 *  Returns 0 on success, -1 on failure.
 */
int process_pseudo_primary_tag( dump_serialiser_t *serialiser, md_id_t dir_id,
				md_id_t parent_tag_id, md_id_t tag_id,
				const char *tag_name, const char *tag_data,
				size_t tag_data_len, md_attr *tag_attr)
{
  ser_return_t ser_rc;

  cache_store_tag( dir_id, tag_id, parent_tag_id, PSEUDO_PRIMARY,
		   tag_name, tag_data, tag_data_len);

  ser_rc = CALL_SERIALISER( serialiser, new_pseudo_primary_tag, (dir_id,
								 parent_tag_id,
								 tag_id,
								 tag_name,
								 tag_attr,
								 tag_data,
								 tag_data_len));
  return ser_rc != ser_error ? 0 : -1;
}


/**
 *  Process an invalid tag.
 *
 *  Returns 0 on success, -1 on failure.
 */
int process_invalid_tag( dump_serialiser_t *serialiser, md_id_t dir_id,
			 md_id_t parent_tag_id, md_id_t tag_id,
			 const char *tag_name)
{
  ser_return_t ser_rc;

  cache_store_tag( dir_id, tag_id, parent_tag_id, INVALID,
		   tag_name, NULL, 0);

  ser_rc = CALL_SERIALISER( serialiser, new_invalid_tag, (dir_id,
							  parent_tag_id,
							  tag_id,
							  tag_name));
  return ser_rc != ser_error ? 0 : -1;
}



/**
 *  Process an inherited tag.
 *
 *  Returns 0 on success, -1 on failure.
 */
int process_inherited_tag( dump_serialiser_t *serialiser, md_id_t dir_id,
			   md_id_t parent_tag_id, md_id_t tag_id,
			   const char *tag_name)
{
  ser_return_t ser_rc;

  cache_store_tag( dir_id, tag_id, parent_tag_id, INHERITED,
		   tag_name, NULL, 0);

  ser_rc = CALL_SERIALISER( serialiser, new_inherited_tag, (dir_id,
							    parent_tag_id,
							    tag_id,
							    tag_name));
  return ser_rc != ser_error ? 0 : -1;
}



/**
 *   Process a file.
 */
int process_file( SCL *scl, dump_serialiser_t *serialiser, md_id_t dir,
		   md_dir_item *item, md_record *record)
{
  md_file_inode *finode = &record->body.fileInode;
  ser_return_t rc1, rc2 = ser_ok;
  filesize_t size;
  lvl2_info_t *level2_info = NULL;

  stats_current.nFile++;


  /**
   *  NFS v2 cannot cope with files with lengths that cannot fit into
   *  a 32-bit unsigned integer; pnfs, however, can.  It stores this
   *  either as two 32-bit parts: mst_size and mst_sizeHigh or (if
   *  MD_VLONG is defined) as a single long long integer.
   *
   *  To cope with this limitation, dCache would use a special
   *  place-holder value for files that are longer than can be stored
   *  in a 32-bit integer.  If a file has a length of '1' then it's
   *  length must be read from the file's Level-2 metadata.
   *
   *  Unfortunately, the pnfs NFSv2 daemon contains a bug where the
   *  high-value part is not always initialised to zero.  Because of
   *  this, we *must* always ignore the high-value part.
   */

#ifdef MD_VLONG
  size  = finode->attr [0].unixAttr.mst_size;
  size &=  ~0UL; // Filter out high-order part;
#else
  size = finode->attr [0].unixAttr.mst_size;
  // Ignore unixAttr.mst_sizeHigh
#endif


  /**
   *  If the file size is the place-holder value, look up the real
   *  filesize from the file's level-2 metadata.
   */
  if( size == 1) {
    if( acquire_file_lvl2( scl, item->ID, finode, item->name))
      size = lvl2_info.length;
    else {
      fprintf( stderr, "Unable to read level-2 metadata:\n");
      fprintf( stderr, "    File:    %s\n", item->name);
      fprintf( stderr, "    PNFSid:  %s\n", mdStringID( item->ID));
      fprintf( stderr, "  Will use size: 1\n");
      size = 1;
    }
  }



  /**
   *  If requested, provide the parsed level-2 information.
   */
  if( serialiser->flags & FLAGS_REQ_LEVEL2)
    if( acquire_file_lvl2( scl, item->ID, finode, item->name))
      level2_info = &lvl2_info;



  /**
   *  Perhaps we should verify the filesize reported by PNFS matches
   *  that stored in the level-2 metadata.
   *
   *  We do this if:
   *     o  we're running in "paranoid mode",
   *     o  we have the data already.
   */
  if( should_be_paranoid || have_file_lvl2()) {

    /**
     * If we have not yet acquired the level-2 metadata and we fail to
     * do so here, we quietly skip this test.
     */
    if( acquire_file_lvl2( scl, item->ID, finode, item->name)) {

      if( lvl2_info.flags & LVL2_FLAG_HAVE_LENGTH && size != lvl2_info.length) {

	/**
	 * If we have an inconsistency, we simply use the size from
	 * PNFS.  This is what dCache does (see ...) and, in practise,
	 * the filesize in level-2 metadata is somewhat unreliable.
	 * Under certain circumstances, dCache will record a filesize
	 * of zero in level-2 metadata.
	 */
	if( verbosity >= 2) {
	  fprintf( stderr, "PNFS file-size does not match Level-2 metadata:\n");
	  fprintf( stderr, "    Filesize: %llu != %llu\n", size, lvl2_info.length);
	  fprintf( stderr, "    File:     %s\n", item->name);
	  fprintf( stderr, "    PNFSid:   %s\n", mdStringID( item->ID));
	  fprintf( stderr, "  Will use size:  %llu\n", size);
	}
      }
    }
  }



  /**
   *  The level-0 attributes correspond to those of the file's
   *  "actual" data.
   */
  rc1 = CALL_SERIALISER(serialiser, begin_file, (dir, item->ID, item->name, size,
						 level2_info, &finode->attr [0]));

  switch( rc1) {
  case ser_error:
    rc2 = ser_error;
    break;

  case ser_ok:
    process_file_levels( scl, serialiser, record);

    rc2 = CALL_SERIALISER(serialiser, end_file, (dir, item->ID, item->name));
    break;

  case ser_skip:
    rc2 = ser_ok;
    break;
  }

  lvl2_flush( &lvl2_info);

  return rc2 != ser_error ? 0 : -1;
}



/**
 *  Try to ensure we have a file's level-2 metadata.  This completes a
 *  lvl2_info_t structure global variable.
 *
 *  Returns 1 on success, 0 on failure.
 */
int acquire_file_lvl2( SCL *scl, md_id_t file_id, md_file_inode *finode,
		       const char *filename)
{
  char *data, file_id_str[25];
  ssize_t count;

  if( have_file_lvl2())
    return 1;  // SUCCESS, cached data is present.

  if( lvl2_info.flags & LVL2_FLAG_CANT_PARSE)
    return 0;  // FAILURE, we've tried to parse the file before but failed.

  count = get_file_level( scl, file_id, finode, FILE_LEVEL_BASIC_METADATA, &data);

  if( count < 0) {
    fprintf( stderr, "get_file_level return %d\n", count);
    return 0;  // FAILURE, cannot fetch lvl2 data for some reason.
  }

  if( count == 0)
    return 0;  // FAILURE, the file has no level-2 data.

  // Build ID as a string:
  mdStringID_r( file_id, file_id_str);

  if( !lvl2_parse_data( &lvl2_info, data, count, file_id_str)) {
    lvl2_info.flags |= LVL2_FLAG_CANT_PARSE;

    // NB: lvl2_parse_data will have emitted a single line explaining the failure.
    fprintf( stderr, "    File:    %s\n", filename);
    fprintf( stderr, "    PNFSid:  %s\n", mdStringID( file_id));
    return 0;  // FAILURE
  }
 
  return 1; // SUCCESS
}



/**
 *  Have we got the parsed level-2 metadata in memory?
 *
 *  Return 1 if we do, 0 if we don't.
 */
int have_file_lvl2()
{
  return lvl2_info.data != NULL ? 1 : 0;
}





/**
 *  Process a file's levels, emitting the correct call-backs with
 *  data.
 *
 *  Returns 0 if everything is OK, -1 if there's an error.
 */
int process_file_levels( SCL *scl, dump_serialiser_t *serialiser,
			 md_record *file_record)
{
  md_file_inode *finode = &file_record->body.fileInode;
  ssize_t count;
  md_attr  *attr;
  char *fresh_lvl_data;
  const char *lvl_data;
  int level;
  ser_return_t rc = ser_ok;

  /* Skip scanning file levels if serialiser isn't interested in them */
  if( !SERIALISER_HAS_FN( serialiser, new_level))
    return 0;

  // Extract different levels.
  for( level = 0; level < N_FILE_LEVELS; level++) {
    attr = &finode->attr [level];
    
    // Completely skip empty levels.
    if( attr->entries == 0
#ifndef MD_VLONG
	&& attr->entriesHigh == 0
#endif
	)
      continue;

    /* Some consistency checking... */
    if( attr->entries != attr->unixAttr.mst_size) {
      fprintf( stderr, "Inconsistency in size of level %u: %lu != %lu\n",
	       level, attr->entries, attr->unixAttr.mst_size);
    }

    /* Level-2 metadata might be cached; if so, use the cached value */
    if( level == 2 && have_file_lvl2()) {
      lvl_data = lvl2_info.data;
      count = lvl2_info.data_size;
    } else {
      count = get_file_level( scl, file_record->head.ID, finode, level,
			      &fresh_lvl_data);

      if( count < 0) {
	fprintf( stderr, "Unable to fetch level %u metadata for %s\n", level,
		 md2PrintID(file_record->head.ID));
	continue;
      }

      if( count != attr->entries) {
	fprintf( stderr, "Unexpected size of level %u metadata for %s (%u != %lu)\n", level,
		 md2PrintID(file_record->head.ID), count, attr->entries);
	continue;
      }

      lvl_data = fresh_lvl_data;
    }

    rc = CALL_SERIALISER(serialiser, new_level, (file_record->head.ID, level,
						 attr, lvl_data, count));
    if( rc == ser_error)
      break;
  }

  
  return rc != ser_error ? 0 : -1;
}



/**
 *  Read a level into memory.  Returns number of bytes read on
 *  success, -1 on failure and update memory pointed to by buffer_p.
 *
 *  NB. the data is in a static buffer (on heap), so is only valid
 *  until the next call to this function.
 *
 *  As a hack, one can force the internal buffer to reset by calling
 *  the function with scl = finode = buffer_p = NULL and id = NullID.
 */
ssize_t get_file_level( SCL *scl, md_id_t id, md_file_inode *finode,
			int level, char **buffer_p)
{
  static char *level_data;
  static size_t level_data_max_size;

  size_t level_data_req_size;
  md_attr *attr;

  size_t read_size;
  long read_offset=0;
  mdPermission perm = {0,0};
  ssize_t count;


  // Free memory that is allocated off the heap.
  if( !scl && !finode && !buffer_p) {
    free( level_data);
    level_data = NULL;
    return 0;
  }

  attr = &finode->attr [level];

  read_size = attr->entries; // NB. we are missing high order long.

  // Make sure we have enough heap space to store level information.
  level_data_req_size = attr->entries+1; // +1 for terminating '\0';

  if( level_data_max_size < level_data_req_size) {
    level_data = realloc( level_data, level_data_req_size);

    if( !level_data) {
      fprintf( stderr, "OUT OF MEMORY when allocating %lu bytes.\n",
	       (unsigned long) level_data_req_size);
      return -1;
    }

    level_data_max_size = level_data_req_size;
  }
    
  mdpSetLevel( perm, level);

  count = myReadData( scl, id, perm, read_offset, read_size, level_data);
  
  if( count < 0) {
    if( count != -1)
      fprintf( stderr, "myReadData() returned %d when reading level %u for %s\n",
	       count, level, md2PrintID(id));
    return -1;
  }

  if( count != read_size)
    fprintf( stderr, "Partial read of level %u for %s (wanted %d, got %d)\n",
	     level, md2PrintID(id),
	     read_size, count);

  if( buffer_p)
    *buffer_p = level_data;

  return count;
}



/**
 *  Handle interrupts more nicely
 */
void handle_sig_shutdown( int signal)
{
  fprintf( stderr, "\nShutting down...\n");

  if( scl_global)
    sclClientClose( scl_global);

  if( verbosity >= 1)
    emit_final_numbers();

  exit(0);
}


void emit_final_numbers()
{
  struct timeval now, elapsed, diff, overhead, db;
  int h, m;
  double s;

  fprintf( stderr, "\nFinal stats:\n");
  fprintf( stderr, "\ninodes:\n");
  fprintf( stderr, "  nDir:     %7u\n", stats_current.nDir);
  fprintf( stderr, "  nFile:    %7u\n", stats_current.nFile);
  fprintf( stderr, "  nUnknown: %7u\n", stats_current.nUnknown);
  fprintf( stderr, "  nSkipped: %7u\n", stats_current.nSkipped);
  fprintf( stderr, "  -----------------\n");
  fprintf( stderr, "  Total:    %7u\n", stats_current.nDir + stats_current.nUnknown + 
	   stats_current.nFile + stats_current.nSkipped);
  fprintf( stderr, "\nMajor DB errors: %u\n\n", stats_current.nError);

  cache_emit_stats( stderr);

  gettimeofday( &now, NULL);
  timersub( &now, &started_at, &elapsed);

  split_time( elapsed, &h, &m, &s);

  fprintf( stderr, "\nTime elapsed: %6.1fs (%02u:%02u:%04.1f)\n",
	   elapsed.tv_sec + ((double)elapsed.tv_usec)/1000000,
	   h, m, s);

  myTimeElapsed( &overhead, &db);

  split_time( db, &h, &m, &s);

  fprintf( stderr, "    dbserver: %6.1fs (%02u:%02u:%04.1f)\n",
	   db.tv_sec + ((double)db.tv_usec)/1000000,
	   h, m, s);

  timersub( &overhead, &db, &diff);
  split_time( diff, &h, &m, &s);

  fprintf( stderr, "    overhead: %6.1fs (%02u:%02u:%04.1f)\n",
	   diff.tv_sec + ((double)diff.tv_usec)/1000000,
	   h, m, s);

  timersub( &elapsed, &overhead, &diff);
  split_time( diff, &h, &m, &s);

  fprintf( stderr, "    pnfsDump: %6.1fs (%02u:%02u:%04.1f)\n",
	   diff.tv_sec + ((double)diff.tv_usec)/1000000,
	   h, m, s);

  emit_freq( "\nAverage inode processing rate:", elapsed.tv_sec, NULL);
}


/**
 *  Split time into hours, minutes and seconds.
 */
inline static void split_time( struct timeval tm, int *h, int *m, double *s)
{
  *h = tm.tv_sec / SECONDS_IN_HOUR;

  *m = (tm.tv_sec - *h * SECONDS_IN_HOUR) / SECONDS_IN_MINUTE;  

  *s = tm.tv_sec - *h * SECONDS_IN_HOUR - *m * SECONDS_IN_MINUTE;

  *s += ((double)tm.tv_usec)/1000000;
}



/**
 *  This is a poor-man's timer: call this fn fairly frequently to
 *  cause emit_periodic_info() fn to be called at the correct time.
 *  (N.B. we can't use system timer due to shm-mem's use of alarm(2)
 *  for a timeout).
 *
 *  The overhead is one call to gettimeofday() (which will jump the
 *  user-kernel boundary), a few integer tests and the overhead in
 *  calling a function.
 */
void test_for_periodic_info()
{
  struct timeval now;

  if( verbosity < N_LEVELS_VERBOSITY)
    return;

  gettimeofday( &now, NULL);

  if( timercmp( &now, &next_periodic_info, >)) {
    timeradd( &now, &periodicity, &next_periodic_info);
    emit_periodic_info();
  }
}



/**
 *  A routine called periodically to emit the current rate over the
 *  past period.
 */
void emit_periodic_info()
{
  emit_freq( "inode processing:", periodicity.tv_sec, &stats_last);

  /* Update when we last emited stats */
  memcpy( &stats_last, &stats_current, sizeof( stats_t));
}



/**
 *  Emit a line on stderr like:
 *
 *    <label> 100 Hz
 *
 *  This takes a label (must non be NULL), an elapsed_time (in
 *  seconds) and a pointer to the old stats.  If old is NULL, then
 *  elapsed_time is assumed to be how long the dump has been underway
 *  and the average frequency is returned.
 */
void emit_freq( const char *label, int elapsed_time, stats_t *old)
{
  int last = 0;
  int current, processed;

  if( old)
    last = old->nDir + old->nFile + old->nUnknown + old->nSkipped;

  current = stats_current.nDir + stats_current.nFile + stats_current.nUnknown + stats_current.nSkipped;

  processed = current - last;

  fprintf( stderr, "%s %u Hz\n", label, (int) (processed / (float)elapsed_time + 0.5));
}


/**
 *  Emit a space-separated list of available serialisers on stderr.
 *  The list is terminated by a new-line character ('\n').
 */
void list_available_serialisers()
{
  fprintf( stderr, "Available output formats: ");
  //pnfs_serialisers_list( stderr, " ", 0); 
  fprintf( stderr, "\n  ");

  pnfs_serialisers_list( stderr, "\n  ", 1); 
  fprintf( stderr, "\n");
}
