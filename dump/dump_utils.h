/**
 *  A set of utility functions that are shared between dump_main
 *  (et. al) and the serialisers.
 */

#ifndef __DUMP_UTILS__
#define __DUMP_UTILS__

#include <time.h>  // Needed for md2ptypes.h

#include "md2ptypes.h" // Needed for md_id_t

#include "parse_lvl2.h"

int dump_parse_hex( const char *txt, size_t length);
char * mdStringID_r( md_id_t id, char *str);

void utils_hex_bytes( char *str, unsigned char *bytes, size_t bytes_len, int uc);
void utils_hex_long( char *str, int num, int uc);
void utils_hex_short( char *str, int num, int uc);
void utils_hex_byte( char *str, int num, int uc);

int deduce_file_al( md_id_t dir_id, md_id_t file_id, lvl2_info_t *lvl2,
		    access_latency_value_t *al_p);
int deduce_file_rp( md_id_t dir_id, md_id_t file_id, lvl2_info_t *lvl2,
		    retention_policy_value_t *rp_p);


#endif
