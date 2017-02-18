#ifndef __PARSE_LVL2_H__
#define __PARSE_LVL2_H__

#include "dump_types.h"


#define LVL2_DEFAULT_LENGTH 1


/**
 *  These are the names as used in the uc field of level-2 metadata.
 *  They are defined in org.dcache.util.ChecksumType.
 */
#define CHKSUM_ADLER32_UC_NAME  "ADLER32"
#define CHKSUM_MD5_UC_NAME      "MD5"
#define CHKSUM_MD4_UC_NAME      "MD4"


/**
 *  These are the default values used when no data is available.
 *
 *  These exist mainly for the parser tests in chk_lvl2_parse.c
 *
 *  Clients of these routines are encouraged to use the flags to
 *  detect when data is missing and not rely on these values.
 */
#define LVL2_DEFAULT_AL  -1
#define LVL2_DEFAULT_RP  -1
#define LVL2_DEFAULT_HSM -1


/**
 *  Bit-wise values used for the flags element of lvl2_info_t.
 */
#define LVL2_FLAG_HAVE_NOTHING 0x00

#define LVL2_FLAG_HAVE_LENGTH  (1<<0)
#define LVL2_FLAG_HAVE_CHKSUM  (1<<1)
#define LVL2_FLAG_HAVE_AL      (1<<2)
#define LVL2_FLAG_HAVE_RP      (1<<3)
#define LVL2_FLAG_HAVE_HSM     (1<<4)

#define LVL2_FLAG_CANT_PARSE   (1<<5)

typedef enum {
  l2_al_nearline,
  l2_al_online,
} access_latency_value_t;


typedef enum {
  l2_rp_custodial,
  l2_rp_output,
  l2_rp_replica,  
} retention_policy_value_t;

typedef enum {
  l2_hsm_pool_without_hsm,
  l2_hsm_pool_with_hsm,
} hsm_pool_value_t;


/**
 *  Information gleaned from the cryptic level-2 file metadata
 */
typedef struct {

  /* Raw data */
  const char *data;
  const size_t data_size;

  int flags;

  /* Length of the file */
  filesize_t length;

  /* A stack of checksum values */
  checksum_value_t *checksums;
  int checksum_count;

  /* An array checksums */
  checksum_value_t *checksum_stack;
  int checksum_stack_count;

  /* The Access-Latency value */
  access_latency_value_t access_latency;
  
  /* The Retention-Policy value */
  retention_policy_value_t retention_policy;

  /* Whether a file was first written on a pool that is "attached to HSM" */
  hsm_pool_value_t hsm_pool;
} lvl2_info_t;

void lvl2_reset( lvl2_info_t *info);
int lvl2_parse_data( lvl2_info_t *info, const char *data, size_t data_size, const char *context);
void lvl2_flush( lvl2_info_t *info);
void lvl2_final_flush( lvl2_info_t *info);


/**
 *  These variables map the Access-Latency and Retention-Policy enum
 *  values back to human-understandable strings.  The case for these
 *  values is not guaranteed, so they should only be used when
 *  printing messages (i.e., for human consumption) and NOT when
 *  storing the values (i.e., for machine consumption).
 */
extern const char *l2_al_text[];
extern const char *l2_rp_text[];


#endif
