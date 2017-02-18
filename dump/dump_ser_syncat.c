/**
 *  Syncat XML serialiser: provide information for catalogue
 *  synchronisation.
 *
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>

#include <time.h> // NEEDED BY MD2PTYPES
#include "md2ptypes.h"

#include "genx.h"

#include "dump_version.h"
#include "dump_serialiser.h"
#include "dump_utils.h"

#include "dump_ser_abs_file.h"
#include "dump_ser_syncat.h"

#define SYNCAT_NS (constUtf8)"http://www.dcache.org/PROPOSED/2008/01/Cat"

#define OUTPUT_FLAG_WITH_SIZE     0x01
#define OUTPUT_FLAG_WITH_ATIME    0x02
#define OUTPUT_FLAG_WITH_CTIME    0x04
#define OUTPUT_FLAG_WITH_MTIME    0x08
#define OUTPUT_FLAG_WITH_CHECKSUM 0x10
#define OUTPUT_FLAG_WITH_AL_RP    0x20

#define OUTPUT_FLAGS_REQ_LVL2    (OUTPUT_FLAG_WITH_AL_RP | OUTPUT_FLAG_WITH_CHECKSUM)

#define COMMENT_PREFIX   " Generated with command: "
#define COMMENT_VERSION  " Generated using pnfsDump v" PNFSDUMP_VERSION_STR " "

static constUtf8 access_latency_str[] = {
  [l2_al_online]   = (constUtf8) "online",
  [l2_al_nearline] = (constUtf8) "nearline",
};

static constUtf8 retention_policy_str[] = {
  [l2_rp_replica]   = (constUtf8) "replica",
  [l2_rp_custodial] = (constUtf8) "custodial",
};



static ser_return_t begin_serialisation( const char *cmdline, int argc, char *argv[]);
static ser_return_t end_serialisation();

static ser_return_t begin_dump( md_id_t root_id);
static ser_return_t end_dump( md_id_t root_id);


static ser_return_t begin_dir( md_id_t parent_dir_id, md_id_t id,
			       const char *name, md_attr *attr);
static ser_return_t end_dir( md_id_t parent_dir_id, md_id_t id, const char *name);

static ser_return_t begin_file( md_id_t parent_dir_id, md_id_t id, const char *name,
				filesize_t size, lvl2_info_t *lvl2, md_attr *attr);



static const char *lookup_algorithm_name( checksum_type_t type);
static const char *normalise_value( const char *value);
static constUtf8 time_to_string( time_t t);
static constUtf8 longlong_to_string( unsigned long long data);


static const char *canonical_algorithm_name[] = {
  "BROKEN",
  "adler32",
  "md5",
};


static dump_serialiser_t info = {
  "generic XML format: [-p <prefix>] [-s] [-a] [-c] [-m] [-k] [-r] [ <VO1> [ <VO2> ...] ]",
  FLAGS_REQ_DEFAULT,
  begin_serialisation, end_serialisation,
  begin_dump, end_dump,
  begin_dir, end_dir,
  NULL, NULL, NULL, NULL, /* Ignore all tag information */
  begin_file, NULL,       /* Ignore end file */
  NULL,                   /* Ignore all levels */
  NULL,                   /* Ignore all links */
};

static char *prefix;

static int flags;

/**
 *  Globals needed for Syncat XML writing.
 */

static genxWriter writer;
static genxNamespace syncat_ns;
static genxElement el_entry, el_size, el_last_accessed, el_created, el_last_modified, el_checksum;
static genxElement el_access_latency, el_retention_policy;
static genxAttribute at_name, at_algorithm;


static char *hex_digits = "0123456789abcdef";

/**
 *  Our registry call-back
 */

dump_serialiser_t *syncat_get_serialiser()
{
  return &info;
}


/**
 *  Call-backs for serialisation.
 */

ser_return_t begin_serialisation( const char *cmdline,
				  int argc, char *argv[])
{
  genxStatus status;
  time_t now;
  char *p, *voname, *comment_str;
  size_t len;
  int i, c;

  optind = 1;  /* Reset getopt() state */

  while( (c = getopt( argc, argv, "+:p:sacmkr")) != -1) {
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

    case 's':
      flags ^= OUTPUT_FLAG_WITH_SIZE;
      break;

    case 'a':
      flags ^= OUTPUT_FLAG_WITH_ATIME;
      break;

    case 'c':
      flags ^= OUTPUT_FLAG_WITH_CTIME;
      break;

    case 'm':
      flags ^= OUTPUT_FLAG_WITH_MTIME;
      break;

    case 'k':
      flags ^= OUTPUT_FLAG_WITH_CHECKSUM;
      info.flags ^= FLAGS_REQ_LEVEL2;
      break;

    case 'r':
      flags ^= OUTPUT_FLAG_WITH_AL_RP;
      info.flags ^= FLAGS_REQ_LEVEL2;
      break;

    case ':':
      fprintf( stderr, "Option -%c requires an argument\n", optopt);
      return ser_error;

    case '?':
      fprintf( stderr, "Unknown output option: -%c\n", optopt);
      return ser_error;
    }

    if( ((info.flags & FLAGS_REQ_LEVEL2) ? 1 : 0) !=
	((flags & OUTPUT_FLAGS_REQ_LVL2) ? 1 : 0))
      info.flags ^= FLAGS_REQ_LEVEL2;
  }


  writer = genxNew(NULL, NULL, NULL);

  if( !(syncat_ns = genxDeclareNamespace(writer, SYNCAT_NS, (constUtf8)"", &status)))
    goto err_exit;


  if( !(el_entry = genxDeclareElement( writer, syncat_ns, (constUtf8)"entry", &status)))
    goto err_exit;

  if( !(el_size = genxDeclareElement( writer, syncat_ns, (constUtf8)"size", &status)))
    goto err_exit;

  if( !(el_last_accessed = genxDeclareElement( writer, syncat_ns, (constUtf8)"last-accessed", &status)))
    goto err_exit;

  if( !(el_created = genxDeclareElement( writer, syncat_ns, (constUtf8)"created", &status)))
    goto err_exit;

  if( !(el_last_modified = genxDeclareElement( writer, syncat_ns, (constUtf8)"last-modified", &status)))
    goto err_exit;

  if( !(el_checksum = genxDeclareElement( writer, syncat_ns, (constUtf8)"checksum", &status)))
    goto err_exit;

  if( !(el_access_latency = genxDeclareElement( writer, syncat_ns, (constUtf8)"access-latency", &status)))
    goto err_exit;

  if( !(el_retention_policy = genxDeclareElement( writer, syncat_ns, (constUtf8)"retention-policy", &status)))
    goto err_exit;



  if( !(at_name = genxDeclareAttribute( writer, NULL, (constUtf8)"name", &status)))
    goto err_exit;

  if( !(at_algorithm = genxDeclareAttribute( writer, NULL, (constUtf8)"algorithm", &status)))
    goto err_exit;


  if( genxStartDocFile( writer, stdout))
    goto err_exit;


  comment_str = malloc( strlen( cmdline) + sizeof( COMMENT_PREFIX) + 2); /* +2 for final " \0" */
  if( comment_str) {
    strcpy( comment_str, COMMENT_PREFIX);
    strcat( comment_str, cmdline);
    strcat( comment_str, " ");

    genxComment( writer, (constUtf8) comment_str);
    free( comment_str);
  }

  genxComment( writer, (constUtf8) COMMENT_VERSION);

  genxStartElementLiteral( writer, SYNCAT_NS, (constUtf8) "cat");

  genxStartElementLiteral( writer, SYNCAT_NS, (constUtf8) "dump");
  genxAddAttributeLiteral( writer, NULL, (constUtf8) "recorded",
			   time_to_string( time( &now)));

  for( i = optind; i < argc; i++) {
    genxStartElementLiteral( writer, SYNCAT_NS, (constUtf8) "for");
    voname = strdup( argv[i]);
    for(p=voname; *p; p++)
    	if( isupper( *p))
    		*p = tolower( *p);
    genxAddText( writer, (constUtf8) "vo:");
    genxAddText( writer, (constUtf8) voname);
    free( voname);
    genxEndElement( writer); // </for>
  }

  return ser_ok;


 err_exit:
  fprintf(stderr, "oops %s\n", genxLastErrorMessage(writer));
  return ser_error;
}


ser_return_t end_serialisation()
{
  genxEndElement( writer); // </dump>
  genxEndElement( writer); // </cat>

  genxEndDocument( writer);

  genxDispose( writer);

  if( prefix) {
    abs_filename_remove_from( prefix);

    if( abs_filename_strlen() != 0)
      fprintf( stderr, "Final abs. filename length unexpectidly not-zero: %ld\n",
	       abs_filename_strlen());
  }

  abs_filename_flush();

  printf( "\n");

  return ser_ok;
}


ser_return_t begin_dump( md_id_t root_id)
{
  genxStartElementLiteral( writer, SYNCAT_NS, (constUtf8) "entry-set");

  return ser_ok;
}

ser_return_t end_dump( md_id_t root_id)
{
  genxEndElement( writer);
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


ser_return_t begin_file( md_id_t parent_dir_id, md_id_t id, const char *name,
			 filesize_t size, lvl2_info_t *lvl2, md_attr *attr)
{
  md_unix *unix_attr = &attr->unixAttr;
  const char *alg_name, *checksum_value;
  checksum_value_t *this_checksum;
  access_latency_value_t al;
  retention_policy_value_t rp;

  genxStartElement( el_entry); // <entry>

  abs_filename_append_to( name);
  genxAddAttribute( at_name, (constUtf8)abs_filename_value());
  abs_filename_remove_from( name);

  if( flags & OUTPUT_FLAG_WITH_SIZE && unix_attr->mst_size != md_no_size) {
    genxStartElement( el_size); // <size>
    genxAddText( writer, longlong_to_string( size));
    genxEndElement( writer);    // </size>
  }

  if( flags & OUTPUT_FLAG_WITH_ATIME && unix_attr->mst_atime != md_no_time) {
    genxStartElement( el_last_accessed);
    genxAddText( writer, time_to_string( unix_attr->mst_atime));
    genxEndElement( writer); // </last-accessed>
  }

  if( flags & OUTPUT_FLAG_WITH_CTIME && unix_attr->mst_ctime != md_no_time) {
    genxStartElement( el_created);
    genxAddText( writer, time_to_string( unix_attr->mst_ctime));
    genxEndElement( writer); // </created>
  }

  if( flags & OUTPUT_FLAG_WITH_MTIME && unix_attr->mst_mtime != md_no_time) {
    genxStartElement( el_last_modified);
    genxAddText( writer, time_to_string( unix_attr->mst_mtime));
    genxEndElement( writer); // </last-modified>
  }

  if( flags & OUTPUT_FLAG_WITH_CHECKSUM && lvl2 && lvl2->flags & LVL2_FLAG_HAVE_CHKSUM) {

    for( this_checksum = lvl2->checksums; this_checksum->value; this_checksum++) {

      if( !(checksum_value = normalise_value( this_checksum->value)))
	continue;

      genxStartElement( el_checksum);

      if( (alg_name = lookup_algorithm_name( this_checksum->type)))
	genxAddAttribute( at_name, (constUtf8)alg_name);

      genxAddText( writer, (constUtf8)checksum_value);

      genxEndElement( writer); // </checksum>
    }
  }

  if( flags & OUTPUT_FLAG_WITH_AL_RP) {
    if( deduce_file_al( parent_dir_id, id, lvl2, &al)) {
      genxStartElement( el_access_latency);
      genxAddText( writer, access_latency_str[al]);
      genxEndElement( writer); // </access-latency>
    }

    if( deduce_file_rp( parent_dir_id, id, lvl2, &rp)) {
      genxStartElement( el_retention_policy);
      genxAddText( writer, retention_policy_str[rp]);
      genxEndElement( writer); // </retention-policy>
    }
  }

  genxEndElement( writer); // </entry>

  return ser_ok;
}




constUtf8 time_to_string( time_t t)
{
  static char buffer [25];
  struct tm tm;

  strftime( buffer, sizeof( buffer), "%Y-%m-%dT%H:%M:%SZ", gmtime_r( &t, &tm));

  return (constUtf8) buffer;
}


constUtf8 longlong_to_string( unsigned long long data)
{
  static char buffer[25];

  snprintf( buffer, sizeof( buffer), "%llu", data);

  return (constUtf8) buffer;
}



/**
 *  Return the canonical SynCat for the algorithm, or NULL if type is
 *  out-of-bounds.
 */
const char *lookup_algorithm_name( checksum_type_t type)
{
  if( type < FIRST_CHECKSUM_TYPE_VALUE || type > LAST_CHECKSUM_TYPE_VALUE)
    return NULL;
 
  return canonical_algorithm_name [type];
}


/**
 *  Normalised value of the checksum value.
 *
 *  Returns the normalised value or NULL if there was some problem.
 */
const char *normalise_value( const char *value)
{
  const char *p;
  static char buffer[1024];
  char *b;

  // Skip over any initial white-space
  for( p = value; *p && isspace( *p); p++);

  // Copy across what's left into static buffer
  strncpy( buffer, p, sizeof( buffer));
  buffer [sizeof( buffer)-1] = '\0';

  if( buffer[0] == '\0') {
    fprintf( stderr, "Empty checksum\n");
    return NULL;
  }

  // Normalise characters
  for( b = buffer; *b; b++) {

    if( isspace( *b)) {
      *b = '\0';
      break;
    }
    
    if( isupper( *b))
      *b = tolower( *b);

    if( !strchr( hex_digits, *b)) {
      fprintf( stderr, "Illegal character ('%c') in checksum\n", *b);
      return NULL;
    }
    
  }

  return buffer;
}
