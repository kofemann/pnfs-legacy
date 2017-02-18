#ifndef __DUMP_INTREE_H__
#define __DUMP_INTREE_H__

#include <time.h>

#include "md2ptypes.h"
#include "shmcom.h"


void insubtree_register_root( md_id_t root_id);
void insubtree_register_sub_root( md_id_t root_id);

md_id_t insubtree_get_sub_root();

int insubtree_tag_within_subtree( md_id_t tag);

#endif
