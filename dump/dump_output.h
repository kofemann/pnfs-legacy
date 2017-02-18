
#ifndef __DUMP_OUTPUT_H__
#define __DUMP_OUTPUT_H__

#include "dump_serialiser.h"


dump_serialiser_t *pnfs_serialisers_find( const char *name);
void pnfs_serialisers_list( FILE *fp, const char *separator, int show_help);

#endif
