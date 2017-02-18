/**
 *  Support for maintaining an absolute filename.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "dump_ser_abs_file.h"


static char *abs_filename;
static ssize_t abs_filename_len;
static size_t abs_filename_storage;

const char *abs_filename_value()
{
  return abs_filename ? abs_filename : "";
}


long abs_filename_strlen()
{
  return abs_filename_len;
}


/**
 *  Given a label (e.g., "foo"), append it to the abs_filename
 *  prepending a "/" character (e.g., "/foo").
 */
void abs_filename_append_to( const char *label)
{
  size_t append_size, new_storage_size;

  append_size = strlen( label) +1; // +1 for extra '/'

  new_storage_size = abs_filename_len + append_size +1; // +1 for '\0'

  if( new_storage_size > abs_filename_storage) { 
    abs_filename = realloc( abs_filename, new_storage_size);

    if( abs_filename_storage == 0)
      abs_filename[0] = '\0';

    abs_filename_storage = new_storage_size;
  }

  strcat( abs_filename, "/");
  strcat( abs_filename, label);

  abs_filename_len += append_size;
}


/**
 *  Shrink abs_filename by strlen(label)+1.  The +1 is for the
 *  preceeding '/' character.
 */
void abs_filename_remove_from( const char *label)
{
  size_t remove_size;

  remove_size = strlen( label) +1; // +1 for extra '/'

  if( remove_size <= abs_filename_len)
    abs_filename_len -= remove_size;
  else {
    fprintf( stderr, "attempting to remove too large a label\n");
    abs_filename_len = 0; // error recovery.
  }

  abs_filename [abs_filename_len] = '\0';
}


/**
 *  Free all memory used by absolute-filename support.
 */
void abs_filename_flush()
{
  free( abs_filename);
  abs_filename_storage = 0;
  abs_filename_len = 0;
}

