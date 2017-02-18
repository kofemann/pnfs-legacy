/**
 *  Files: provide a simple list of files in the catalogue.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "dump_utils.h"

#include "dump_serialiser.h"

#include "dump_ser_abs_file.h"
#include "dump_ser_files.h"

#define BYTES_IN_PNFS_ID 24

static ser_return_t begin_serialisation( const char *cmdline, int argc, char *argv[]);
static ser_return_t end_serialisation();

static ser_return_t begin_dir( md_id_t parent_dir_id, md_id_t id,
			       const char *name, md_attr *attr);
static ser_return_t end_dir( md_id_t parent_dir_id, md_id_t id, const char *name);

static ser_return_t begin_file( md_id_t parent_dir_id, md_id_t id, const char *name,
				filesize_t size, lvl2_info_t *info, md_attr *attr);
static ser_return_t new_level( md_id_t file_id, int level, md_attr *attr,
			       const char *data, size_t size);
static ser_return_t end_file( md_id_t dir_id, md_id_t id, const char *name);
inline static void append_to( const char *dest_start, char **dest_p,
			      const char *src, size_t len);


static dump_serialiser_t info = {
  "simple list of files: [-p <prefix>] [-l | -s] [-f] [-d] [-a] [-r] [-h <HSM type>]",
  FLAGS_REQ_DEFAULT,
  begin_serialisation, end_serialisation,
  NULL, NULL,
  begin_dir, end_dir,
  NULL, NULL, NULL, NULL, /* Ignore all tag information */
  begin_file, end_file,   /* Ignore file end */
  new_level,
  NULL,                   /* Ignore all links */
};


static char *access_latency_str[] = {
  [l2_al_online]   = "ONLINE  ",
  [l2_al_nearline] = "NEARLINE",
};

static char *retention_policy_str[] = {
  [l2_rp_replica]   = "REPLICA  ",
  [l2_rp_custodial] = "CUSTODIAL",
};



typedef enum {
  SHORT_NAMES,
  LONG_NAMES,
} output_mode_t;

typedef enum {
  HSM_OSM,
} hsm_type_t;

#define FLAG_SHOW_FILE_PNFS_ID   0x01
#define FLAG_SHOW_DIR_PNFS_ID    0x02
#define FLAG_SHOW_FILENAME       0x04
#define FLAG_SHOW_FILE_AL        0x08
#define FLAG_SHOW_FILE_RP        0x10
#define FLAG_SHOW_HSM            0x20

#define FLAG_SHOW_DEFAULT        FLAG_SHOW_FILENAME

#define FLAG_SHOW_REQ_LEVEL2     (FLAG_SHOW_FILE_AL | FLAG_SHOW_FILE_RP)

static int output_flags = FLAG_SHOW_DEFAULT;
static output_mode_t output_mode = LONG_NAMES;
static char *prefix;
static lvl2_info_t *stored_lvl2;
static hsm_type_t hsm_type = HSM_OSM;
static int hsm_stored;


/**
 *  Our registry call-back
 */

dump_serialiser_t *files_get_serialiser()
{
  return &info;
}


/**
 *  Call-backs for serialisation.
 */

ser_return_t begin_serialisation( const char *cmdline,
				  int argc, char *argv[])
{
  char *p = NULL;
  size_t len;
  int c, is_first=1;

  optind = 1;  /* Reset getopt() state */

  while( (c = getopt( argc, argv, "+:p:h:slfdar")) != -1) {

    if( is_first && strchr( "slfdarh", c)) {
      output_flags = 0;
      is_first = 0;
    }

    switch( c) {
    case 'p':
      /* Skip over any initial '/' */
      for( p = optarg; *p == '/'; p++);

      /* Remove any trailing '/' */
      for(;;) {
	len = strlen(p);

	if( len > 0 && p [len-1] == '/')
	  p [len-1] = '\0';
	else
	  break;
      }

      if( len > 0) {
	prefix = strdup( p);
	abs_filename_append_to( prefix);
      } else {
	fprintf( stderr, "Invalid prefix: %s\n", optarg);
	return ser_error;
      }
      break;

    case 'h':
      if( strcasecmp( optarg, "osm")) {
	fprintf( stderr, "Unknown HSM type: \"%s\"\n", optarg);
	return ser_error;
      }
      hsm_type = HSM_OSM;
      output_flags ^= FLAG_SHOW_HSM;
      break;
      
    case 's':
      output_flags ^= FLAG_SHOW_FILENAME;
      output_mode = SHORT_NAMES;
      break;

    case 'l':
      output_flags ^= FLAG_SHOW_FILENAME;
      output_mode = LONG_NAMES;
      break;

    case 'f':
      output_flags ^= FLAG_SHOW_FILE_PNFS_ID;
      break;

    case 'd':
      output_flags ^= FLAG_SHOW_DIR_PNFS_ID;
      break;

    case 'a':
      output_flags ^= FLAG_SHOW_FILE_AL;
      break;

    case 'r':
      output_flags ^= FLAG_SHOW_FILE_RP;
      break;

    case ':':
      fprintf( stderr, "Option -%c requires an argument\n", optopt);
      return ser_error;

    case '?':
      fprintf( stderr, "Unknown output option: -%c\n", optopt);
      return ser_error;
    }

    is_first = 0;
  }

  if( p && output_mode != LONG_NAMES)
    fprintf( stderr, "Prefix will be ignored unless using long filename output.\n");

  
  // Make sure we get level-2 metadata, if needed
  if( output_flags & FLAG_SHOW_REQ_LEVEL2)
    info.flags ^= FLAGS_REQ_LEVEL2;

  return ser_ok;
}


ser_return_t end_serialisation()
{
  static md_id_t null_id;

  if( prefix) {
    abs_filename_remove_from( prefix);
    
    if( abs_filename_strlen() != 0)
      fprintf( stderr, "Final abs. filename length unexpectidly not-zero: %ld\n",
	       abs_filename_strlen());
  }

  abs_filename_flush();

  /* Free up dynamically allocated storage */
  begin_file( null_id, null_id, NULL, 0, NULL, NULL);


  return ser_ok;
}





/**
 *  Call-back for beginning and end of a directory.
 */
ser_return_t begin_dir( md_id_t parent_dir_id, md_id_t id, const char *name,
	       md_attr *attr)
{
  abs_filename_append_to( name);
  return ser_ok;
}

ser_return_t end_dir( md_id_t parent_dir_id, md_id_t id, const char *name)
{
  abs_filename_remove_from( name);
  return ser_ok;
}


ser_return_t begin_file( md_id_t dir_id, md_id_t id, const char *name,
			 filesize_t size, lvl2_info_t *lvl2, md_attr *attr)
{
  stored_lvl2 = lvl2;
  hsm_stored=0;
  return ser_ok;
}


ser_return_t new_level( md_id_t file_id, int level, md_attr *attr, const char *data,
			size_t size)
{
  switch( hsm_type) {
  case HSM_OSM:
    if( level == 1)
      hsm_stored = size > 0;
    break;
  }

  return ser_ok;
}


ser_return_t end_file( md_id_t dir_id, md_id_t id, const char *name)
{
  char buf[25], *p;
  const char *printed_name=NULL;
  static char *line;
  static size_t max_line_len=0;
  size_t req_line_len, printed_name_len=0;
  access_latency_value_t al;
  retention_policy_value_t rp;

  /* Small "hack" to allow clean-up of dynamically allocated storage */
  if( name == NULL) {
    free( line);
    line = NULL;
    max_line_len = 0;
    return ser_ok;
  }

  if( output_flags & FLAG_SHOW_FILENAME) {

    if( output_mode == LONG_NAMES)
      abs_filename_append_to( name);
  
    printed_name = output_mode == LONG_NAMES ? abs_filename_value() : name;
    printed_name_len = strlen( printed_name);
  }

  /* Length of line, not including final '\n' */
  req_line_len = (output_flags & FLAG_SHOW_FILE_PNFS_ID ? 25 : 0) +
    (output_flags & FLAG_SHOW_DIR_PNFS_ID ? 25 : 0) + printed_name_len +
    (output_flags & FLAG_SHOW_FILE_AL ? 22 : 0) +
    (output_flags & FLAG_SHOW_FILE_RP ? 25 : 0);

  if( req_line_len >= max_line_len) {
    max_line_len = req_line_len;
    line = realloc( line, max_line_len +1); /* +1 for '\0' */
    if( !line) {
      max_line_len = 0;
      goto clean_exit;
    }
  }

  p = line;
  
  if( output_flags & FLAG_SHOW_FILE_PNFS_ID) {
    mdStringID_r( id, buf);
    append_to( line, &p, buf, BYTES_IN_PNFS_ID);
  }

  if( output_flags & FLAG_SHOW_DIR_PNFS_ID) {
    mdStringID_r( dir_id, buf);
    append_to( line, &p, buf, BYTES_IN_PNFS_ID);
  }

  if( output_flags & FLAG_SHOW_FILE_RP) {
    if( deduce_file_rp( dir_id, id, stored_lvl2, &rp))
      append_to( line, &p, retention_policy_str[rp], 9);
    else 
      append_to( line, &p, "UNKNOWN  ", 9);
  }


  if( output_flags & FLAG_SHOW_FILE_AL) {
    if( deduce_file_al( dir_id, id, stored_lvl2, &al))
      append_to( line, &p, access_latency_str[al], 8);
    else 
      append_to( line, &p, "UNKNOWN ", 8);
  }

  if( output_flags & FLAG_SHOW_HSM) {
    if( hsm_stored)
      append_to( line, &p, "IS-STORED ", 10);
    else
      append_to( line, &p, "NOT-STORED", 10);
  }

  if( output_flags & FLAG_SHOW_FILENAME)
    append_to( line, &p, printed_name, printed_name_len);

  *p = '\0';

  puts( line);

 clean_exit:
  if( output_flags & FLAG_SHOW_FILENAME &&
      output_mode == LONG_NAMES)
    abs_filename_remove_from( name);

  stored_lvl2 = NULL;

  return ser_ok;
}




/**
 *  If *dest_p != dest_start then a space character is appended to
 *  *dest_p and *dest_p is increased by one.
 *
 *  The first len bytes from src to *dest_p are copied and *dest_p is
 *  increased by len.
 */
inline void append_to( const char *dest_start, char **dest_p, const char *src, size_t len)
{
  char *p = *dest_p;

  if( p != dest_start)
    *p++ = ' ';

  memcpy( p, src, len);

  *dest_p = p+len;
}


