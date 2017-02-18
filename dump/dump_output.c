/**
 *  Provide simple access to a "database" of serialisers.
 */
#include <stdio.h>
#include <string.h>

#include "dump_ser_null.h"
#include "dump_ser_simple.h"
#include "dump_ser_xml.h"
#include "dump_ser_syncat.h"
#include "dump_ser_chimera.h"
#include "dump_ser_files.h"
#include "dump_ser_verify.h"


typedef struct {
  const char *name;
  dump_serialiser_t *(*get_serialiser)();
} pnfs_serialisers_t;


/**
 *  Information about our different serialisers.  Each available
 *  serialiser should register itself here.  We use a call-back to
 *  discover the dump_serialiser_t structure so each instance does not
 *  need to expose their functions for a static declaration of the
 *  information.
 */

#define BUILD_ENTRY(NAME) { #NAME, NAME ## _get_serialiser}

static pnfs_serialisers_t pnfs_serialisers[] = {

#ifdef ENABLE_SERIALISER_null
  BUILD_ENTRY(null),
#endif

#ifdef ENABLE_SERIALISER_simple
  BUILD_ENTRY(simple),
#endif

#ifdef ENABLE_SERIALISER_xml
  BUILD_ENTRY(xml),
#endif

#ifdef ENABLE_SERIALISER_syncat
  BUILD_ENTRY(syncat),
#endif

#ifdef ENABLE_SERIALISER_chimera
  BUILD_ENTRY(chimera),
#endif

#ifdef ENABLE_SERIALISER_files
  BUILD_ENTRY(files),
#endif

#ifdef ENABLE_SERIALISER_verify
  BUILD_ENTRY(verify),
#endif

};



/**
 *  Find a named serialiser and return its dump_serialiser_t
 *  structure.  Returns NULL if the serialiser couldn't be found.
 */
dump_serialiser_t *pnfs_serialisers_find( const char *name)
{
  int i;
  dump_serialiser_t *ser = NULL;

  for( i = 0; i < sizeof( pnfs_serialisers) / sizeof( pnfs_serialisers_t); i++)
    if( !strcmp( name, pnfs_serialisers [i].name)) {
      ser = pnfs_serialisers [i].get_serialiser();
      break;
    }

  return ser;
}


/**
 *  List the available serialisers on the given FILE ptr.  The
 *  separator string interleaves the entries.
 */
void pnfs_serialisers_list( FILE *fp, const char *separator, int show_help)
{
  dump_serialiser_t *ser;
  int i;

  for( i = 0; i < sizeof( pnfs_serialisers) / sizeof( pnfs_serialisers_t); i++) {
    if( i != 0)
      fprintf( fp, "%s", separator);

    fprintf( fp, "%s", pnfs_serialisers [i].name);

    if( show_help) {
      ser = pnfs_serialisers [i].get_serialiser();

      fprintf( fp, ": ");
      fprintf( fp, "%s", ser->help);
    }
  }
}
