/**
 *  Primative types shared between different dump source-code.
 */
#ifndef __PARSE_TYPES_H__
#define __PARSE_TYPES_H__

typedef unsigned long long filesize_t;


/**
 *  Sometimes we need to use the explicit number rather than the enum
 *  defined below.  This should only be used when building explicit
 *  references to the number and this should only happen inside the
 *  parser tests.
 *
 *  If in doubt, use the checksum_type_t enum below. 
 */
#define CHKSUM_ADLER32_C_NUMBER 1
#define CHKSUM_MD5_C_NUMBER     2
#define CHKSUM_MD4_C_NUMBER     3

#define FIRST_CHECKSUM_TYPE_VALUE  chksum_adler32
#define LAST_CHECKSUM_TYPE_VALUE   chksum_md4

/**
 *  These values are taken from:
 *
 *    dCache/modules/dCache/diskCacheV111/util/Checksum.java
 *
 *  You MUST manually update the LAST_CHECKSUM_TYPE_VALUE above.
 */
typedef enum {
  chksum_adler32=CHKSUM_ADLER32_C_NUMBER,
  chksum_md5=CHKSUM_MD5_C_NUMBER,
  chksum_md4=CHKSUM_MD4_C_NUMBER
} checksum_type_t;


typedef struct {
  checksum_type_t type;
  const char *value;
} checksum_value_t;




#endif
