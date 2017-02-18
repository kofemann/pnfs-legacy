
/**
 *  SQL serialiser: produces SQL statement to populate Chimera based
 *  installation.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>

#include "dump_cache.h"
#include "dump_serialiser.h"
#include "dump_utils.h"
#include "dump_version.h"

#include "dump_ser_chimera.h"


/* These are what certain integers mean in Chimera */

#define CHIMERA_CHECKSUM_TYPE_ADLER  1
#define CHIMERA_CHECKSUM_TYPE_MD5    2
#define CHIMERA_CHECKSUM_TYPE_MD4    3

#define CHIMERA_AL_NEARLINE   0
#define CHIMERA_AL_ONLINE     1

#define CHIMERA_RP_CUSTODIAL  0
#define CHIMERA_RP_OUTPUT     1
#define CHIMERA_RP_REPLICA    2


#define CHIMERA_ABSOLUTE_ROOT_ID  "000000000000000000000000000000000000"


/* The number of characters in PNFS and Chimera IDs */
#define ID_PNFS_LENGTH    24
#define ID_CHIMERA_LENGTH 36




static ser_return_t begin_serialisation( const char *cmdline, int argc, char *argv[]);
static ser_return_t end_serialisation();

static ser_return_t begin_dump( md_id_t root_id);

static ser_return_t begin_dir( md_id_t parent_dir_id, md_id_t id,
		      const char *name, md_attr *attr);
static ser_return_t begin_file( md_id_t parent_dir_id, md_id_t id, const char *name,
				filesize_t filesize, lvl2_info_t *lvl2, md_attr *attr);
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
static ser_return_t new_level( md_id_t file_id, int level,
			       md_attr *attr, const char *data,
			       size_t size);
static ser_return_t new_link( md_id_t parent, md_id_t link_id, const char *name,
			      md_attr *attr, const char *data, size_t size);

static int map_checksum_type( checksum_type_t type);
const char *to_escaped_string( const char *data, size_t len);
static size_t calc_escaped_string_size( const char *data, size_t len);
static int write_escaped_string( char *buffer, const char *data, size_t len);
static const char *safe_string( const char *name, md_id_t id);
static int process_options( int argc, char *argv[]);
static void establish_chimera_dir( char **chimera_dir_id_p, md_id_t pnfs_dir_id);

static int get_store(md_id_t dir_id, char *store, size_t store_size);
static int get_sgroup(md_id_t dir_id, char *sgroup, size_t sgroup_size);
static int get_tag( md_id_t dir_id, const char *tag,
		    char *tag_data_buf, size_t tag_data_buf_size,
		    size_t *tag_size_p, int be_silent);
static void emit_al_rp_schema_1( md_id_t parent_id, md_id_t file_id,
				 lvl2_info_t *lvl2, const char *name);
static void emit_al_rp_schema_2( md_id_t parent_id, md_id_t file_id,
				 lvl2_info_t *lvl2, const char *name);



static dump_serialiser_t info = {
  "SQL suitable for Chimera and PostGreSQL: [-1 | -2] [-f] [-p <Chimera root ID>]",
  FLAGS_REQ_LEVEL2,
  begin_serialisation, end_serialisation,
  begin_dump, NULL,  /* ignore end dump */
  begin_dir, NULL,   /* ignore end directory */
  new_primary_tag, new_pseudo_tag, new_invalid_tag, NULL, /* ignore inherited tags */
  begin_file, end_file,
  new_level,
  new_link,
};


/**
 *  Map level-2 checksum types into corresponding Chimera values.
 */
static int checksum_type_map[] = {
  [chksum_adler32] = CHIMERA_CHECKSUM_TYPE_ADLER,
  [chksum_md5]     = CHIMERA_CHECKSUM_TYPE_MD5,
  [chksum_md4]     = CHIMERA_CHECKSUM_TYPE_MD4,
};


/**
 *  Map level-2 access-latency and retention-policy types into
 *  corresponding Chimera values.
 */

static int chimera_access_latency[] = {
  [l2_al_online] = CHIMERA_AL_ONLINE,
  [l2_al_nearline] = CHIMERA_AL_NEARLINE
};

static int chimera_retention_policy[] = {
  [l2_rp_replica] = CHIMERA_RP_REPLICA,
  [l2_rp_custodial] = CHIMERA_RP_CUSTODIAL,
  [l2_rp_output] = CHIMERA_RP_OUTPUT,
};


/*
 * output stream
 */
static FILE *out = NULL;

static char *chimera_root_id;
static md_id_t pnfs_root_id;

static void (*emit_al_rp)( md_id_t parent_id, md_id_t file_id, lvl2_info_t *lvl2, const char *name);


/**
 *  Our registry call-back
 */

dump_serialiser_t *chimera_get_serialiser()
{
  return &info;
}


/**
 *  Call-backs for serialisation.
 */

ser_return_t begin_serialisation( const char *cmdline,
				  int argc, char *argv[])
{
  time_t now;

  if( process_options( argc, argv) == -1)
    return ser_error;


  /*
   * In the future we can switch to external file defined in argv[]
   */
  out = stdout;
  
  time( &now);

  fprintf(out,  "---\n");
  fprintf(out,  "--- BEGIN of Dump\n");
  fprintf(out,  "---\n");
  fprintf(out,  "--- Output generated by pnfsDump v" PNFSDUMP_VERSION_STR  "\n");
  fprintf(out,  "---\n");
  fprintf(out,  "--- using command-line\n");
  fprintf(out,  "---\n");
  fprintf(out,  "---     %s\n", cmdline);
  fprintf(out,  "---\n");
  fprintf(out,  "--- taken on %s", ctime( &now));
  fprintf(out,  "---\n");
    
  return ser_ok;
}


/**
 *  Process the options a user might supply.
 *
 *  Returns 0 on success -1 if the process should terminate.
 */
int process_options( int argc, char *argv[])
{
  int c, allow_pnfs=0;

  optind = 1;  /* Reset getopt() state */

  while( (c = getopt( argc, argv, "+:12fp:a:r:v:")) != -1) {
    switch( c) {

    case 'f':
      /* Allow -p options with a PNFS ID */
      allow_pnfs = 1;
      break;

    case 'p':
      if( dump_parse_hex( optarg, ID_CHIMERA_LENGTH) == -1) {

	if( !allow_pnfs || dump_parse_hex( optarg, ID_PNFS_LENGTH) == -1) {
	  fprintf( stderr, "Invalid ID %s\n", optarg);
	  return -1;
	}

	fprintf( stderr, "Warning: you have given a PNFS-ID as the root Chimera ID.  Continuing as instructed!\n");
      }
      
      free( chimera_root_id);
      chimera_root_id = strdup( optarg);

      break;

    case '1':
      if( emit_al_rp)
	fprintf( stderr, "Overriding previous schema version selection by switching schema to version 1\n");
      emit_al_rp = emit_al_rp_schema_1;
      break;

    case '2':
      if( emit_al_rp)
	fprintf( stderr, "Overriding previous schema version selection by switching schema to version 2\n");
      emit_al_rp = emit_al_rp_schema_2;
      break;

    case ':':
      fprintf( stderr, "Option -%c requires an argument\n", optopt);
      return -1;

    case '?':
      fprintf( stderr, "Unknown output option: -%c\n", optopt);
      return -1;
    }
  }

  return 0;
}


ser_return_t end_serialisation()
{
  fprintf(out,  "\n");
  fprintf(out,  "---\n");
  fprintf(out,  "--- END of Dump\n");
  fprintf(out,  "---\n");

  fflush(out);

  to_escaped_string( NULL, 0); /* Free any allocated memory */

  free( chimera_root_id);

  return ser_ok;
}

/*
 * create root element
 */
ser_return_t begin_dump( md_id_t root_id)
{
  /* Remember the PNFS root dir ID */
  memcpy( &pnfs_root_id, &root_id, sizeof( md_id_t));

  fprintf(out, "\n\n\n");

  /*
   * create inode
   * new inde created with link count == 2. the value will be increased with each new file
   * to keep file system view concistet
   */

  /**
   *  NB.  we do this only if the user hasn't specified a root Chimera
   *  ID.  We quite expect this to fail, though.
   */
  if( !chimera_root_id) {

    fprintf(out, "BEGIN;\n");

    fprintf(out, "    INSERT INTO t_inodes VALUES('" CHIMERA_ABSOLUTE_ROOT_ID "', 16384, %d, %d, %d, %d, %ld, %d, NOW(), NOW(), NOW());\n",
	    (int)0755, 
	    2,
	    0,
	    0,
	    512L,
	    0
	    );

    /*
     * Create initial '.' and '..'
     */

    fprintf(out, "    INSERT INTO t_dirs VALUES('"CHIMERA_ABSOLUTE_ROOT_ID"', '.', '"CHIMERA_ABSOLUTE_ROOT_ID"');\n");
    fprintf(out, "    INSERT INTO t_dirs VALUES('"CHIMERA_ABSOLUTE_ROOT_ID"', '..', '"CHIMERA_ABSOLUTE_ROOT_ID"');\n");
    
    fprintf(out,  "COMMIT;\n");

  }

  return ser_ok;

}


/**
 *  Call-back for beginning and end of a directory.
 */
ser_return_t begin_dir( md_id_t pnfs_parent_dir_id, md_id_t id, const char *name,
	       md_attr *attr)
{
  char *chimera_parent_dir_id;
  char buf[25];
  char buf1[25];
  fprintf(out,  "\n\n\n");
  fprintf(out,  "BEGIN;\n");

  const char *safe_name;

  safe_name = safe_string( name, id);

  establish_chimera_dir( &chimera_parent_dir_id, pnfs_parent_dir_id);
      

  /*
   * create inode
   * new inde created with link count == 2. the value will be increased with each new file
   * to keep file system view concistet
   */
  fprintf(out, "    INSERT INTO t_inodes VALUES('%s', 16384, %d, %d, %d, %d, %ld, %d, to_timestamp(%ld), to_timestamp(%ld), to_timestamp(%ld));\n",
       mdStringID_r(id, buf),
       (int)attr->unixAttr.mst_mode & 0777, 
       2,
       (int)attr->unixAttr.mst_uid,
       (int)attr->unixAttr.mst_gid,
       (long)attr->unixAttr.mst_size,
       0,
       (long)attr->unixAttr.mst_ctime,
       (long)attr->unixAttr.mst_atime,
       (long)attr->unixAttr.mst_mtime);

  /*
   * create entry in the directory
   */
  fprintf(out, "    INSERT INTO t_dirs VALUES('%s', '%s', '%s');\n", chimera_parent_dir_id, safe_name, mdStringID_r(id, buf) );

  /*
   * correct parent directory link count
   */

  fprintf(out, "    UPDATE t_inodes SET inlink = inlink +1 WHERE ipnfsid='%s';\n",  chimera_parent_dir_id );
  
  /*
   * Create initial '.' and '..'
   */

  fprintf(out, "    INSERT INTO t_dirs VALUES('%s', '.', '%s');\n", mdStringID_r(id, buf), mdStringID_r(id, buf1) );
  fprintf(out, "    INSERT INTO t_dirs VALUES('%s', '..', '%s');\n", mdStringID_r(id, buf), chimera_parent_dir_id );

  fprintf(out,  "COMMIT;\n");

  if( safe_name != name)
    free( (void*)safe_name);

  return ser_ok;
}



ser_return_t begin_file( md_id_t pnfs_parent_dir_id, md_id_t id, const char *name,
			 filesize_t filesize, lvl2_info_t *lvl2,
			 md_attr *attr)
{
  char *chimera_parent_dir_id;
  char buf[25];
  const char *safe_name;
  int chksum_type;
  checksum_value_t *chk;

  fprintf(out,  "\n\n\n");
  fprintf(out,  "BEGIN;\n");

  establish_chimera_dir( &chimera_parent_dir_id, pnfs_parent_dir_id);
      

  safe_name = safe_string( name, id);

  /*
   * create inode
   */
  fprintf(out, "    INSERT INTO t_inodes VALUES('%s', 32768, %d, %d, %d, %d, %llu, %d, to_timestamp(%ld), to_timestamp(%ld), to_timestamp(%ld));\n",
       mdStringID_r(id, buf),
       (int)attr->unixAttr.mst_mode & 0777,
       (int)attr->unixAttr.mst_nlink,
       (int)attr->unixAttr.mst_uid,
       (int)attr->unixAttr.mst_gid,
       (unsigned long long)filesize,
       0, /* no IO allowed */
       (long)attr->unixAttr.mst_ctime,
       (long)attr->unixAttr.mst_atime,
       (long)attr->unixAttr.mst_mtime);

  /*
   * create entry in the directory
   */
  fprintf(out, "    INSERT INTO t_dirs VALUES('%s', '%s', '%s');\n", chimera_parent_dir_id, name, mdStringID_r(id,buf) );

  /*
   * correct parent directory link count
   */

  fprintf(out, "    UPDATE t_inodes SET inlink = inlink +1 WHERE ipnfsid='%s';\n", chimera_parent_dir_id);
  fprintf(out,  "\n");


  /**
   *  Add file checksums, if there are any.
   */
  if( lvl2 && lvl2->flags & LVL2_FLAG_HAVE_CHKSUM) {    
    for( chk = lvl2->checksums; chk && chk->value; chk++) {
      chksum_type = map_checksum_type( chk->type);

      if( chksum_type != -1) {
	fprintf( out, "   INSERT INTO t_inodes_checksum VALUES('%s',%u,'%s');\n",
		 mdStringID_r(id,buf),
		 chksum_type,
		 chk->value);
      } else {
	fprintf( stderr, "Skipping checksum with unknown type:\n");
	fprintf( stderr, "  PNFSid: %s\n", mdStringID_r(id,buf));
	fprintf( stderr, "  Type: %d\n", chk->type);
	fprintf( stderr, "  Value: %s\n", chk->value);
      }
    }
  }


  /**
   *  If level-2 data has either the Access-Latency or the Retension
   *  Policy specified, we need to store this somehow.
   */
  if( lvl2 && lvl2->flags & (LVL2_FLAG_HAVE_AL|LVL2_FLAG_HAVE_RP)) {

    if( !emit_al_rp) {
      fprintf( stderr, "A file with explicit access-latency or retention-policy information detected.\n");
      fprintf( stderr, "No Chimera schema version has been specified, so using defaults.\n");
      fprintf( stderr, "Please specify either -1 or -2 to specify which schema version is in use.\n");
      fprintf( stderr, "  (future occurrences will not be reported).\n");

      emit_al_rp = emit_al_rp_schema_1;
    }

    emit_al_rp( pnfs_parent_dir_id, id, lvl2, name);
  }

  if( safe_name != name)
    free( (void*)safe_name);

  return ser_ok;
}



/**
 *  Emit a file's Access Latency and Retention policy suitable for
 *  Chimera schema pre dCache-v 1.9.2
 */
void emit_al_rp_schema_1( md_id_t parent_id, md_id_t file_id, lvl2_info_t *lvl2,
			  const char *name)
{
  access_latency_value_t file_al;
  retention_policy_value_t file_rp;
  char hsm_type_storage [100], *hsm_type;
  char *escaped_storeName;
  char sGroup[255], storeName[255];
  char pnfs_id_buf[25];
  static int have_warned;

  if( !deduce_file_al( parent_id, file_id, lvl2, &file_al))
    goto err_exit;

  if( !deduce_file_rp( parent_id, file_id, lvl2, &file_rp))
    goto err_exit;

  if( get_tag( parent_id, "hsmType", hsm_type_storage, sizeof( hsm_type_storage),
	       NULL, 1)) {
    hsm_type_storage [sizeof( hsm_type_storage)-1] = '\0';
    hsm_type = hsm_type_storage;

    if( strcmp( hsm_type, "osm") && !have_warned) {
      fprintf( stderr, "You have tag hsmType == \"%s\" (!= \"osm\").  This may cause problems.\n",
	       hsm_type);
      fprintf( stderr, "  (future occurrences will not be reported).\n");
      fprintf( stderr, "\tfile: %s\n", name);
      fprintf( stderr, "\tfile id: %s\n", md2PrintID(file_id));
      fprintf( stderr, "\tdir id: %s\n", md2PrintID(parent_id));
      have_warned = 1;
    }
  } else {
    hsm_type = "osm";
  }


  /**
   *  We must construct a t_storageinfo entry for this file.
   */

  if( !get_store( parent_id, storeName, sizeof( storeName)))
    goto err_exit;

  if( !get_sgroup( parent_id, sGroup, sizeof( sGroup)))
    goto err_exit;
    
  escaped_storeName = strdup( to_escaped_string( storeName, strlen( storeName)));

  fprintf(out, "    INSERT INTO t_storageinfo VALUES('%s', '%s', E'%s', E'%s', %d, %d);\n",
	  mdStringID_r( file_id, pnfs_id_buf),
	  hsm_type, 
	  escaped_storeName,
	  to_escaped_string( sGroup, strlen( sGroup)),
	  chimera_access_latency [file_al],
	  chimera_retention_policy [file_rp]);
  
  free( escaped_storeName);

  return;

 err_exit:
  fprintf( stderr, "Unable to establish file storage info; access-latency and retention policy information will be lost.\n");
  fprintf( stderr, "\tfile: %s\n", name);
  fprintf( stderr, "\tfile id: %s\n", md2PrintID( file_id));
  fprintf( stderr, "\tdir id: %s\n", md2PrintID( parent_id));
}


/**
 *  Emit SQL to store a file's access-latency and retention policy
 *  suitable for the newer version of the schema.
 */
void emit_al_rp_schema_2( md_id_t parent_id, md_id_t file_id, lvl2_info_t *lvl2,
			  const char *name)
{
  if( lvl2  && lvl2->flags & LVL2_FLAG_HAVE_AL)
    fprintf(out, "    INSERT INTO t_access_latency VALUES('%s', %d);\n",
	    md2PrintID( file_id),
	    chimera_access_latency [lvl2->access_latency]);

  if( lvl2 && lvl2->flags & LVL2_FLAG_HAVE_RP)
    fprintf(out, "    INSERT INTO t_retention_policy VALUES('%s', %d);\n",
	    md2PrintID( file_id),
	    chimera_retention_policy [lvl2->retention_policy]);

  return;
}

 
ser_return_t  end_file( md_id_t parent_dir_id, md_id_t id, const char *name)
{
   fprintf(out,  "COMMIT;\n");
   return ser_ok;
}

ser_return_t new_primary_tag( md_id_t pnfs_parent_dir_id, md_id_t id, const char *name,
			      md_attr *attr, const char *data, size_t data_length)
{
  char *chimera_parent_dir_id;
  const char *safe_name;    
  char buf[25];
  
  establish_chimera_dir( &chimera_parent_dir_id, pnfs_parent_dir_id);

  safe_name = safe_string( name, id);

  fprintf(out,  "\n\n\n");
  fprintf(out,  "BEGIN;\n");

    
  fprintf(out, "    INSERT INTO t_tags_inodes VALUES('%s',%d,1,%d,%d,%d,to_timestamp(%ld),to_timestamp(%ld),to_timestamp(%ld), E'%s');\n",
            mdStringID_r(id, buf),
            (int)attr->unixAttr.mst_mode & 0100000, /* tag is always a file */
            (int)attr->unixAttr.mst_uid,
            (int)attr->unixAttr.mst_gid,
            data_length,
            (long)attr->unixAttr.mst_ctime,
            (long)attr->unixAttr.mst_atime,
            (long)attr->unixAttr.mst_mtime,
  	    to_escaped_string(data, data_length) 
          );
  
  
  fprintf(out, "    SELECT update_tag('%s','%s','%s',1);\n",
          chimera_parent_dir_id,
          name,
          mdStringID_r(id, buf));

  fprintf(out,  "COMMIT;\n");
    
  if( safe_name != name)
    free( (void*)safe_name);

  return ser_ok;
}


ser_return_t new_pseudo_tag( md_id_t pnfs_parent_dir_id, md_id_t parent_tag_id,
			     md_id_t id, const char *name, md_attr *attr,
			     const char *data, size_t data_length)
{
  /*
   * promote preudo-primary tags into primary tags
   */
  fprintf(out,  "\n\n\n");
  fprintf(out,  "---\n");
  fprintf(out,  "--- Promoting a pseudo-primary tag\n");
  fprintf(out,  "---\n");
  return  new_primary_tag( pnfs_parent_dir_id, id, name, attr, data, data_length) ;
}


ser_return_t new_invalid_tag( md_id_t pnfs_parent_dir_id, md_id_t parent_tag_id,
			      md_id_t id, const char *name)
{
  /*
    char *chimera_parent_dir_id;
    establish_chimera_dir( &chimera_parent_dir_id, pnfs_parent_dir_id);
  */


 /* NOP (yet) */
  return ser_ok;
}


/**
 *  
 */
ser_return_t new_level( md_id_t file_id, int level,
			md_attr *attr, const char *data,
			size_t size)
{
  char buf[25];

  if( level == 0 ) {
      fprintf(out, "    INSERT INTO t_inodes_data  VALUES('%s', E'%s');\n", mdStringID_r(file_id, buf), to_escaped_string( data, size));
  }else{
      fprintf(out, "    INSERT INTO t_level_%d VALUES('%s', %d, %d, %d, %d, %ld, to_timestamp(%ld), to_timestamp(%ld), to_timestamp(%ld), E'%s');\n",
       level,
         mdStringID_r(file_id, buf),
         (int)attr->unixAttr.mst_mode & 0777,
         (int)attr->unixAttr.mst_nlink,
         (int)attr->unixAttr.mst_uid,
         (int)attr->unixAttr.mst_gid,
         (long)attr->unixAttr.mst_size,
         (long)attr->unixAttr.mst_ctime,
         (long)attr->unixAttr.mst_atime,
         (long)attr->unixAttr.mst_mtime,
         to_escaped_string( data, size));
  }

  return ser_ok;
}



ser_return_t new_link( md_id_t pnfs_parent_dir_id, md_id_t link_id, const char *name,
		       md_attr *attr, const char *data, size_t size)
{
  char *chimera_parent_dir_id;
  char buf[25];
  const char *safe_name, *link_target;

  fprintf(out,  "\n\n\n");
  fprintf(out,  "BEGIN;\n");

  establish_chimera_dir( &chimera_parent_dir_id, pnfs_parent_dir_id);
      
  /*
   * create inode
   */
  fprintf(out, "    INSERT INTO t_inodes VALUES('%s', 40960, %d, %d, %d, %d, %llu, %d, to_timestamp(%ld), to_timestamp(%ld), to_timestamp(%ld));\n",
	  mdStringID(link_id),
	  (int)attr->unixAttr.mst_mode & 0777,
	  (int)attr->unixAttr.mst_nlink,
	  (int)attr->unixAttr.mst_uid,
	  (int)attr->unixAttr.mst_gid,
	  (unsigned long long)size,
	  1, /* IO allowed */
	  (long)attr->unixAttr.mst_ctime,
	  (long)attr->unixAttr.mst_atime,
	  (long)attr->unixAttr.mst_mtime);

  /*
   * store link pinter as the inode data
   */
  link_target = to_escaped_string( data, size);
  fprintf(out, "    INSERT INTO t_inodes_data VALUES('%s', E'%s');\n",
	  mdStringID(link_id),
	  link_target );


  /*
   * create entry in the directory
   */
  safe_name = safe_string( name, link_id);
  fprintf(out, "    INSERT INTO t_dirs VALUES('%s', '%s', '%s');\n",
	  chimera_parent_dir_id,
	  safe_name,
	  mdStringID_r(link_id,buf) );

  /*
   * correct parent directory link count
   */

  fprintf(out, "    UPDATE t_inodes SET inlink = inlink +1 WHERE ipnfsid='%s';\n", chimera_parent_dir_id);

  fprintf(out,  "COMMIT;\n");

  if( safe_name != name)
    free( (void*)safe_name);

  return ser_ok;
}



/**
 *  Ensure that the input string has no strange (non-printable)
 *  characters in it.
 *
 *  If the given string contains at least one character that isprint()
 *  returns false then a new string is created with those characters
 *  replaced by a '+' and a warning emitted.
 */
const char *safe_string( const char *name, md_id_t id)
{
  int idx;

  char *new_name;

  /* Scan through the name to see if there are any evil characters */
  for( idx = 0; name [idx]; idx++)
    if( name [idx] == '/' || !isprint(name [idx]))
      break;

  if( !name [idx])
    return name;

  /* Map to a new name */

  if( !(new_name = strdup( name))) {
    fprintf( stderr, "Out of memory in safe_string()\n");
    return name;
  }
    
  for( idx = 0; new_name [idx]; idx++)
    if( new_name [idx] == '/' || !isprint(new_name [idx]))
      new_name [idx] = '+';

  fprintf( stderr, "Dangerous name for %s mapped to \"%s\"\n", 
	   md2PrintID( id), new_name);
  
  return new_name;
}


/**
 *  Convert a char sequence to an escaped sequence suitable for a
 *  bytea field.
 *
 *  See 8.4 Binary Data Types
 *
 *    http://www.postgresql.org/docs/8.2/static/datatype-binary.html
 *
 *  Call this fn with data == NULL to free dynamically allocated local
 *  buffer.
 *
 *  data is the raw data to escape, len is the length of this data in
 *  bytes.
 */
const char *to_escaped_string( const char *data, size_t len)
{
  static char *buffer;
  static size_t buffer_size;
  size_t esc_len, desired_buffer_size;

  if( data == NULL) {
    free( buffer);
    buffer_size = 0;
    return NULL;
  }
    
  esc_len = calc_escaped_string_size( data, len);
  desired_buffer_size = esc_len+1;  /* +1 for final '\0' byte */

  if( buffer_size < desired_buffer_size) {  
    buffer_size = desired_buffer_size;
    buffer = realloc( buffer, buffer_size);
  }

  write_escaped_string( buffer, data, len);

  buffer [esc_len] = '\0';

  return buffer;
}



/**
 *  Calculate the length of the string after encoding the string.  We
 *  use ' => '' mapping, but all other characters are left alone.
 */
size_t calc_escaped_string_size( const char *data, size_t len)
{
  size_t escaped_len=len;
  int i;

  for( i = 0; i < len; i++) {
    if( data [i] <= 31 || data [i] >= 127 || data [i] == '\\' || data [i] == '\'')
      escaped_len += 4;
  }

  return escaped_len;
}



/**
 *  Write the first len bytes of data into buffer, escaping any '
 *  characters as double-apostophes ('').
 */
int write_escaped_string( char *buffer, const char *data, size_t len)
{
  int in_idx=0, out_idx=0;
  int rc=0;
  char this_byte;

  for( in_idx = 0; in_idx < len; in_idx++) {
    
    this_byte = data [in_idx];

    if( this_byte <= 31 || this_byte >= 127 || this_byte == '\\' || this_byte == '\'') {
      buffer [out_idx++] = '\\';
      buffer [out_idx++] = '\\';
      sprintf( buffer+out_idx, "%03o", (unsigned char) this_byte);
      out_idx += 3;
    } else 
      buffer [out_idx++] = this_byte;
  }

  return rc;
}



int map_checksum_type( checksum_type_t type)
{
  if( type < FIRST_CHECKSUM_TYPE_VALUE ||
      type > LAST_CHECKSUM_TYPE_VALUE)
    return -1;

  return checksum_type_map [type];
}


/**
 *  Convert a PNFS directory ID to a Chimera directory.
 *
 *  For most directory IDs, this is the "identity" mapping, the
 *  chimera ID is identical to the PNFS ID.
 *
 *  As a special case: if we are referring to the PNFS root directory,
 *  should we use either the user-supplied Chimera-root-ID value or
 *  the Chimera ID of the absolute root.
 */
void establish_chimera_dir( char **chimera_dir_id_p, md_id_t pnfs_dir_id)
{
  static char pnfs_str[25];

  if( mdIsEqualID( pnfs_dir_id, pnfs_root_id)) {
    *chimera_dir_id_p = chimera_root_id ? chimera_root_id : CHIMERA_ABSOLUTE_ROOT_ID;
  } else {
    mdStringID_r( pnfs_dir_id, pnfs_str);
    *chimera_dir_id_p = pnfs_str;
  }
}



/**
 *  Get the value of a tag.  The cache is used wherever possible.
 *
 *  dir_id is the PNFS ID of the directory in which to look for a tag
 *  tag is the name of the tag
 *  tag_data_buf is the buffer into which the data will be placed
 *  tag_data_buf_size is the maximum size of this buffer
 *  tag_size_p is a pointer to a size_t variable that will be updated
 *             with the tag's data size or NULL if this information isn't
 *             needed.
 *
 *  Returns 1 on success, 0 on error.
 */
int get_tag( md_id_t dir_id, const char *tag,
	     char *tag_data_buf, size_t tag_data_buf_size,
	     size_t *tag_size_p, int be_silent)
{
    md_id_t tag_id;


    if( !cache_get_tag_id( dir_id, tag,  &tag_id) ) {

      if( !be_silent) {
	fprintf( stderr, "Can't find tag '%s':\n", tag);
	fprintf( stderr, "\tdir id: %s\n", md2PrintID( dir_id));
      }
      return 0;
    }

    if( !cache_find_tag_value( tag_id,
			       tag_data_buf, tag_data_buf_size,
			       tag_size_p) ) {

      if( !be_silent) {
	fprintf( stderr, "Can't get tag '%s' data:\n", tag);
	fprintf( stderr, "\tdir id: %s\n", md2PrintID( dir_id));
	fprintf( stderr, "\ttag id: %s\n", md2PrintID( tag_id));
      }
      return 0;
    }

    return 1;

}


/**
 * get sGroup tag to populate sorage info
 *
 * @returns:
 *     1 if sgroup found and zero other wise
 */
int get_sgroup(md_id_t dir_id, char *sgroup, size_t sgroup_size)
{
  char *eol;

  if( get_tag(dir_id, "sGroup", sgroup, sgroup_size, NULL, 0) ) {
    
    /**
     * we expect single line with storage group.
     * just in case of eat the rest.
     */

    eol = strchr(sgroup, '\n');
    if( eol != NULL ) {
      eol[0] = '\0';
    }

    return 1;
  }
  
  return 0;
}


/**
 * get store name to populate storage info
 *
 * @returns:
 *      1 if store name was found and zero otherwise
 */
int get_store(md_id_t dir_id, char *store, size_t store_size)
{

  char *eol, *space, *value;

  if( get_tag(dir_id, "OSMTemplate", store, store_size, NULL, 0) ) {

    /**
     * OSMTemplate has more or less free form content.
     * But we expect the first line to be have sore name:
     *   StoreName <name>
     */

    /* pick the first line only */
    eol = strchr(store, '\n');
    if( eol != NULL ) {
      eol[0] = '\0';
    }

    /* if there is something more than 'StoreName ' */
    if( strlen((const char *)store) > 10 ) {
      space = strchr( store, ' ');

      if( space == NULL ) {

	/* crap! */
	fprintf( stderr, "Missing space in OSMTemplate tag first line\n");
	fprintf( stderr, "\tdir: %s\n", md2PrintID( dir_id));
	fprintf( stderr, "\tvalue: \"%s\"\n", store);

      }else{

	/* Locate first non white-space character after the initial space. */
	for( value = space; isspace( *value); value++);

	memmove( store, value, strlen( value)+1);  /* +1 to include terminating '\0' */
	
	return 1;
      }
    }
    
  }

  return 0;
}
