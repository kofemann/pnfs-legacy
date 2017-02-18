/**
 *  Emit suitable output to feed into "md5sum -c"
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>


#include "dump_version.h"
#include "dump_serialiser.h"
#include "dump_utils.h"
#include "dump_ser_abs_file.h"

#include "dump_cache.h"

#include "md5.h"

#include "dump_ser_verify.h"

#define FLAG_EMIT_ALL   (FLAG_EMIT_LEVEL_TESTS | FLAG_EMIT_TAG_TESTS | FLAG_EMIT_FILE_ID_TESTS | FLAG_EMIT_DIR_ID_TESTS)
#define FLAG_EMIT_NONE  0x00

#define FLAG_EMIT_LEVEL_TESTS   0x01
#define FLAG_EMIT_TAG_TESTS     0x02
#define FLAG_EMIT_FILE_ID_TESTS 0x04
#define FLAG_EMIT_DIR_ID_TESTS 0x08

#define append_to( DEST, SRC, SIZE) {memcpy( DEST, SRC, SIZE); DEST += (SIZE);}


static ser_return_t begin_serialisation( const char *cmdline, int argc, char *argv[]);
static ser_return_t end_serialisation();


static ser_return_t begin_dir( md_id_t parent_dir_id, md_id_t id,
			       const char *name, md_attr *attr);
static ser_return_t end_dir( md_id_t parent_dir_id, md_id_t id, const char *name);

static ser_return_t begin_file( md_id_t parent_dir_id, md_id_t id, const char *name,
				filesize_t size, lvl2_info_t *lvl2, md_attr *attr);

static ser_return_t end_file( md_id_t parent_dir_id, md_id_t id, const char *name);


static ser_return_t new_primary_tag( md_id_t parent_dir_id,
				     md_id_t id,
				     const char *name,
				     md_attr *attr,
				     const char *data,
				     size_t data_length);
static ser_return_t new_pseudo_tag( md_id_t parent_dir_id,
				    md_id_t parent_tag_id,
				    md_id_t id,
				    const char *name,
				    md_attr *attr,
				    const char *data,
				    size_t data_length);
static ser_return_t new_invalid_tag( md_id_t parent_dir_id,
				     md_id_t parent_tag_id,
				     md_id_t id,
				     const char *name);
static ser_return_t new_inherited_tag( md_id_t parent_dir_id,
				       md_id_t parent_tag_id,
				       md_id_t id, const char *name);
static ser_return_t new_level( md_id_t file_id, int level,
			       md_attr *attr, const char *data,
			       size_t size);
static ser_return_t new_link( md_id_t parent, md_id_t link_id, const char *name,
			      md_attr *attr, const char *data, size_t size);
static char *calculate_md5sum( const char *data, size_t len);
static const char *build_path( const char *filename, size_t filename_len, size_t *path_len_p);
static void emit_line( const char *md5sum, const char *cmd, size_t cmd_len,
		       const char *filename, size_t filename_len);



static dump_serialiser_t info = {
  "suitable output for md5sum(1): [-p <path-to-root> | -r] [-l] [-t] [-f] [-d]",
  FLAGS_REQ_DEFAULT,
  begin_serialisation, end_serialisation,
  NULL, NULL,  /* ignore begin and end dump */
  begin_dir, end_dir,
  new_primary_tag, new_pseudo_tag, new_invalid_tag, new_inherited_tag,
  begin_file, end_file,
  new_level,
  new_link,
};

static int n_levels, n_inherited_tags, n_inherited_tags_tested;
static int n_inherited_tags_no_top, n_inherited_tags_no_data;
static int should_emit = FLAG_EMIT_ALL;
static char *path_prefix;
static char *current_filename;
static size_t current_filename_len;




/**
 *  Our registry call-back
 */

dump_serialiser_t *verify_get_serialiser()
{
  return &info;
}


/**
 *  Call-backs for serialisation.
 */

ser_return_t begin_serialisation( const char *cmdline,
				  int argc, char *argv[])
{
  int c, first_flag=1, absolute_path=0, relative_path=0;
  char *p;
  size_t len;
  time_t now;

  optind = 1;  /* Reset getopt() state */

  while( (c = getopt( argc, argv, "+:p:rltfd")) != -1) {
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

      if( len == 0) {
	fprintf( stderr, "Invalid prefix: %s\n", optarg);
	return ser_error;
      }

      free( path_prefix);
      path_prefix = strdup( p);
      abs_filename_flush();
      abs_filename_append_to( path_prefix);
      absolute_path = 1;
      break;

    case 'l':
      if( first_flag) {
	first_flag = 0;
	should_emit = FLAG_EMIT_NONE;
      }
      should_emit ^= FLAG_EMIT_LEVEL_TESTS;
      break;

    case 'f':
      if( first_flag) {
	first_flag = 0;
	should_emit = FLAG_EMIT_NONE;
      }
      should_emit ^= FLAG_EMIT_FILE_ID_TESTS;
      break;

    case 'd':
      if( first_flag) {
	first_flag = 0;
	should_emit = FLAG_EMIT_NONE;
      }
      should_emit ^= FLAG_EMIT_DIR_ID_TESTS;
      break;

    case 'r':
      free( path_prefix);
      path_prefix = NULL;
      abs_filename_flush();
      relative_path = 1;
      break;

    case 't':
      if( first_flag) {
	first_flag = 0;
	should_emit = FLAG_EMIT_NONE;
      }
      should_emit ^= FLAG_EMIT_TAG_TESTS;
      break;
      
    case ':':
      fprintf( stderr, "Option -%c requires an argument\n", optopt);
      return ser_error;

    case '?':
      fprintf( stderr, "Unknown output option: -%c\n", optopt);
      return ser_error;
    }
  }

  if( absolute_path && relative_path) {
    fprintf( stderr, "You've specified an absolute path and that relative paths should be used; please make your mind up!\n");
    return ser_error;
  }

  if( !absolute_path && !relative_path) {
    fprintf( stderr, "You have to specify a path (with -p option) or that relative paths should be used (-r)\n");
    return ser_error;
  }

  /* Emit comments at beginning of file */
  time( &now);
  puts( "#");
  puts( "#              pnfsDump md5sum verification script");
  puts( "#              -----------------------------------");
  printf( "#\n");
  printf( "#  Generated using pnfsDump v" PNFSDUMP_VERSION_STR " on %s", ctime( &now));
  printf( "#\n");
  printf( "#  Command-line:\n");
  printf( "#\n");
  printf( "#     %s\n", cmdline);
  printf( "#\n");
  if( relative_path) {
    printf( "#  To verify, make sure the namespace is mounted (for example, \n");
    printf( "#  localhost:/pnfs mounted at /pnfs) and, if the PNFS root\n");
    printf( "#  directory is /pnfs/path/to/root, run:\n");
    printf( "#\n");
    printf( "#      cd /pnfs/path/to/root\n");
  } else {
    printf( "#  To verify, make sure the namespace is mounted so that\n");
    printf( "#\n");
    printf( "#      /%s\n", path_prefix);
    printf( "#\n");
    printf( "#  is a valid path and run:\n");
    printf( "#\n");
  }
  printf( "#      md5sum -c this-file | grep -v ': OK$'\n");
  printf( "#\n");
  printf( "#  Where \"this-file\" is the path to this file.\n");
  printf( "#\n");
  printf( "#  This test is successful if the line above (starting \"md5sum -c ...\")\n");
  printf( "#  produces no output.\n");
  printf( "#\n");
  printf( "#  Some additional statistics are available at the end of this file.\n");
  printf( "#\n");

  return ser_ok;
}



ser_return_t end_serialisation()
{
  time_t now;

  time( &now);

  printf( "#\n");

  if( n_inherited_tags > 0) {
    printf( "#  %u inherited tags encountered; of these:\n", n_inherited_tags);
    printf( "#  \t%u tags will be tested,\n", n_inherited_tags_tested);
    printf( "#  \t%u tags will be skipped (%u unresolved top-tag, %u no data)\n",
	    n_inherited_tags - n_inherited_tags_tested,
	    n_inherited_tags_no_top, n_inherited_tags_no_data);
    printf( "#\n");
  }

  printf( "#  Dump completed %s", ctime( &now));
  printf( "#\n");

  if( path_prefix)
    abs_filename_remove_from( path_prefix);

  if( abs_filename_strlen() != 0)
    fprintf( stderr, "Final abs. filename length unexpectidly not-zero: %ld\n",
	     abs_filename_strlen());

  abs_filename_flush();
  build_path( NULL, 0, NULL);
  emit_line( NULL, NULL, 0, NULL, 0);

  return ser_ok;
}




/**
 *  Call-back for beginning and end of a directory.
 */
ser_return_t begin_dir( md_id_t parent_dir_id, md_id_t id, const char *name,
	       md_attr *attr)
{
  char pnfs_id[26];
  char *md5sum;

  if( should_emit & FLAG_EMIT_DIR_ID_TESTS) {    

    mdStringID_r( id, pnfs_id);

    /*  pnfsd adds a line-feed at the end of the PNFS-ID  */
    pnfs_id[24] = '\n';
    pnfs_id[25] = '\0';

    md5sum = calculate_md5sum( pnfs_id, 25);

    emit_line( md5sum, "id", 2, name, strlen( name));
  }

  abs_filename_append_to( name);
  
  return ser_ok;
}

ser_return_t end_dir( md_id_t parent_dir_id, md_id_t id, const char *name)
{
  abs_filename_remove_from( name);

  return ser_ok;
}


ser_return_t begin_file( md_id_t parent_dir_id, md_id_t id, const char *name,
			 filesize_t size, lvl2_info_t *lvl2, md_attr *attr)
{
  char pnfs_id[26];
  char *md5sum;

  if( current_filename) {
    /* This shouldn't happen */
    fprintf( stderr, "begin_file() / end_file() out of sequence.\n");
    free( current_filename);
  }
  
  current_filename_len = strlen( name);
  current_filename = malloc( current_filename_len+1);
  if( current_filename) {
    memcpy( current_filename, name, current_filename_len);
    current_filename [current_filename_len] = '\0';
  }

  n_levels = 0;

  if( should_emit & FLAG_EMIT_FILE_ID_TESTS) {
    mdStringID_r( id, pnfs_id);

    /*  pnfsd adds a line-feed at the end of the PNFS-ID  */
    pnfs_id[24] = '\n';
    pnfs_id[25] = '\0';

    md5sum = calculate_md5sum( pnfs_id, 25);

    emit_line( md5sum, "id", 2, name, current_filename_len);
  }

  return ser_ok;
}


ser_return_t end_file( md_id_t parent_dir_id, md_id_t id, const char *name)
{
  if( n_levels == 0 && !FLAG_EMIT_FILE_ID_TESTS) {
    fprintf( stderr, "File has no level metadata; this file will not be checked.\n");
    fprintf( stderr, "\tdirectory: %s\n", md2PrintID( parent_dir_id));
    fprintf( stderr, "\tfile: %s\n", md2PrintID( id));
    fprintf( stderr, "\tname: %s\n", name);
    fprintf( stderr, "\tpath: %s\n", build_path( name, strlen( name), NULL));
  }

  free( current_filename);
  current_filename = NULL;
  current_filename_len = 0;

  return ser_ok;
}



ser_return_t new_primary_tag( md_id_t parent_dir_id, md_id_t id, const char *name,
			      md_attr *attr, const char *data, size_t data_length)
{
  char *md5sum;

  if( !(should_emit & FLAG_EMIT_TAG_TESTS))
    return ser_ok;

  md5sum = calculate_md5sum( data, data_length);

  emit_line( md5sum, "tag", 3, name, strlen( name));

  return ser_ok;
}


ser_return_t new_pseudo_tag( md_id_t parent_dir_id, md_id_t parent_tag_id,
			     md_id_t id, const char *name, md_attr *attr,
			     const char *data, size_t data_length)
{
  char *md5sum;

  if( !(should_emit & FLAG_EMIT_TAG_TESTS))
    return ser_ok;

  md5sum = calculate_md5sum( data, data_length);
  
  emit_line( md5sum, "tag", 3, name, strlen( name));

  return ser_ok;
}


ser_return_t new_invalid_tag( md_id_t parent_dir_id, md_id_t parent_tag_id,
			      md_id_t id, const char *name)
{
  if( !(should_emit & FLAG_EMIT_TAG_TESTS))
    return ser_ok;

  fprintf( stderr, "Encountered an invalid tag; value will not be checked.\n");
  fprintf( stderr, "\tdirectory: %s\n", md2PrintID( parent_dir_id));
  fprintf( stderr, "\ttag: %s\n", md2PrintID( parent_tag_id));
  fprintf( stderr, "\tname: %s\n", name);
  fprintf( stderr, "\tparent-tag: %s\n", md2PrintID( id));

  return ser_ok;
}


/**
 *  A best-effort at emitting a check for an inherited tag.  If tag
 *  inheritance doesn't follow subdirectories then this will fail.
 *
 *  We do this in two steps: 1) identify the primary or pseudo-primary
 *  tag, 2) get this tag's data.  This is to give us a chance to use
 *  our local data cache and so, should we loop when getting the tag
 *  data, we don't hit the db a second time when looking for the
 *  top-most tag.
 */
ser_return_t new_inherited_tag( md_id_t parent_dir_id, md_id_t parent_tag_id,
				md_id_t id, const char *name)
{
  char *md5sum, data [TAG_DATA_SIZE];
  size_t data_len;

  if( !(should_emit & FLAG_EMIT_TAG_TESTS))
    return ser_ok;

  n_inherited_tags++;

  /**
   *  Get tag data, this may result in a db lookups, but cache should
   *  make this unlikely.
   */
  if( !cache_find_tag_value( id, data, sizeof( data), &data_len)) {
    n_inherited_tags_no_data++;

    fprintf( stderr, "Failed to find tag data for inherited tag.\n");
    fprintf( stderr, "\tdirectory: %s\n", md2PrintID( parent_dir_id));
    fprintf( stderr, "\ttag-name: %s\n", name);
    fprintf( stderr, "\ttag-id: %s\n", md2PrintID( id));
    fprintf( stderr, "\tparent tag: %s\n", md2PrintID( parent_tag_id));

    return ser_ok;
  }

  md5sum = calculate_md5sum( data, data_len);

  emit_line( md5sum, "tag", 3, name, strlen( name));

  n_inherited_tags_tested++;

  return ser_ok;
}



/**
 *  Emit a check for a file's metadata, stored in a level.
 */
ser_return_t new_level( md_id_t file_id, int level,
			md_attr *attr, const char *data,
			size_t size)
{  
  char cmd[] = "use)(2";
  char *md5sum;

  n_levels++;
  
  if( should_emit & FLAG_EMIT_LEVEL_TESTS) {

    md5sum = calculate_md5sum( data, size);

    if( level != 2)
      cmd [5] = '0' + level;

    emit_line( md5sum, cmd, 6, current_filename,
	       current_filename_len);
  }

  return ser_ok;
}


ser_return_t new_link( md_id_t parent, md_id_t link_id, const char *name,
		       md_attr *attr, const char *data, size_t size)
{
  char *targetname, *md5sum;
  char pnfs_id [26];

  if( !(targetname = malloc( size+1))) {
    fprintf( stderr, "out of memory.\n");
    return ser_ok;
  }

  strncpy( targetname, data, size);
  targetname [size] = '\0';

  if( should_emit & FLAG_EMIT_FILE_ID_TESTS) {

    mdStringID_r( link_id, pnfs_id);

    /*  pnfsd adds a line-feed at the end of the PNFS-ID  */
    pnfs_id[24] = '\n';
    pnfs_id[25] = '\0';

    md5sum = calculate_md5sum( pnfs_id, 25);

    emit_line( md5sum, "id", 2, name, strlen( name));
  }

  fprintf( stderr, "The link's presence will be checked but what the link points to (its target) will not.\n");
  fprintf( stderr, "\tdirectory: %s\n", md2PrintID( parent));
  fprintf( stderr, "\tlink-ID: %s\n", md2PrintID( link_id));
  fprintf( stderr, "\tfilename: %s\n", name);
  fprintf( stderr, "\ttarget: %s\n", targetname);

  free( targetname);

  return ser_ok;
}




/**
 *  Calculate the MD5sum of a string and return the value.
 *
 *  The caller MUST NOT free the returned string.
 */
char *calculate_md5sum( const char *data, size_t len)
{
  static char digest[32+1];
  MD5_CTX mdContext;

  MD5Init (&mdContext);
  MD5Update (&mdContext, data, len);
  MD5Final (&mdContext);

  utils_hex_bytes( digest, (unsigned char *) mdContext.digest, 16, 0);

  return digest;
}


/**
 *  Return either the absolute or relative path to the given file.
 *
 *  If path_len_p is not NULL, then the memory area it points to is
 *  updated to contain the string-length of the resulting path.
 *
 *  As a special case: passing NULL as a parameter free()s up any
 *  allocated memory.
 */
const char *build_path( const char *filename, size_t filename_len,
			size_t *path_len_p)
{
  static char *path_storage;
  static size_t path_storage_len;
  char *p;
  const char *abs_path;
  size_t req_len, abs_path_len;

  if( filename == NULL) {
    free( path_storage);
    path_storage_len = 0;
    return NULL;
  }

  abs_path = abs_filename_value();
  abs_path_len = abs_filename_strlen();

  if( abs_path[0] == '\0') {

    if( path_prefix != NULL)
      fprintf( stderr, "ASSERT: path_prefix != NULL\n");

    if( filename_len+1 > path_storage_len) {
      path_storage_len = filename_len+1; 
      path_storage = realloc( path_storage, path_storage_len);
    }

    p = path_storage;
    append_to( p, filename, filename_len);
    *p = '\0';

    if( path_len_p)
      *path_len_p = filename_len;

    return path_storage;
  }


  req_len = abs_path_len + filename_len + 1;

  if( req_len+1 > path_storage_len) {
    path_storage_len = req_len+1;
    path_storage = realloc( path_storage, path_storage_len);
  }

  p = path_storage;
  if( path_prefix) {
    append_to( p, abs_path, abs_path_len);
  } else {
    append_to( p, abs_path+1, abs_path_len-1); // skip over initial '/'
  }

  *p++ = '/';
  append_to( p, filename, filename_len);
  *p = '\0';

  if( path_len_p)
    *path_len_p = req_len;

  return path_storage;
}




#define MD5_STRING_LEN  32
#define DECORATION_LEN  5


/**
 *  Emit a new line involving an md5sum a command and (optionally) a
 *  filename.  The format of the output is:
 *
 *    <md5sum> <space> <space> ".(" <cmd> ")(" <filename> ")"
 *
 *  The filename_len may be zero.  If so, filename is ignored and the
 *  following line is emitted:
 *
 *    <md5sum> <space> <space> ".(" <cmd> ")()"
 *
 *  As a special case, mdsum may be NULL, resulting in any allocated
 *  memory being free()ed.
 */
void emit_line( const char *md5sum, const char *cmd, size_t cmd_len,
		const char *filename, size_t filename_len)
{
  static size_t line_storage, file_storage;
  static char *line, *file;
  char *f, *l;
  const char *file_and_path;
  size_t line_len, file_len, file_and_path_len;

  if( !md5sum) {
    free( file);
    free( line);
    file_storage = line_storage = 0;
    return;
  }

  file_len = cmd_len + DECORATION_LEN + filename_len;

  if( file_len >= file_storage) {
    file_storage = file_len+1;
    file = realloc( file, file_storage);
  }

  f = file;
  append_to( f, ".(", 2);
  append_to( f, cmd, cmd_len);
  append_to( f, ")(", 2);
  append_to( f, filename, filename_len);
  append_to( f, ")", 1);
  *f = '\0';

  file_and_path = build_path( file, file_len, &file_and_path_len);

  line_len = MD5_STRING_LEN + 2 + file_and_path_len;

  if( line_len >= line_storage) {
    line_storage = line_len +1;
    line = realloc( line, line_storage);
  }

  l = line;
  append_to( l, md5sum, MD5_STRING_LEN);
  append_to( l, "  ", 2);
  append_to( l, file_and_path, file_and_path_len);
  *l = '\0';

  puts( line);
}
		
