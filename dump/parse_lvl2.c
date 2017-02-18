#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>

#include "parse_lvl2.h"

#define LVL2_STD_ERR_PREFIX     "Failed to parse level-2 metadata: "
#define LVL2_STD_WARNING_PREFIX "Warning: "

#define VALID_HEX_DIGITS     "0123456789abcdef" 

static size_t expected_chksum_size[] = {
  [chksum_adler32] =  8,   //  "328ace33"
  [chksum_md5]     =  32,  //  "9e107d9d372bb6826bd81d3542a419d6"
  [chksum_md4]     =  32,  //  "6df23dc03f9b54cc38a0fc1483df6e21"
};

static const char *chksum_name[] = {
  [chksum_adler32] =  CHKSUM_ADLER32_UC_NAME,
  [chksum_md5]     =  CHKSUM_MD5_UC_NAME,
  [chksum_md4]     =  CHKSUM_MD4_UC_NAME,
};


const char *l2_al_text[] = {
  [l2_al_online] = "ONLINE",
  [l2_al_nearline] = "NEARLINE",
};

const char *l2_rp_text[] = {
  [l2_rp_replica] = "REPLICA",
  [l2_rp_custodial] = "CUSTODIAL",
};


static char *extract_keyvalue_data( const char *data, size_t data_len);
static ssize_t extract_single_line( char *storage, size_t storage_size,
				    const char *text, size_t text_size);

/* Prototype fns for parsing the value of a keyword-value pair */
static int parse_c_checksum( lvl2_info_t *info, char *data);
static int parse_uc_checksums( lvl2_info_t *info, char *data);
static int parse_uc_single_checksum( lvl2_info_t *info, char *data);
static int parse_length( lvl2_info_t *info, char *data);
static int parse_access_latency( lvl2_info_t *info, char *value);
static int parse_retention_policy( lvl2_info_t *info, char *value);
static int parse_hsm_pool( lvl2_info_t *info, char *value);
static char *truncated_string( const char *data);
static int add_checksum( lvl2_info_t *info, checksum_type_t type,
			  char *chksum_data);


/**
 *  Reset structure to known-good values.  This should only be used
 *  once, thereafter lvl2_flush should be used to avoid memory
 *  leaking.
 */
void lvl2_reset( lvl2_info_t *info)
{
  if( !info)
    return;

  memset( info, 0, sizeof( lvl2_info_t));

  lvl2_flush( info);
}


/**
 *  Parse a file's level-2 metadata, filling out the relevant
 *  information.
 *
 *  Madness in choice of C1 token checksum separator requires a
 *  stateful (and look-ahead) parser.
 *
 *  A simple context may be provided.  This string will be included in
 *  warning messages, those messages where 1 (success) is returned.
 *
 *  Returns 1 on success, 0 on failure.
 */
int lvl2_parse_data( lvl2_info_t *info, const char *data, size_t data_size,
		     const char *context)
{
  char *str, *token, *saveptr=NULL;
  char *equals, *value, *keyvalue_data;
  char *context_str;
  int dealing_with_c1=0;

  if( context) {
    context_str = malloc( strlen( context)+4);
    context_str [0] = '[';
    context_str [1] = '\0';
    strcat( context_str, context);
    strcat( context_str, "] ");
  } else {
    context_str = "";
  }

  info->data = malloc( data_size+1); /* +1 for our '\0' */
  
  if( !info->data) {
    fprintf( stderr, LVL2_STD_ERR_PREFIX "out of memory.\n");
    info->data = NULL;
    goto err_exit;
  }

  memcpy( (void *) info->data, data, data_size);
  ((char *) info->data) [data_size] = '\0';
  *((size_t *) &info->data_size) = data_size;

  keyvalue_data = extract_keyvalue_data( data, data_size);

  if( !keyvalue_data)
    goto err_exit;

  /* tokenise, based on ';' characters. */
  for( str = keyvalue_data; ; str = NULL) {
    token = strtok_r(str, ";", &saveptr);

    if( token == NULL)
      break;
    
    equals = strchr( token, '=');

    /* If we have no equals symbol */
    if( equals == NULL) {
      if( dealing_with_c1) {
	if( !parse_c_checksum( info, token))
	  goto err_exit;
      } else {
	fprintf( stderr, LVL2_STD_WARNING_PREFIX "%smissing equals in token \"%s\".\n",
		 context_str, token);
      }

      continue;
    }

    dealing_with_c1 = 0;

    *equals = '\0';
    value = equals+1;

    if( !strcmp( token, "l")) {

      if( !parse_length( info, value))
	goto err_exit;

    } else if( !strcmp( token, "c")) {

      if( !parse_c_checksum( info, value))
	goto err_exit;

    } else if( !strcmp( token, "uc")) {

      if( !parse_uc_checksums( info, value))
	goto err_exit;

    } else if( !strcmp( token, "c1")) {
      if( !parse_c_checksum( info, value))
	goto err_exit;

      dealing_with_c1 = 1;
    } else if( !strcmp( token, "al")) {
      if( !parse_access_latency( info, value))
	goto err_exit;

    } else if( !strcmp( token, "rp")) {
      if( !parse_retention_policy( info, value))
	goto err_exit;

    } else if( !strcmp( token, "h")) {
      if( !parse_hsm_pool( info, value))
	goto err_exit;

    } else if( !strcmp( token, "s")) {
      /* Silently ignore any s= flags */
    } else if( !strcmp( token, "d")) {

      /**
       *  Silently ignore any d= flags.  These were used to indicate
       *  whether a file may be deleted from the namespace when the
       *  last disk copy is removed.
       *
       *  Support for this in dCache has now been removed.
       */
    } else {
      fprintf( stderr, LVL2_STD_WARNING_PREFIX
	       "%sunknown token \"%s\" (with value \"%s\") in level-2 metadata.\n",
	       context_str, token, value);
    }
  }

  if( context_str[0] != '\0')
    free( context_str);

  return 1; // SUCCESS

 err_exit:
  if( context_str[0] != '\0')
    free( context_str);

  lvl2_flush( info);
  return 0;
}

/**
 *  Parse an integer.
 *
 *  Return 1 on success, 0 on failure.
 */
int parse_length( lvl2_info_t *info, char *data)
{
  char *end;
  long long length;

  if( *data == '\0') {
    fprintf( stderr, LVL2_STD_ERR_PREFIX "missing data for length.\n");
    return 0;
  }

  length = strtoll( data, &end, 10);

  if( *end != '\0') {
    fprintf( stderr, LVL2_STD_ERR_PREFIX "length data is misformed \"%s\".\n",
	     data);
    return 0;
  }

  if( length < 0) {
    fprintf( stderr, LVL2_STD_ERR_PREFIX "negative length detected \"%s\".\n",
	     data);
    return 0;
  }

  info->length = length;
  info->flags |= LVL2_FLAG_HAVE_LENGTH;

  return 1;
}



/**
 *  Parse an al flag value.  This should be either ONLINE or NEARLINE.
 *
 *  Return 1 on success, 0 on failure.
 */
int parse_access_latency( lvl2_info_t *info, char *value)
{
  access_latency_value_t al_value;

  if( !strcmp( value, "ONLINE"))
    al_value = l2_al_online;
  else if( !strcmp( value, "NEARLINE"))
    al_value = l2_al_nearline;
  else {
    fprintf( stderr, LVL2_STD_ERR_PREFIX "unknown access-latency (\"al\") value (\"%s\")\n", value);
    return 0; /* failure */
  }

  if( info->flags & LVL2_FLAG_HAVE_AL && info->access_latency != al_value) {
    fprintf( stderr, LVL2_STD_ERR_PREFIX "duplicate access-latency has different value (%u != %u)\n",
	     info->access_latency, al_value);
    return 0; /* Failure */
  }

  info->flags |= LVL2_FLAG_HAVE_AL;
  info->access_latency = al_value;

  return 1; /* Success */
}


/**
 *  Parse a rp flag value.  This should be either REPLICA or CUSTODIAL
 */
int parse_retention_policy( lvl2_info_t *info, char *value)
{
  retention_policy_value_t rp_value;

  if( !strcmp( value, "REPLICA"))
    rp_value = l2_rp_replica;
  else if( !strcmp( value, "CUSTODIAL"))
    rp_value = l2_rp_custodial;
  else {
    fprintf( stderr, LVL2_STD_ERR_PREFIX "unknown retention-policy (\"rp\") value (\"%s\")\n", value);
    return 0; /* Failure */
  }

  if( info->flags & LVL2_FLAG_HAVE_RP && info->retention_policy != rp_value) {
    fprintf( stderr, LVL2_STD_ERR_PREFIX "duplicate retention-policy has different value (%u != %u)\n",
	     info->retention_policy, rp_value);
    return 0; /* Failure */
  }

  info->flags |= LVL2_FLAG_HAVE_RP;
  info->retention_policy = rp_value;

  return 1; /* Success */
}



/**
 *  Parse an h flag.  This marks whether a file was first written to a
 *  pool that was (at that time) attached to an HSM.  The possible
 *  values are "yes" or "no".
 *
 *  Returns 1 on success, 0 on failure.
 */
int parse_hsm_pool( lvl2_info_t *info, char *value)
{
  hsm_pool_value_t hsm_value;


  if( !strcmp( value, "yes"))
    hsm_value = l2_hsm_pool_with_hsm;
  else if( !strcmp( value, "no"))
    hsm_value = l2_hsm_pool_without_hsm;
  else {
    fprintf( stderr, LVL2_STD_ERR_PREFIX "unknown hsm-pool (\"h\") value (\"%s\")\n", value);
    return 0; /* Failure */
  }

  if( info->flags & LVL2_FLAG_HAVE_HSM && info->hsm_pool != hsm_value) {
    fprintf( stderr, LVL2_STD_ERR_PREFIX "duplicate hsm flag (\"h\") has different value (%u != %u)\n",
	     info->hsm_pool, hsm_value);
    return 0; /* Failure */
  }

  info->flags |= LVL2_FLAG_HAVE_HSM;
  info->hsm_pool = hsm_value;

  return 1; /* Success */

}



/**
 *  Parse a uc flag value.  This is a comma-separated list of checksum
 *  values and have the form:
 *
 *       <chksum entry> == <chksum_name>:<chksum_value>
 *       data = <chksum entry>[,<chksum entry>, [...]]
 *
 *  Return 1 on success, 0 on failure.
 */
int parse_uc_checksums( lvl2_info_t *info, char *data)
{
  char *saveptr=NULL, *str, *token;

  for( str = data; ; str = NULL) {
    token = strtok_r(str, ",", &saveptr);

    if( !token)
      break;

    if( !parse_uc_single_checksum( info, token))
      return 0;
  }

  return 1;
}


/**
 *  Parse a single uc entry.  These have the form:
 *
 *       <chksum entry> == <chksum_name>:<chksum_value>
 *
 *  Return 1 on success, 0 on failure.
 */
int parse_uc_single_checksum( lvl2_info_t *info, char *data)
{
  char *colon;
  int this_type, found_type=0;
  checksum_type_t type;

  colon = strchr( data, ':');

  if( !colon || colon == data) {
    fprintf( stderr, LVL2_STD_ERR_PREFIX "missing colon in checksum data \"%s\"\n", data);
    return 0;
  }

  *colon = '\0';

  /* Try to identify the checksum type. */
  for( this_type = 0; this_type < sizeof( chksum_name) / sizeof( const char *); this_type++) {
    // This skips any "holes" in our array.
    if( !chksum_name [this_type])
      continue;

    if( !strcmp( data, chksum_name [this_type])) {
      type = this_type;
      found_type = 1;
    }
  }

  if( !found_type) {
    fprintf( stderr, LVL2_STD_ERR_PREFIX "unknown checksum prefix \"%s\"\n", data);
    return 0;
  }

  return add_checksum( info, type, colon+1);
}


/**
 *  Parse a chksum value from a c or c1 tag.  These have the form:
 *
 *     <chksum_type_number>:<chksum_value>
 *
 *  Return 1 on success, 0 on failure.
 */
int parse_c_checksum( lvl2_info_t *info, char *data) 
{
  char *colon, *end;
  checksum_type_t type;

  colon = strchr( data, ':');

  if( !colon || colon == data) {
    fprintf( stderr, LVL2_STD_ERR_PREFIX "misplaced colon in checksum data \"%s\"\n", data);
    return 0;
  }

  *colon = '\0';

  type = strtol( data, &end, 10);

  if( end == data || *end != '\0') {
    fprintf( stderr, LVL2_STD_ERR_PREFIX "malformed checksum type \"%s\"\n",
	     data);
    return 0;
  }
 
  return add_checksum( info, type, colon+1);
}


/**
 *  Apply some final checks on the checksum and add it to the level-2
 *  stack of checksums.
 *
 *  Returns 1 on success, 0 on error.
 */
int add_checksum( lvl2_info_t *info, checksum_type_t type, char *chksum_data)
{
  int req_size, i;
  char *c;

  if( type < FIRST_CHECKSUM_TYPE_VALUE || type > LAST_CHECKSUM_TYPE_VALUE) {
    fprintf( stderr, LVL2_STD_ERR_PREFIX "illegal checksum type %u in checksum data\n",
	     type);
    return 0;
  }


  if( strlen( chksum_data) != expected_chksum_size [type]) {
    fprintf( stderr,
	     LVL2_STD_ERR_PREFIX "checksum data is wrong length for %s checksum value (%u != %u)\n",
	     chksum_name[type], strlen( chksum_data), expected_chksum_size [type]);
    return 0;
  }


  /**
   *  Check that the checksum value is in hex and convert to lower-case.
   */
  for( c = chksum_data; *c != '\0'; c++) {
    if( isupper( *c))
      *c = tolower( *c);

    if( !strchr( VALID_HEX_DIGITS, *c)) {
      fprintf( stderr, LVL2_STD_ERR_PREFIX "illegal char 0x%02x in checksum data (\"%s\")\n",
	       *c, chksum_data);
      return 0;
    }
  }

  /**
   *  Check that the checksum value for this type is not already
   *  specified.  If it is, check the values agree.  If not, remove
   *  the checksum and report an error.
   */
  for( i = 0; i < info->checksum_count; i++) {

    /* Skip different checksum values */
    if( info->checksums[i].type != type)
      continue;

    /* If the values don't match... */
    if( strcmp(info->checksums [i].value, chksum_data)) {

      fprintf( stderr,
	       LVL2_STD_ERR_PREFIX "stored multiple checksum values for %s that don't match (%s != %s)\n",
	       chksum_name [type], chksum_data,
	       info->checksums [i].value);

      /* Move rest of stack down one: remember the empty checksum_value_t! */
      free( (char *)info->checksums [i].value);
      memmove( &info->checksums [i], &info->checksums [i+1],
	       (info->checksum_count-i) * sizeof( checksum_value_t));

      info->checksum_count--;

      return 0; /* Error */
    }

    return 1; /* OK, but we don't actually store the extra chksum */
  }


  /**
   *  Finally, add the checksum to the level-2 data
   */

  info->checksum_count++;

  /**
   *  We need +1 for storage necessary for empty checksum data that
   *  terminates the array.
   */
  req_size = info->checksum_count +1;

  if( req_size > info->checksum_stack_count) {
    info->checksum_stack = realloc( info->checksum_stack,
				     req_size * sizeof( checksum_value_t));
    if( !info->checksum_stack) {
      fprintf( stderr, "out of memory.\n");
      info->checksum_stack_count = 0;
      return 0;
    }

    info->checksum_stack_count = req_size;
  }

  info->checksums = info->checksum_stack;

  info->checksums [info->checksum_count-1].value = strdup( chksum_data);
  info->checksums [info->checksum_count-1].type = type;

  info->checksums [info->checksum_count].value = NULL;
  info->checksums [info->checksum_count].type = 0;

  info->flags |= LVL2_FLAG_HAVE_CHKSUM;

  return 1;
}




/**
 *  Level-2 metadata contains metadata about a file.  There are two versions
 *  of the file format: version 1 and version 2.
 *
 *  Both versions start with a single line:
 *
 *    <version> ',' <cache statistics> '\n'
 *
 *  <version> is either '1' or '2'.
 *
 *  The format of <cache statitics> is (see [1]):
 *
 *    <totalAccesses> ',' <accessTime> ',' <score> ',' <halfLife>
 *
 *  These numbers most likely all zero: <totalAccess> and <accessTime>
 *  are integers, <score> and <halfLife> are floating-point numbers.
 *
 *  Version 1 follows this initial line with zero or more pool names,
 *  each indicating the location of the file.
 *
 *  Version 2 has a more baroque format.  After the initial first line
 *  there are zero or more lines starting with a colon.  After this
 *  there are zero or more lines, each line naming a pool on which the
 *  file is located.  If a companion database is being used, then this
 *  list of pools will always be empty.
 *
 *  The overall format for version-2 is (see [2]):
 *
 *    <version> ',' <cache statistics> '\n'
 *    ':' <cache flags>  '\n'
 *   [<cache location>   '\n']*n  (for 0 <= n)
 *  
 *
 * The format of <cache flags> is (see [3]):
 *
 * ':' [<keyword> '=' <value> ';']*n (for 0 <= n)
 *
 * with the complication that if, after appending a keyword-value
 * pair, a line exceeds 70 then a
 *
 *  '\n' ':'
 *
 * sequence is introduced; i.e., the following keyword-value pairs
 * appear on new line that starts with a colon.
 *
 * This routine returns just the set of keyword-value pairs without
 * any line-breaks; that is, a string containing only <cache flags>
 * and where keyword-value pairs are separated by ';' and everything
 * is on one line.
 *
 * For version-1 data, this is trivially an empty string ("").
 *
 * [1] - toPnfsString() modules/dCache/diskCacheV111/vehicles/CacheStatistics.java
 * [2] - writeCacheInfo(PnfsFile pnfsFile) modules/dCache/diskCacheV111/vehicles/CacheInfo.java
 * [3] - toPnfsString() modules/dCache/diskCacheV111/vehicles/CacheInfo.java
 *
 * Returns a pointer to a static buffer on success, NULL on an error.
 */
char *extract_keyvalue_data( const char *data, size_t data_len)
{
  static char buffer[1024];
  char single_line[200], *comma;
  size_t offset=0;
  ssize_t appended;
  int version, have_valid_colon_line=0;


  /**
   *  First, extract the first line and parse the format version number.
   */
  appended = extract_single_line( buffer, sizeof( buffer), data, data_len);

  if( appended < 0) {
    fprintf( stderr, LVL2_STD_ERR_PREFIX "no newline in \"%s\"\n", data);
    return NULL;
  }

  offset += appended +1; // +1 to skip over '\n'

  if( !(comma = strchr( buffer, ',')))
    goto formatting_error;

  if( comma == buffer)
    goto formatting_error;

  *comma = '\0';

  version = atoi( buffer);

  if( version != 1 && version != 2) {
    fprintf( stderr, LVL2_STD_ERR_PREFIX "bad version %d\n", version);
    return NULL;
  }

  /**
   *  For version 1, no further parsing is possible
   */
  if( version == 1)
    return "";

  // Reset our buffer.
  buffer [0] = '\0';


  /**
   *  Next, reformat the keyword-value pairs to form a long line.
   */

  // Build a line by looping over lines that start ":"
  for(;;) {
    if( offset >= data_len || data[offset] != ':')
      break;

    appended = extract_single_line( single_line, sizeof( single_line), data + offset, data_len - offset);

    if( appended < 0) {
      fprintf( stderr, LVL2_STD_ERR_PREFIX "no new-line whilst parsing level-2 metadata key-vale pairs.\n");
      goto formatting_error;
    }

    offset += appended +1; // +1 to skip over '\n'

    if( strlen( buffer) + appended >= sizeof( buffer)) {
      fprintf( stderr, LVL2_STD_ERR_PREFIX "key-value pairs too long.\n");
      goto formatting_error;
    }
      
    strcat( buffer, single_line +1); // +1 to skip over ':'

    have_valid_colon_line = 1;
  }

  if( !have_valid_colon_line) {
    fprintf( stderr, LVL2_STD_ERR_PREFIX "missing any lines starting with a colon.\n");
    goto formatting_error;
  }

  // Finally ... done!
  return buffer;

 formatting_error:
  fprintf( stderr, LVL2_STD_ERR_PREFIX "data is badly formatted.\n");
  return NULL;
}


/**
 *   Append a line's worth of text to storage to the end of storage.
 *
 *   Ruturns the number of bytes appended to storage or -1 if there
 *   was a problem.
 */
ssize_t extract_single_line( char *storage, size_t storage_size,
			     const char *text, size_t text_len)
{
  const char *nl;
  size_t line_len;

  nl = memchr( text, '\n', text_len);

  if( !nl)
    return -1;

  /* Length of line, not including the '\n' character */
  line_len = nl - text;

  if( line_len >= storage_size) {
    fprintf( stderr, LVL2_STD_ERR_PREFIX "key-value mapping line too long.\n");
    fprintf( stderr, "level-2 metadata is: \"%s\"\n", truncated_string(text));
  }

  strncpy( storage, text, line_len);
  storage [line_len] = '\0';

  return line_len;
}



/**
 *  Remove any cached data about file's metadata.
 */
void lvl2_flush( lvl2_info_t *info)
{
  int i;

  free( (void *) info->data);
  info->data = NULL;


  info->length = LVL2_DEFAULT_LENGTH;
  info->flags = LVL2_FLAG_HAVE_NOTHING;
  info->access_latency = LVL2_DEFAULT_AL;
  info->retention_policy = LVL2_DEFAULT_RP;
  info->hsm_pool = LVL2_DEFAULT_HSM;

  for( i = 0; i < info->checksum_count; i++) {
    free( (void *) info->checksums[i].value);
    info->checksums[i].value = NULL;
  }
  
  info->checksums = NULL;
  info->checksum_count = 0;
}


/**
 *  Remove all memory usage by a lvl2_info_t structure.  This is a
 *  more extreme version of lvl2_flush() that should be called just
 *  before a program terminates.  This is to help trance memory leaks
 *  via valgrind.
 *
 *  The structure *is* valid after calling this fn, but using it is
 *  sub-optimal.
 */
void lvl2_final_flush( lvl2_info_t *info)
{
  lvl2_flush( info);

  free( info->checksum_stack);
  info->checksum_stack_count = 0;
}



/**
 *  Return a potentially truncated string.
 */
#define ELLIPSIS "[...]"
char *truncated_string( const char *data)
{
  static char buffer[40];

  strncpy( buffer, data, sizeof(buffer)-sizeof(ELLIPSIS)-1);
  
  buffer [sizeof(buffer)-(sizeof(ELLIPSIS)+1)] = '\0';

  if( strlen( data) > sizeof( buffer) - (sizeof( ELLIPSIS)+1))
    strcat( buffer, ELLIPSIS);

  return buffer;
}
