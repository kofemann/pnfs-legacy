#include <string.h>
#include <ctype.h>
#include <stdio.h>

#include "dump_utils.h"
#include "dump_cache.h"

#define FILE_DCACHECONFIG "/opt/d-cache/config/dCacheSetup"


/* These are dCache system defaults */
#define AL_SYSTEM_DEFAULT      l2_al_nearline
#define RP_SYSTEM_DEFAULT      l2_rp_custodial


/* The name of the tags used to define Access Latency and Retention Policy */
#define AL_TAG_NAME            "accesslatency"
#define RP_TAG_NAME            "retentionpolicy"


/* String values for different AL and RP values */
#define AL_VALUE_ONLINE    "ONLINE"
#define AL_VALUE_NEARLINE  "NEARLINE"
#define AL_VALUE_OFFLINE   "OFFLINE"
#define RP_VALUE_REPLICA   "REPLICA"
#define RP_VALUE_OUTPUT    "OUTPUT"
#define RP_VALUE_CUSTODIAL "CUSTODIAL"



/* Local functions that do the actual work */
inline static char hex_digit( int num, char a);
inline static void hex_long( char *str, long num, char a);
inline static void hex_short( char *str, short num, char a);
static int parse_dcachesetup( access_latency_value_t *system_default_al_p,
			      retention_policy_value_t *system_default_rp_p);
static void get_default_values( access_latency_value_t *al_p,
				retention_policy_value_t *rp_p);

static int parse_access_latency_value( const char *al_txt, access_latency_value_t *value_p,
				       const char *context);
static int parse_retention_policy_value( const char *rp_txt, retention_policy_value_t *value_p,
					 const char *context);


/**
 *  Best effort at converting a string containing a hex number into
 *  that number.
 *
 *  length is the expected length of the string: 0 implies unlimited.
 *
 *  Returns -1 if the hex value is invalid.
 */
int dump_parse_hex( const char *txt, size_t length)
{
  int val=0;
  const char *p, *h, *hex="0123456789abcdef";

  if( length && strlen( txt) != length)
    return -1;  

  for( p = txt; *p; p++) {
    h = strchr( hex, tolower(*p));
    if( !h)
      return -1;

    val *= 16;
    val += h-hex;
  }

  return val;
}


/**
 *  Pretty much the same as mdStringID / md2PrintID but allows the
 *  user to specify a buffer into which the ID will be stored.  The
 *  buffer must be at least 25 bytes in size.
 */
char * mdStringID_r( md_id_t id, char *str)
{
  hex_short( str+0,  id.db,   'A');
  hex_short( str+4,  id.ext,  'A');
  hex_long(  str+8,  id.high, 'A');
  hex_long(  str+16, id.low,  'A');
  str [24] = '\0';

  return str ;
}


/**
 *  This fn is copied from dbfs/md2lib.c because we don't want to link
 *  against md2lib.o
 */
char * md2PrintTypes( md_type_t t )
{
  unsigned long l ;
  int i ;
  static char str[mdtLast+2] ;

  l = t.low ;
  for( i = 0 ; i < (mdtLast+1) ; i++ )
     str[i] = ( l & (1<<i) ) ? mdtToString[i] : '-' ;

  return str ;
}



/**
 *  Write out a sequence of bytes as their hexadecimal string
 *  equivalent.  str must point to memory with at least 2*bytes_len+1
 *  writable.
 */
void utils_hex_bytes( char *str, unsigned char *bytes, size_t bytes_len, int uc)
{
  size_t i, idx;
  char a = uc ? 'A' : 'a';

  for (i = 0, idx=0; i < bytes_len; i++, idx+=2) {
    str [idx]   = hex_digit(bytes[i] >> 4,   a);
    str [idx+1] = hex_digit(bytes[i] & 0x0f, a);
  }

  str [i*2] = '\0';
}


void utils_hex_long( char *str, int num, int uc)
{
  hex_long( str, num, uc ? 'A' : 'a');
  str [8] = '\0';
}

void utils_hex_short( char *str, int num, int uc)
{
  hex_short( str, num, uc ? 'A' : 'a');
  str [4] = '\0';
}

void utils_hex_byte( char *str, int num, int uc)
{
  char a = uc ? 'A' : 'a';

  str [0] = hex_digit((num >> 4) & 0x0f, a);
  str [1] = hex_digit(num & 0x0f, a);
  str [2] = '\0';
}

/**
 *  Overwrite the first 9 bytes of str with the long integer num.  The
 *  parameter a should be either 'a' or 'A'.
 */
inline static void hex_long( char *str, long num, char a)
{
  str [0] = hex_digit((num >> 28) & 0x0f, a);
  str [1] = hex_digit((num >> 24) & 0x0f, a);
  str [2] = hex_digit((num >> 20) & 0x0f, a);
  str [3] = hex_digit((num >> 16) & 0x0f, a);
  str [4] = hex_digit((num >> 12) & 0x0f, a);
  str [5] = hex_digit((num >> 8) & 0x0f, a);
  str [6] = hex_digit((num >> 4) & 0x0f, a);
  str [7] = hex_digit(num & 0x0f, a);
}

/**
 *  Overwrite the first 5 bytes of str with the long integer num.  The
 *  parameter a should be either 'a' or 'A'.
 */
inline static void hex_short( char *str, short num, char a)
{
  str [0] = hex_digit((num >> 12) & 0x0f, a);
  str [1] = hex_digit((num >> 8) & 0x0f, a);
  str [2] = hex_digit((num >> 4) & 0x0f, a);
  str [3] = hex_digit(num & 0x0f, a);
}

/**
 *  Convert a number (0..15) into its equiv. hex digit character.  The
 *  parameter a should be either 'a' or 'A'.
 */
inline static char hex_digit( int num, char a)
{
  if( num < 10)
    return '0' + num;
  else
    return a + num - 10;
}





/**
 *  Given a file (in a given directory), establish the intended access-latency.
 *
 *  Return 1 on success, 0 on failure.
 */
int deduce_file_al( md_id_t dir_id, md_id_t file_id, lvl2_info_t *lvl2, access_latency_value_t *al_p)
{
  md_id_t tag_id;
  char tag_data[TAG_DATA_SIZE+1];
  char context[70], file_id_str[25];
  size_t tag_data_size;

  /* Try level-2 metadata, if we have any */
  if( lvl2 && lvl2->flags & LVL2_FLAG_HAVE_AL) {
    *al_p = lvl2->access_latency;
    return 1; /* Success */
  }

  /* Try directory tag */
  if( cache_get_tag_id( dir_id, AL_TAG_NAME,  &tag_id)) {
    if( !cache_find_tag_value( tag_id, tag_data, sizeof(tag_data), &tag_data_size))
      return 0; /* FAILED */

    snprintf( context, sizeof( context), "access-latency for file %s via tag",
	      mdStringID_r( file_id, file_id_str));
    context [ sizeof( context)-1] = '\0';

    if( !parse_access_latency_value( tag_data, al_p, context))
      return 0; /* FAILED */

    return 1; /* Success */
  }

  /* Use dCache default */
  get_default_values( al_p, NULL);
  
  return 1; /* Success */
}


/**
 *  Given a file (in a given directory), establish the intended retention-policy.
 *
 *  Return 1 on success, 0 on failure.
 */
int deduce_file_rp( md_id_t dir_id, md_id_t file_id, lvl2_info_t *lvl2, retention_policy_value_t *rp_p)
{
  md_id_t tag_id;
  char tag_data[TAG_DATA_SIZE+1];
  char context[70], file_id_str[25];
  size_t tag_data_size;

  /* Try level-2 metadata, if we have any */
  if( lvl2 && lvl2->flags & LVL2_FLAG_HAVE_RP) {
    *rp_p = lvl2->retention_policy;
    return 1;
  }

  /* Try to parse the directory tags */
  if( cache_get_tag_id( dir_id, RP_TAG_NAME,  &tag_id)) {

    if( !cache_find_tag_value( tag_id, tag_data, sizeof(tag_data), &tag_data_size))
      return 0; /* FAILED */

    snprintf( context, sizeof( context), "retention-policy for file %s via tag",
	      mdStringID_r( file_id, file_id_str));
    context [ sizeof( context)-1] = '\0';

    if( !parse_retention_policy_value( tag_data, rp_p, context))
      return 0; /* FAILED */

    return 1;
  }

  get_default_values( NULL, rp_p);

  return 1;
}



/**
 *  Update either the access-latency or retention-policy values with
 *  the system default.  This system default takes into account the
 *  values in dCacheSetup file (if one can be parsed).
 *
 *  Either al_p or rp_p may be NULL; both parameters may be NULL,
 *  which may result in dCacheSetup being parsed but has no other
 *  effect.
 */
void get_default_values( access_latency_value_t *al_p, retention_policy_value_t *rp_p)
{
  static int have_read_dcache;
  static access_latency_value_t system_default_al = AL_SYSTEM_DEFAULT;
  static retention_policy_value_t system_default_rp = RP_SYSTEM_DEFAULT;

  if( !have_read_dcache) { 
    have_read_dcache = 1; /* Do this only once */
    if( !parse_dcachesetup( &system_default_al, &system_default_rp))
      fprintf( stderr, "Unable to parse dCacheSetup file, assuming dCache default values (%s, %s)\n",
	       l2_rp_text [RP_SYSTEM_DEFAULT], l2_al_text [AL_SYSTEM_DEFAULT]);
  }

  if( al_p)
    *al_p = system_default_al;

  if( rp_p)
    *rp_p = system_default_rp;
}



/**
 *  Attempt to parse the dCache setup file.
 */
int parse_dcachesetup( access_latency_value_t *system_default_al_p,
		       retention_policy_value_t *system_default_rp_p)
{
  FILE *fp;
  char buffer[1024], *eq, *key, *value, *end;

  fp = fopen( FILE_DCACHECONFIG, "r");

  if( !fp) {
    perror( FILE_DCACHECONFIG);
    return 0;
  }

  while( !feof( fp) ) {

    if( !fgets( buffer, sizeof( buffer), fp)) {
      if( !feof(fp))
	perror( FILE_DCACHECONFIG);
      break;
    }

    /* Skip any initial white space */
    for( key = buffer; isspace( *key); key++);

    /* Skip comments */
    if( *key == '#')
      continue;

    /* Split at equals, skip if there is none. */
    if( !(eq = strchr( key, '=')))
      continue;
    *eq = '\0';

    /* Strip off any white space at the end of the key-word */
    for( end=eq-1; isspace( *end) && end > key; end--);
    end[1] = '\0';

    /* Skip any initial white space in value */
    for( value = eq+1; isspace( *value); value++);

    /* Strip off any white space at the end of the value */
    for( end = value + strlen( value)-2; isspace( *end) && end > value; end--);
    end[1] = '\0';

    if( !strcasecmp( key, "DefaultAccessLatency")) {
      
      parse_access_latency_value( value, system_default_al_p,
				  FILE_DCACHECONFIG " DefaultAccessLatency" );

    } else if( !strcasecmp( key, "DefaultRetentionPolicy")) {

      parse_retention_policy_value( value, system_default_rp_p,
				    FILE_DCACHECONFIG " DefaultRetentionPolicy");
    }
  }

  fclose( fp);

  return 1;
}





/**
 *  Parse an access-latency value.
 *
 *  Returns 1 on success, 0 on failure.
 */
int parse_access_latency_value( const char *al_txt, access_latency_value_t *value_p,
				const char *context)
{
  if( !strcasecmp( al_txt, AL_VALUE_ONLINE)) {

    *value_p = l2_al_online;
    return 1;

  } else if( !strcasecmp( al_txt, AL_VALUE_NEARLINE)) {

    *value_p = l2_al_nearline;
    return 1;

  } else if( !strcasecmp( al_txt, AL_VALUE_OFFLINE)) {

    fprintf( stderr, "Offline access-latency is not supported, substituting \"Nearline\".\n");
    *value_p = l2_al_nearline;
    return 1;
  }

  fprintf( stderr, "%s: unknown access-latency value \"%s\", should be \"Online\" or \"Nearline\".\n",
	   context, al_txt);
  return 0;
}


/**
 *  Parse an access-latency value and return the corresponding level-2 value.
 *  Returns 1 on success, 0 on failure.
 */
int parse_retention_policy_value( const char *rp_txt, retention_policy_value_t *value_p,
				  const char *context)
{
  if( !strcasecmp( rp_txt, RP_VALUE_REPLICA)) {
    
    *value_p = l2_rp_replica;
    return 1;

  } else if( !strcasecmp( rp_txt, RP_VALUE_OUTPUT)) {

    fprintf( stderr, "Output retention-policy is not supported, substituting \"Custodial\".\n");
    *value_p = l2_rp_custodial;
    return 1;

  } else if( !strcasecmp( rp_txt, RP_VALUE_CUSTODIAL)) {

    *value_p = l2_rp_custodial;
    return 1;
  }

  fprintf( stderr, "%s: unknown retention-policy value \"%s\", should be \"Replica\" or \"Custodial\".\n",
	   context, rp_txt);
  return 0;
}


