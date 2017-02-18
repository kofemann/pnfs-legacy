/**
 *  simple serialiser: very simple example of serialising PNFS data.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dump_serialiser.h"
#include "dump_utils.h"

#include "dump_ser_simple.h"

static ser_return_t begin_serialisation( const char *cmdline, int argc, char *argv[]);
static ser_return_t end_serialisation();

static ser_return_t begin_dump( md_id_t root_id);

static ser_return_t begin_dir( md_id_t parent_dir_id, md_id_t id,
		      const char *name, md_attr *attr);
static ser_return_t end_dir( md_id_t parent_dir_id, md_id_t id, const char *name);

static ser_return_t begin_file( md_id_t parent_dir_id, md_id_t id, const char *name,
				filesize_t size, lvl2_info_t *lvl2, md_attr *attr);

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
static ser_return_t new_level( md_id_t file_id, int level,
			       md_attr *attr, const char *data,
			       size_t size);
static ser_return_t new_link( md_id_t parent, md_id_t link_id, const char *name,
			      md_attr *attr, const char *data, size_t size);


static char *truncate_string( const char *string);


static dump_serialiser_t info = {
  "a terse and somewhat cryptic format",
  FLAGS_REQ_DEFAULT,
  begin_serialisation, end_serialisation,
  begin_dump, NULL,  /* ignore end dump */
  begin_dir, end_dir,
  new_primary_tag, new_pseudo_tag, new_invalid_tag, NULL, /* ignore inherited tags */
  begin_file, NULL, /* ignore end file */
  new_level,
  new_link,
};


/**
 *  Our registry call-back
 */

dump_serialiser_t *simple_get_serialiser()
{
  return &info;
}


/**
 *  Call-backs for serialisation.
 */

ser_return_t begin_serialisation( const char *cmdline,
				  int argc, char *argv[])
{
  int i;

  printf( "CMD-LINE: %s\n", cmdline);

  if( !argc) {
    printf( "STARTING.\n");
    return ser_ok;
  }

  printf( "STARTING: ");

  for( i = 1; i < argc; i++) {
    printf( "%s", argv[i]);
    if( i < argc)
      printf( " ");
  }

  printf( "\n");

  return ser_ok;
}


ser_return_t end_serialisation()
{
  printf( "FINISHING.\n");

  return ser_ok;
}

ser_return_t begin_dump( md_id_t root_id)
{
  printf( "Root is %s\n", mdStringID(root_id));

  return ser_ok;
}


/**
 *  Call-back for beginning and end of a directory.
 */
ser_return_t begin_dir( md_id_t parent_dir_id, md_id_t id, const char *name,
	       md_attr *attr)
{
  char parent_dir_id_buf [25];

  printf( "Entering dir [%s, %s]: %s\n",
	  mdStringID( id),
	  mdStringID_r( parent_dir_id, parent_dir_id_buf),
	  name);

  return ser_ok;
}

ser_return_t end_dir( md_id_t parent_dir_id, md_id_t id, const char *name)
{
  char parent_dir_id_buf [25];

  printf( "Leaving dir [%s, %s]: %s\n",
 	  mdStringID( id),
	  mdStringID_r( parent_dir_id, parent_dir_id_buf),
	  name);

  return ser_ok;
}


ser_return_t begin_file( md_id_t parent_dir_id, md_id_t id, const char *name,
			 filesize_t size, lvl2_info_t *lvl2, md_attr *attr)
{
  char parent_dir_id_buf [25];

  printf( "    File [%s, %s]: %s (%llu)\n",
	  mdStringID( id),
	  mdStringID_r( parent_dir_id, parent_dir_id_buf),
	  name, size);

  return ser_ok;
}


ser_return_t new_primary_tag( md_id_t parent_dir_id, md_id_t id, const char *name,
			      md_attr *attr, const char *data, size_t data_length)
{
  char parent_dir_id_buf [25];

  printf( "    P-tag  [%s, %s]: %s (%lu) \"%s\"\n", 
	  mdStringID( id),
	  mdStringID_r( parent_dir_id, parent_dir_id_buf),
	  name, (unsigned long)data_length, 
	  truncate_string( data));
  return ser_ok;
}


ser_return_t new_pseudo_tag( md_id_t parent_dir_id, md_id_t parent_tag_id,
			     md_id_t id, const char *name, md_attr *attr,
			     const char *data, size_t data_length)
{
  char parent_dir_id_buf [25];
  char parent_tag_id_buf [25];

  printf( "    Pp-tag [%s, %s, %s]: %s (%lu) \"%s\"\n",
	  mdStringID( id),
	  mdStringID_r( parent_tag_id, parent_tag_id_buf),
	  mdStringID_r( parent_dir_id, parent_dir_id_buf),
	  name,
	  (unsigned long)data_length,
	  truncate_string( data));

  return ser_ok;
}


ser_return_t new_invalid_tag( md_id_t parent_dir_id, md_id_t parent_tag_id,
			      md_id_t id, const char *name)
{
  char parent_dir_id_buf [25];
  char parent_tag_id_buf [25];

  printf( "  Invald-tag: [%s, %s, %s] %s\n",
	  mdStringID( id),
	  mdStringID_r( parent_dir_id, parent_dir_id_buf),
	  mdStringID_r( parent_tag_id, parent_tag_id_buf),
	  name);

  return ser_ok;
}


/**
 *  
 */
ser_return_t new_level( md_id_t file_id, int level,
			md_attr *attr, const char *data,
			size_t size)
{
  char *data_str;

  data_str = malloc( size+1);
  if( !data_str) {
    fprintf( stderr, "out of memory.\n");
    return ser_ok;
  }

  memcpy( data_str, data, size);
  data_str [size] = '\0';

  printf( "        Level %u [%s] (%lu bytes): \"%s\"\n", level,
	  mdStringID( file_id),
	  (unsigned long) size,
	  truncate_string( data_str));

  free( data_str);

  return ser_ok;
}



ser_return_t new_link( md_id_t parent, md_id_t link_id, const char *name,
		       md_attr *attr, const char *data, size_t size)
{
  char parent_buf [25];

  char *data_str;

  data_str = malloc( size+1); /* +1 for '\0' */

  if( !data_str) {
    fprintf( stderr, "out of memory\n");
    return ser_ok;
  }

  memcpy( data_str, data, size);
  data_str [size] = '\0';

  printf( "    Link [%s, %s]: %s  \"%s\"\n",
	  mdStringID( link_id),
	  mdStringID_r( parent, parent_buf),
	  name,
	  truncate_string( data_str));

  free( data_str);

  return ser_ok;
}



#define ELLIPSIS "[...]"

/**
 *  Quick-n-dirty hack to get a useful string.
 */
char *truncate_string( const char *data)
{
  static char buffer[60];
  char *p;

  strncpy( buffer, data, sizeof( buffer)-1);
  buffer[sizeof( buffer)-1] = '\0';

  while( (p = strchr( buffer, '\n')))
    *p = ' ';

  if( strlen( data) > sizeof(buffer)-1)
    strcpy( buffer + (sizeof(buffer)-sizeof(ELLIPSIS))-1, ELLIPSIS);

  return buffer;
}


