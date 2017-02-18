/**
 *  XML serialiser: dump name-space as XML.
 *
 *  TODO:
 *
 *   1.  GenX assumes that all text data is UTF-8.  For most data this
 *       is likely to be true, but what do we do if it's not?
 *
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <time.h> // NEEDED BY MD2PTYPES
#include "md2ptypes.h"

#include "genx.h"

#include "dump_version.h"
#include "dump_serialiser.h"
#include "dump_utils.h"

#include "dump_ser_xml.h"

#define COMMENT_PREFIX   " Generated with the command: "
#define COMMENT_VERSION  " Generated using pnfsDump v" PNFSDUMP_VERSION_STR " "

#define FLAG_SHOW_AL (1<<0)
#define FLAG_SHOW_RP (1<<1)

#define ACCESS_LATENCY_TXT(AL)   access_latency_txt[AL]
#define RETENTION_POLICY_TXT(RP) retention_policy_txt[RP]


static ser_return_t begin_serialisation( const char *cmdline, int argc, char *argv[]);
static ser_return_t end_serialisation();

static ser_return_t begin_dump( md_id_t root_id);
static ser_return_t end_dump( md_id_t root_id);


static ser_return_t begin_dir( md_id_t parent_dir_id, md_id_t id,
		      const char *name, md_attr *attr);
static ser_return_t end_dir( md_id_t parent_dir_id, md_id_t id, const char *name);

static ser_return_t begin_file( md_id_t parent_dir_id, md_id_t id, const char *name,
				filesize_t size, lvl2_info_t *lvl2, md_attr *attr);

static ser_return_t end_file( md_id_t parent_dir_id, md_id_t id,
			      const char *name);

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
				       md_id_t id,
				       const char *name);

static ser_return_t new_level( md_id_t file_id, int level,
			       md_attr *attr, const char *data,
			       size_t size);

static ser_return_t new_link( md_id_t parent, md_id_t link_id, const char *name,
			      md_attr *attr, const char *data, size_t size);


static int establish_xml_elements( void);
static int start_writer( FILE *out_stream, const char *cmdline);
static void emit_unix_attr( md_unix *attr);
static constUtf8 long_to_string( unsigned long data);
static constUtf8 longlong_to_string( long long size);
static constUtf8 short_to_octal_string( unsigned short data);
static constUtf8 time_to_string( time_t t);


static dump_serialiser_t info = {
  "the PNFS namespace in an XML format: [-a] [-r]",
  FLAGS_REQ_DEFAULT,
  begin_serialisation, end_serialisation,
  begin_dump, end_dump,
  begin_dir, end_dir,
  new_primary_tag, new_pseudo_tag, new_invalid_tag, new_inherited_tag,
  begin_file, end_file,
  new_level,
  new_link,
};

static const char *access_latency_txt[] = {
  [l2_al_online]   "online",
  [l2_al_nearline] "nearline",
};

static const char *retention_policy_txt[] = {
  [l2_rp_replica]   "replica",
  [l2_rp_output]    "output",
  [l2_rp_custodial] "custodial",
};


/**
 *  Globals needed for XML writing.
 */

genxWriter writer;
genxElement el_dir, el_file, el_tag, el_level, el_link;
genxAttribute at_id, at_pid, at_name, at_depth, at_uid, at_gid, at_mode;
genxAttribute at_ctime, at_mtime, at_atime, at_size, at_type, at_al, at_rp;
static int flags;


/**
 *  Our registry call-back
 */

dump_serialiser_t *xml_get_serialiser()
{
  return &info;
}


/**
 *  Call-backs for serialisation.
 */

ser_return_t begin_serialisation( const char *cmdline,
				  int argc, char *argv[])
{
  int c;

  optind = 1;  /* Reset getopt() state */

  while( (c = getopt( argc, argv, "+:ar")) != -1) {
    switch( c) {
    case 'a':
      flags ^= FLAG_SHOW_AL;
      break;

    case 'r':
      flags ^= FLAG_SHOW_RP;
      break;

    case ':':
      fprintf( stderr, "Option -%c requires an argument\n", optopt);
      return ser_error;

    case '?':
      fprintf( stderr, "Unknown output option: -%c\n", optopt);
      return ser_error;

    }
  }

  if( !start_writer( stdout, cmdline))
    goto err_exit;

  return ser_ok;

 err_exit:
  fprintf(stderr, "oops %s\n", genxLastErrorMessage(writer));
  return ser_error;
}


ser_return_t end_serialisation()
{
  genxEndDocument( writer);

  genxDispose( writer);

  return ser_ok;
}

ser_return_t begin_dump( md_id_t root_id)
{
  genxStartElementLiteral( writer, NULL, (constUtf8) "pnfs");
  genxAddAttributeLiteral( writer, NULL, (constUtf8) "root-id",
			   (constUtf8) mdStringID( root_id));

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

  genxStartElement( el_dir);
  genxAddAttribute( at_id, (constUtf8) mdStringID( id));
  genxAddAttribute( at_name, (constUtf8) name);

  return ser_ok;
}

ser_return_t end_dir( md_id_t parent_dir_id, md_id_t id, const char *name)
{
  genxEndElement( writer);

  return ser_ok;
}


ser_return_t begin_file( md_id_t parent_dir_id, md_id_t id, const char *name,
			 filesize_t size, lvl2_info_t *lvl2, md_attr *attr)
{
  access_latency_value_t al;
  retention_policy_value_t rp;

  genxStartElement( el_file);
  genxAddAttribute( at_id, (constUtf8) mdStringID( id));
  genxAddAttribute( at_name, (constUtf8) name);

  if( flags & FLAG_SHOW_AL &&
      deduce_file_al( parent_dir_id, id, lvl2, &al))
    genxAddAttribute( at_al, (constUtf8) ACCESS_LATENCY_TXT(al));

  if( flags & FLAG_SHOW_RP &&
      deduce_file_rp( parent_dir_id, id, lvl2, &rp))
    genxAddAttribute( at_rp, (constUtf8) RETENTION_POLICY_TXT(rp));

  if( attr->unixAttr.mst_size != md_no_size)
    genxAddAttribute( at_size,  longlong_to_string( size));

  emit_unix_attr( &attr->unixAttr);

  return ser_ok;
}


ser_return_t end_file( md_id_t parent_dir_id, md_id_t id, const char *name)
{

  genxEndElement( writer);

  return ser_ok;
}
  



ser_return_t new_primary_tag( md_id_t parent_dir_id, md_id_t id, const char *name,
			      md_attr *attr, const char *data, size_t data_length)
{
  genxStartElement( el_tag);
  genxAddAttribute( at_id, (constUtf8) mdStringID( id));
  genxAddAttribute( at_type, (constUtf8) "primary");
  genxAddAttribute( at_name, (constUtf8) name);
  emit_unix_attr( &attr->unixAttr);

  genxAddText(writer, (constUtf8) data);

  genxEndElement( writer);

  return ser_ok;
}



ser_return_t new_pseudo_tag( md_id_t parent_dir_id, md_id_t parent_tag_id,
			     md_id_t id, const char *name, md_attr *attr,
			     const char *data, size_t data_length)
{
  genxStartElement( el_tag);
  genxAddAttribute( at_id, (constUtf8) mdStringID( id));
  genxAddAttribute( at_pid, (constUtf8) mdStringID( parent_tag_id));
  genxAddAttribute( at_type, (constUtf8) "pseudo");
  genxAddAttribute( at_name, (constUtf8) name);
  emit_unix_attr( &attr->unixAttr);

  genxAddText(writer, (constUtf8) data);

  genxEndElement( writer);

  return ser_ok;
}



ser_return_t new_invalid_tag( md_id_t parent_dir_id, md_id_t parent_tag_id,
			      md_id_t id, const char *name)
{
  genxStartElement( el_tag);
  genxAddAttribute( at_id, (constUtf8) mdStringID( id));
  genxAddAttribute( at_pid, (constUtf8) mdStringID( parent_tag_id));
  genxAddAttribute( at_type, (constUtf8) "invalid");
  genxAddAttribute( at_name, (constUtf8) name);

  genxEndElement( writer);

  return ser_ok;
}


ser_return_t new_inherited_tag( md_id_t parent_dir_id, md_id_t parent_tag_id,
				md_id_t id, const char *name)
{
  genxStartElement( el_tag);
  genxAddAttribute( at_id, (constUtf8) mdStringID( id));
  genxAddAttribute( at_pid, (constUtf8) mdStringID( parent_tag_id));
  genxAddAttribute( at_type, (constUtf8) "inherited");
  genxAddAttribute( at_name, (constUtf8) name);

  genxEndElement( writer);

  return ser_ok;
}




/**
 *  Include a dump of Level information.
 */
ser_return_t new_level( md_id_t file_id, int level,
			md_attr *attr, const char *data,
			size_t size)
{
  char buffer[3];
  char *data_str;

  data_str = malloc( size+1);
  if( !data_str) {
    fprintf( stderr, "out of memory.\n");
    return ser_ok;
  }

  memcpy( data_str, data, size);
  data_str [size] = '\0';

  snprintf( buffer, sizeof( buffer), "%u", level);

  genxStartElement( el_level);
  genxAddAttribute( at_depth, (constUtf8) buffer);
  emit_unix_attr( &attr->unixAttr);

  genxAddText( writer, (constUtf8) data_str);

  genxEndElement( writer);

  free( data_str);

  return ser_ok;
}



ser_return_t new_link( md_id_t parent, md_id_t link_id, const char *name,
		       md_attr *attr, const char *data, size_t size)
{
  char *filename;

  if( !(filename = malloc( size+1))) {
    fprintf( stderr, "out of memory.\n");
    return ser_ok;
  }

  strncpy( filename, data, size);
  filename [size] = '\0';

  genxStartElement( el_link);
  genxAddAttribute( at_name, (constUtf8) name);
  emit_unix_attr( &attr->unixAttr);

  genxAddText( writer, (constUtf8) filename);

  genxEndElement( writer);

  free( filename);

  return ser_ok;
}



/**
 *    P R I V A T E   F U N C T I O N S
 */


/**
 *  Allocate GenX XML elements
 *
 *  Return 1 on success, 0 on failure.
 */
int establish_xml_elements()
{
  genxStatus status;

  if( !(el_dir = genxDeclareElement( writer, NULL, (constUtf8) "dir", &status)))
    return 0;

  if( !(el_file = genxDeclareElement( writer, NULL, (constUtf8) "file", &status)))
    return 0;
  
  if( !(el_tag = genxDeclareElement( writer, NULL, (constUtf8) "tag", &status)))
    return 0;

  if( !(el_level = genxDeclareElement( writer, NULL, (constUtf8) "level", &status)))
    return 0;

  if( !(el_link = genxDeclareElement( writer, NULL, (constUtf8) "link", &status)))
    return 0;


  if( !(at_id = genxDeclareAttribute( writer, NULL, (constUtf8) "id", &status)))
    return 0;

  if( !(at_pid = genxDeclareAttribute( writer, NULL, (constUtf8) "pid", &status)))
    return 0;

  if( !(at_type = genxDeclareAttribute( writer, NULL, (constUtf8) "type", &status)))
    return 0;

  if( !(at_name = genxDeclareAttribute( writer, NULL, (constUtf8) "name", &status)))
    return 0;

  if( !(at_depth = genxDeclareAttribute( writer, NULL, (constUtf8) "depth", &status)))
    return 0;

  if( !(at_uid = genxDeclareAttribute( writer, NULL, (constUtf8) "uid", &status)))
    return 0;

  if( !(at_gid = genxDeclareAttribute( writer, NULL, (constUtf8) "gid", &status)))
    return 0;

  if( !(at_mode = genxDeclareAttribute( writer, NULL, (constUtf8) "mode", &status)))
    return 0;

  if( !(at_ctime = genxDeclareAttribute( writer, NULL, (constUtf8) "ctime", &status)))
    return 0;

  if( !(at_mtime = genxDeclareAttribute( writer, NULL, (constUtf8) "mtime", &status)))
    return 0;

  if( !(at_atime = genxDeclareAttribute( writer, NULL, (constUtf8) "atime", &status)))
    return 0;

  if( !(at_size = genxDeclareAttribute( writer, NULL, (constUtf8) "size", &status)))
    return 0;

  if( !(at_al = genxDeclareAttribute( writer, NULL, (constUtf8) "access-latency", &status)))
    return 0;

  if( !(at_rp = genxDeclareAttribute( writer, NULL, (constUtf8) "retention-policy", &status)))
    return 0;

  return 1;
}


/**
 *  Start the GenX writer and emit initial comments.
 *
 *  Returns 1 on success, 0 on failure.
 */
int start_writer( FILE *out_stream, const char *cmdline)
{
  char *comment_str;

  writer = genxNew(NULL, NULL, NULL);

  if( genxStartDocFile( writer, out_stream))
    return 0;

  if( !establish_xml_elements())
    return 0;

  comment_str = malloc( strlen( cmdline) + sizeof( COMMENT_PREFIX) + 2); /* +2 for final " \0" */
  if( comment_str) {
    strcpy( comment_str, COMMENT_PREFIX);
    strcat( comment_str, cmdline);
    strcat( comment_str, " ");

    genxComment( writer, (constUtf8) comment_str);
    free( comment_str);
  }

  genxComment( writer, (constUtf8) COMMENT_VERSION);

  return 1;
}


void emit_unix_attr( md_unix *attr)
{  
  if( attr->mst_uid != md_no_uid)
    genxAddAttribute( at_uid,  long_to_string( attr->mst_uid));

  if( attr->mst_gid != md_no_gid)
    genxAddAttribute( at_gid,  long_to_string( attr->mst_gid));

  if( attr->mst_mode != md_no_mode)
    genxAddAttribute( at_mode, short_to_octal_string( attr->mst_mode));

  if( attr->mst_ctime != md_no_time)
    genxAddAttribute( at_ctime, time_to_string( attr->mst_ctime));

  if( attr->mst_mtime != md_no_time)
    genxAddAttribute( at_mtime, time_to_string( attr->mst_mtime));

  if( attr->mst_atime != md_no_time)
    genxAddAttribute( at_atime, time_to_string( attr->mst_atime));
}



constUtf8 long_to_string( unsigned long data)
{
  static char buffer[10];

  snprintf( buffer, sizeof( buffer), "%lu", data);

  return (constUtf8) buffer;
}


constUtf8 longlong_to_string( long long data)
{
  static char buffer[25];

  snprintf( buffer, sizeof( buffer), "%llu", data);

  return (constUtf8) buffer;
}



constUtf8 short_to_octal_string( unsigned short data)
{
  static char buffer[10];

  snprintf( buffer, sizeof( buffer), "0%.4o", data);

  return (constUtf8) buffer;
}


constUtf8 time_to_string( time_t t)
{
  static char buffer [25];
  struct tm tm;

  strftime( buffer, sizeof( buffer), "%Y-%m-%dT%H:%M:%SZ", gmtime_r( &t, &tm));

  return (constUtf8) buffer;
}
