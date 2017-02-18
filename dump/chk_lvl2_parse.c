/**
 *   Routines to verify that parse-lvl2.c works as expected.
 *
 *   There are three groups of tests: those requiring no data, those
 *   testing behaviour with broken input and those testing behaviour
 *   with correct input.
 *
 *   The values returned by routines with the behaviour with no data
 *   and behaviour with correct data are also checked against static
 *   "correct" values.
 *
 *   The code returns 0 on success, non-zero (1) if there was a
 *   problem.  This is checked automatically by the build process.
 *   The program should also be verified by running within valgrind,
 *   but this isn't checked automatically.
 */
#include <stdio.h>
#include <string.h>

#include "parse_lvl2.h"
#include "dump_serialiser.h"


#define FIVE_TERABYTES 5497558138880

#define CHKSUM_1_TYPE_NUMBER CHKSUM_ADLER32_C_NUMBER
#define CHKSUM_1_TYPE_NAME   CHKSUM_ADLER32_UC_NAME
#define CHKSUM_1_VALUE       "deadbeef"

#define CHKSUM_2_TYPE_NUMBER CHKSUM_MD5_C_NUMBER
#define CHKSUM_2_TYPE_NAME   CHKSUM_MD5_UC_NAME
#define CHKSUM_2_VALUE       "9e107d9d372bb6826bd81d3542a419d6"

#define CHKSUM_3_TYPE_NUMBER CHKSUM_MD4_C_NUMBER
#define CHKSUM_3_TYPE_NAME   CHKSUM_MD4_UC_NAME
#define CHKSUM_3_VALUE       "6df23dc03f9b54cc38a0fc1483df6e21"

// A different Adler-32 checksum, taken from FZK
#define CHKSUM_4_TYPE_NUMBER CHKSUM_ADLER32_C_NUMBER
#define CHKSUM_4_TYPE_NAME   CHKSUM_ADLER32_UC_NAME
#define CHKSUM_4_VALUE       "328ace33"


#define DUMMY_VALUE 0
#define DUMMY_LEN   0

/**
 *   These are all derived values, based on above definitions
 */

#define STR(X) #X
#define XSTR(X) STR(X)

#define LONGLONGCONST(A) A ## ULL
#define XLONGLONGCONST(A) LONGLONGCONST(A)

#define FIVE_TERABYTES_VAL XLONGLONGCONST( FIVE_TERABYTES)
#define FIVE_TERABYTES_STR XSTR(FIVE_TERABYTES)

// Values for c and c1 flag
#define CHKSUM_1_STR  XSTR(CHKSUM_1_TYPE_NUMBER) ":" CHKSUM_1_VALUE
#define CHKSUM_2_STR  XSTR(CHKSUM_2_TYPE_NUMBER) ":" CHKSUM_2_VALUE
#define CHKSUM_3_STR  XSTR(CHKSUM_3_TYPE_NUMBER) ":" CHKSUM_3_VALUE
#define CHKSUM_4_STR  XSTR(CHKSUM_4_TYPE_NUMBER) ":" CHKSUM_4_VALUE

// Values for c1 flag
#define CHKSUM_1_2_STR   CHKSUM_1_STR ";" CHKSUM_2_STR
#define CHKSUM_2_1_STR   CHKSUM_2_STR ";" CHKSUM_1_STR
#define CHKSUM_1_3_STR   CHKSUM_1_STR ";" CHKSUM_3_STR
#define CHKSUM_3_1_STR   CHKSUM_3_STR ";" CHKSUM_1_STR
#define CHKSUM_2_3_STR   CHKSUM_2_STR ";" CHKSUM_3_STR
#define CHKSUM_3_2_STR   CHKSUM_3_STR ";" CHKSUM_2_STR
#define CHKSUM_1_2_3_STR CHKSUM_1_STR ";" CHKSUM_2_STR ";" CHKSUM_3_STR
#define CHKSUM_1_3_2_STR CHKSUM_1_STR ";" CHKSUM_3_STR ";" CHKSUM_2_STR
#define CHKSUM_2_1_3_STR CHKSUM_2_STR ";" CHKSUM_1_STR ";" CHKSUM_3_STR
#define CHKSUM_2_3_1_STR CHKSUM_2_STR ";" CHKSUM_3_STR ";" CHKSUM_1_STR
#define CHKSUM_3_2_1_STR CHKSUM_3_STR ";" CHKSUM_2_STR ";" CHKSUM_1_STR

// Values for uc flag
#define CHKSUM_UC_1_STR  CHKSUM_1_TYPE_NAME ":" CHKSUM_1_VALUE
#define CHKSUM_UC_2_STR  CHKSUM_2_TYPE_NAME ":" CHKSUM_2_VALUE
#define CHKSUM_UC_3_STR  CHKSUM_3_TYPE_NAME ":" CHKSUM_3_VALUE
#define CHKSUM_UC_4_STR  CHKSUM_4_TYPE_NAME ":" CHKSUM_4_VALUE

// Values for uc flag when multiple checksums are specified.
#define CHKSUM_UC_1_2_STR   CHKSUM_UC_1_STR "," CHKSUM_UC_2_STR
#define CHKSUM_UC_1_3_STR   CHKSUM_UC_1_STR "," CHKSUM_UC_3_STR
#define CHKSUM_UC_2_3_STR   CHKSUM_UC_2_STR "," CHKSUM_UC_3_STR
#define CHKSUM_UC_2_1_STR   CHKSUM_UC_2_STR "," CHKSUM_UC_1_STR
#define CHKSUM_UC_3_2_STR   CHKSUM_UC_3_STR "," CHKSUM_UC_2_STR
#define CHKSUM_UC_1_2_3_STR CHKSUM_UC_1_STR "," CHKSUM_UC_2_STR "," CHKSUM_UC_3_STR
#define CHKSUM_UC_1_3_2_STR CHKSUM_UC_1_STR "," CHKSUM_UC_3_STR "," CHKSUM_UC_2_STR
#define CHKSUM_UC_2_1_3_STR CHKSUM_UC_2_STR "," CHKSUM_UC_1_STR "," CHKSUM_UC_3_STR
#define CHKSUM_UC_2_3_1_STR CHKSUM_UC_2_STR "," CHKSUM_UC_3_STR "," CHKSUM_UC_1_STR
#define CHKSUM_UC_3_2_1_STR CHKSUM_UC_3_STR "," CHKSUM_UC_2_STR "," CHKSUM_UC_1_STR


#define EMPTY_INFO  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_NOTHING, LVL2_DEFAULT_LENGTH, NULL, 0, NULL, DUMMY_VALUE, LVL2_DEFAULT_AL, LVL2_DEFAULT_RP, LVL2_DEFAULT_HSM}

const char *expected_fail[] = {
  "",                             // Empty data
  "    ",                         // Empty data
  "\n2,0,0,0.0,0.0\n:\n",         // New-line at beginning of data.
  "3,0,0,0,0,0\n:a=1;b=2;\n",     // Wrong version
  "2,0,0,0,0,0",                  // Missing keyword-value pairs
  "2,0,0,0,0,0\n",                // Missing keyword-value pairs
  "2,0,0,0,0,0\na=1;b=2;",        // Keyword-value line doesn't end with NL
  "2,0,0,0,0,0\na=1;b=2;\n",      // Keyword-value line doesn't start with colon
  "2,0,0,0,0,0\na=1;b=2:22;\n",   // Keyword-value line doesn't start with colon

  "2,0,0,0,0,0\n:a=1;l=foo;\n",    // Length is not a number
  "2,0,0,0,0,0\n:a=1;l=69foo;\n",  // Length is not a number
  "2,0,0,0,0,0\n:a=1;l=foo69;\n",  // Length is not a number
  "2,0,0,0,0,0\n:a=1;l=-5;\n",     // Length is negative number

  "2,0,0,0,0,0\n:a=1;c=4d7a8;\n",    // no colon in chksum algorithm number
  "2,0,0,0,0,0\n:a=1;c=:4d7a8;\n",   // empty checksum algorithm number
  "2,0,0,0,0,0\n:a=1;c=a:4d7a8;\n",  // non-number checksum algorithm number 
  "2,0,0,0,0,0\n:a=1;c=1a:4d7a8;\n", // non-number checksum algorithm number 
  "2,0,0,0,0,0\n:a=1;c=1:blah;\n",   // contains illegal checksum value 'l' & 'h'

  // Check repeated checksum values with different values.
  "2,0,0,0.0,0.0\n:c=" CHKSUM_1_STR ";c1=" CHKSUM_4_STR";\n",
  "2,0,0,0.0,0.0\n:c=" CHKSUM_4_STR ";c1=" CHKSUM_4_STR ";" CHKSUM_1_STR";\n",
  "2,0,0,0.0,0.0\n:c=" CHKSUM_4_STR ";c1=" CHKSUM_1_STR ";" CHKSUM_4_STR ";\n",
  "2,0,0,0.0,0.0\n:c=" CHKSUM_4_STR ";c1=" CHKSUM_1_2_STR ";\n",
  "2,0,0,0.0,0.0\n:c=" CHKSUM_4_STR ";c1=" CHKSUM_2_1_STR ";\n",
  "2,0,0,0.0,0.0\n:c=" CHKSUM_4_STR ";c1=" CHKSUM_1_2_3_STR ";\n",
  "2,0,0,0.0,0.0\n:c=" CHKSUM_4_STR ";c1=" CHKSUM_1_3_2_STR ";\n",
  "2,0,0,0.0,0.0\n:c=" CHKSUM_4_STR ";c1=" CHKSUM_2_3_1_STR ";\n",
  "2,0,0,0.0,0.0\n:c=" CHKSUM_4_STR ";c1=" CHKSUM_2_1_3_STR ";\n",
  "2,0,0,0.0,0.0\n:c=" CHKSUM_4_STR ";c1=" CHKSUM_3_2_1_STR ";\n",

  "2,0,0,0,0,0\n:c1=1:0000;2:blah;a=1;\n", // contains illegal checksum value 'l' 'h'
  "2,0,0,0,0,0\n:c1=1:0000;0000;a=1;\n",   // missing colon
  "2,0,0,0,0,0\n:c1=1:0000;2:;a=1;\n",     // missing data
  "2,0,0,0,0,0\n:uc=:\n",                  // missing UC data
  "2,0,0,0,0,0\n:uc=foo:\n",               // non-standard algorithm "foo"
  "2,0,0,0,0,0\n:uc=" CHKSUM_1_TYPE_NAME ":\n", // missing data
  "2,0,0,0,0,0\n:uc=" CHKSUM_2_TYPE_NAME ":\n", // missing data
  "2,0,0,0,0,0\n:uc=" CHKSUM_3_TYPE_NAME ":\n", // missing data

  // UC specifying inconsistant checksums.
  "2,0,0,0.0,0.0\n:c=" CHKSUM_4_STR ";c1=" CHKSUM_UC_1_STR ";\n",
  "2,0,0,0.0,0.0\n:c=" CHKSUM_1_STR ";c1=" CHKSUM_UC_4_STR ";\n",
  "2,0,0,0.0,0.0\n:c=" CHKSUM_4_STR ";c1=" CHKSUM_UC_1_2_STR ";\n",
  "2,0,0,0.0,0.0\n:c=" CHKSUM_4_STR ";c1=" CHKSUM_UC_2_1_STR ";\n",
  "2,0,0,0.0,0.0\n:c=" CHKSUM_4_STR ";c1=" CHKSUM_UC_1_2_3_STR ";\n",
  "2,0,0,0.0,0.0\n:c=" CHKSUM_4_STR ";c1=" CHKSUM_UC_3_2_1_STR ";\n",
  "2,0,0,0.0,0.0\n:c=" CHKSUM_4_STR ";c1=" CHKSUM_UC_2_3_1_STR ";\n",
  "2,0,0,0.0,0.0\n:c=" CHKSUM_4_STR ";c1=" CHKSUM_UC_2_1_3_STR ";\n",

  // Check with bogus values for AL, RP and HSM
  "2,0,0,0.0,0.0\n:al=foo;c=" CHKSUM_4_STR ";c1=" CHKSUM_UC_2_1_3_STR ";\n",
  "2,0,0,0.0,0.0\n:c=" CHKSUM_4_STR ";al=foo;c1=" CHKSUM_UC_2_1_3_STR ";\n",
  "2,0,0,0.0,0.0\n:c=" CHKSUM_4_STR ";c1=" CHKSUM_UC_2_1_3_STR ";al=foo;\n",
  "2,0,0,0.0,0.0\n:rp=foo;c=" CHKSUM_4_STR ";c1=" CHKSUM_UC_2_1_3_STR ";\n",
  "2,0,0,0.0,0.0\n:c=" CHKSUM_4_STR ";rp=foo;c1=" CHKSUM_UC_2_1_3_STR ";\n",
  "2,0,0,0.0,0.0\n:c=" CHKSUM_4_STR ";c1=" CHKSUM_UC_2_1_3_STR ";rp=foo;\n",
  "2,0,0,0.0,0.0\n:h=foo;c=" CHKSUM_4_STR ";c1=" CHKSUM_UC_2_1_3_STR ";\n",
  "2,0,0,0.0,0.0\n:c=" CHKSUM_4_STR ";h=foo;c1=" CHKSUM_UC_2_1_3_STR ";\n",
  "2,0,0,0.0,0.0\n:c=" CHKSUM_4_STR ";c1=" CHKSUM_UC_2_1_3_STR ";h=foo;\n",

  // Try repeated and inconsistant declaration of AL, RP and HSM
  "2,0,0,0.0,0.0\n:al=ONLINE;al=NEARLINE;\n",
  "2,0,0,0.0,0.0\n:rp=CUSTODIAL;rp=REPLICA;\n",
  "2,0,0,0.0,0.0\n:h=yes;h=no;\n",
};


const char *expected_success[] = {
  /* Test 0.. */
  "1,0,0,0.0,0.0\ngeneric-oho-pool-0",                       // Version 1 data

  "2,0,0,0.0,0.0\n:\n",                                      // Empty of content


  "2,0,0,0.0,0.0\n:l=" FIVE_TERABYTES_STR ";\n",             // Contains a single entry

  "2,0,0,0.0,0.0\n:c=" CHKSUM_1_STR ";\n",                   // Contains a single line
  "2,0,0,0.0,0.0\n:c=" CHKSUM_2_STR ";\n",                   // Contains a single line

  "2,0,0,0.0,0.0\n:l=" FIVE_TERABYTES_STR ";c=" CHKSUM_1_STR ";\n",    // Contains a single line
  "2,0,0,0.0,0.0\n:l=" FIVE_TERABYTES_STR ";c=" CHKSUM_2_STR ";\n",    // Contains a single line

  "2,0,0,0.0,0.0\n:l=" FIVE_TERABYTES_STR ";\n:c=" CHKSUM_1_STR ";\n", // Contains multiple lines
  "2,0,0,0.0,0.0\n:l=" FIVE_TERABYTES_STR ";\n:c=" CHKSUM_2_STR ";\n", // Contains multiple lines

  "2,0,0,0.0,0.0\n:c=" CHKSUM_1_STR ";\n:l=" FIVE_TERABYTES_STR ";\n", // Contains multiple lines
  "2,0,0,0.0,0.0\n:c=" CHKSUM_2_STR ";\n:l=" FIVE_TERABYTES_STR ";\n", // Contains multiple lines

  // Test 11..  The C1 values

  "2,0,0,0.0,0.0\n:c1=" CHKSUM_1_STR ";\n", // Contains a c1 entry
  "2,0,0,0.0,0.0\n:c1=" CHKSUM_2_STR ";\n", // Contains a c1 entry

  "2,0,0,0.0,0.0\n:c1=" CHKSUM_1_2_STR ";""\n", // Contains a c1 entry
  "2,0,0,0.0,0.0\n:c1=" CHKSUM_2_1_STR ";""\n", // Contains a c1 entry
  "2,0,0,0.0,0.0\n:c1=" CHKSUM_1_3_STR ";""\n", // Contains a c1 entry
  "2,0,0,0.0,0.0\n:c1=" CHKSUM_3_1_STR ";""\n", // Contains a c1 entry
  "2,0,0,0.0,0.0\n:c1=" CHKSUM_2_3_STR ";""\n", // Contains a c1 entry
  "2,0,0,0.0,0.0\n:c1=" CHKSUM_3_2_STR ";""\n", // Contains a c1 entry
  "2,0,0,0.0,0.0\n:c1=" CHKSUM_1_2_3_STR ";""\n", // Contains a c1 entry

  // Test 20.. Check repeated checksum values.
  "2,0,0,0.0,0.0\n:c=" CHKSUM_1_STR ";c1=" CHKSUM_1_STR ";\n", // Repeated checksum value
  "2,0,0,0.0,0.0\n:c=" CHKSUM_1_STR ";c1=" CHKSUM_1_2_STR ";\n", // Repeated checksum value
  "2,0,0,0.0,0.0\n:c=" CHKSUM_2_STR ";c1=" CHKSUM_1_2_STR ";\n", // Repeated checksum value
  "2,0,0,0.0,0.0\n:c=" CHKSUM_2_STR ";c1=" CHKSUM_1_2_3_STR ";\n", // Repeated checksum value
  "2,0,0,0.0,0.0\n:c=" CHKSUM_2_STR ";c1=" CHKSUM_2_1_3_STR ";\n", // Repeated checksum value
  "2,0,0,0.0,0.0\n:c=" CHKSUM_2_STR ";c1=" CHKSUM_3_2_1_STR ";\n", // Repeated checksum value
  "2,0,0,0.0,0.0\n:c=" CHKSUM_2_STR ";c1=" CHKSUM_1_3_2_STR ";\n", // Repeated checksum value

  "2,0,0,0.0,0.0\n:l=" FIVE_TERABYTES_STR ";c1=" CHKSUM_1_STR ";\n", // Contains a c1 entry
  "2,0,0,0.0,0.0\n:l=" FIVE_TERABYTES_STR ";c1=" CHKSUM_2_STR ";\n", // Contains a c1 entry

  "2,0,0,0.0,0.0\n:l=" FIVE_TERABYTES_STR ";c1=" CHKSUM_1_2_STR ";""\n", // Contains a c1 entry
  "2,0,0,0.0,0.0\n:l=" FIVE_TERABYTES_STR ";c1=" CHKSUM_2_1_STR ";""\n", // Contains a c1 entry
  "2,0,0,0.0,0.0\n:l=" FIVE_TERABYTES_STR ";c1=" CHKSUM_1_3_STR ";""\n", // Contains a c1 entry
  "2,0,0,0.0,0.0\n:l=" FIVE_TERABYTES_STR ";c1=" CHKSUM_1_2_3_STR ";""\n", // Contains a c1 entry

  // Test 33.. Contains multiple lines
  "2,0,0,0.0,0.0\n:c=" CHKSUM_2_STR ";\n:l=" FIVE_TERABYTES_STR ";\n",

  // Check with just uc flags specified
  "2,0,0,0.0,0.0\n:uc=\n",             // Not sure if this is valid, but we can understand it
  "2,0,0,0,0,0\n:uc=" CHKSUM_UC_1_STR "\n",
  "2,0,0,0,0,0\n:uc=" CHKSUM_UC_2_STR "\n",
  "2,0,0,0,0,0\n:uc=" CHKSUM_UC_3_STR "\n",
  "2,0,0,0,0,0\n:uc=" CHKSUM_UC_1_2_STR "\n",
  "2,0,0,0,0,0\n:uc=" CHKSUM_UC_2_3_STR "\n",
  "2,0,0,0,0,0\n:uc=" CHKSUM_UC_1_3_STR "\n",
  "2,0,0,0,0,0\n:uc=" CHKSUM_UC_1_2_3_STR "\n",


  // Test 42.. Check with c and uc flags specified 
  "2,0,0,0,0,0\n:c=" CHKSUM_1_STR ";uc=" CHKSUM_UC_2_STR "\n", // Contains a c and uc entry
  "2,0,0,0,0,0\n:c=" CHKSUM_1_STR ";uc=" CHKSUM_UC_3_STR "\n", // Contains a c and uc entry
  "2,0,0,0,0,0\n:c=" CHKSUM_1_STR ";uc=" CHKSUM_UC_2_3_STR "\n", // Contains a c and two uc entries.
  "2,0,0,0,0,0\n:c=" CHKSUM_1_STR ";uc=" CHKSUM_UC_3_2_STR "\n", // Contains a c and two uc entries.

  /* Specific examples to catch bugs **/
  "2,0,0,0.0,0.0\n:h=yes;c=" CHKSUM_4_STR ";l=84195046;\n",   // From FZK

  /* Test 47..  Check access-latency and retention-policy by themselves are OK */
  "2,0,0,0.0,0.0\n:al=ONLINE;\n",
  "2,0,0,0.0,0.0\n:al=NEARLINE;\n",
  "2,0,0,0.0,0.0\n:rp=REPLICA;\n",
  "2,0,0,0.0,0.0\n:rp=CUSTODIAL;\n",
  /* Duplicates are allowed, if they are the same value */
  "2,0,0,0.0,0.0\n:al=ONLINE;al=ONLINE;\n",
  "2,0,0,0.0,0.0\n:al=NEARLINE;al=NEARLINE;\n",
  "2,0,0,0.0,0.0\n:rp=REPLICA;rp=REPLICA;\n",
  "2,0,0,0.0,0.0\n:rp=CUSTODIAL;rp=CUSTODIAL;\n",
  /* Check either access-latency or retention-policy with other flags */
  "2,0,0,0.0,0.0\n:al=ONLINE;c=" CHKSUM_1_STR ";l=" FIVE_TERABYTES_STR ";\n",
  "2,0,0,0.0,0.0\n:al=NEARLINE;c=" CHKSUM_1_STR ";l=" FIVE_TERABYTES_STR ";\n",
  "2,0,0,0.0,0.0\n:rp=REPLICA;c=" CHKSUM_1_STR ";l=" FIVE_TERABYTES_STR ";\n",
  "2,0,0,0.0,0.0\n:rp=CUSTODIAL;c=" CHKSUM_1_STR ";l=" FIVE_TERABYTES_STR ";\n",
  "2,0,0,0.0,0.0\n:c=" CHKSUM_1_STR ";al=ONLINE;l=" FIVE_TERABYTES_STR ";\n",
  "2,0,0,0.0,0.0\n:c=" CHKSUM_1_STR ";al=NEARLINE;l=" FIVE_TERABYTES_STR ";\n",
  "2,0,0,0.0,0.0\n:c=" CHKSUM_1_STR ";rp=REPLICA;l=" FIVE_TERABYTES_STR ";\n",
  "2,0,0,0.0,0.0\n:c=" CHKSUM_1_STR ";rp=CUSTODIAL;l=" FIVE_TERABYTES_STR ";\n",
  "2,0,0,0.0,0.0\n:c=" CHKSUM_1_STR ";l=" FIVE_TERABYTES_STR ";al=ONLINE;\n",
  "2,0,0,0.0,0.0\n:c=" CHKSUM_1_STR ";l=" FIVE_TERABYTES_STR ";al=NEARLINE;\n",
  "2,0,0,0.0,0.0\n:c=" CHKSUM_1_STR ";l=" FIVE_TERABYTES_STR ";rp=REPLICA;\n",
  "2,0,0,0.0,0.0\n:c=" CHKSUM_1_STR ";l=" FIVE_TERABYTES_STR ";rp=CUSTODIAL;\n",

  /**
   *  Test 67.. Check combinations of AL and RP: four combinations (two providing
   *  redundant information) are repeated five times
   */
  "2,0,0,0.0,0.0\n:al=ONLINE;rp=REPLICA;c="     CHKSUM_1_STR ";l=" FIVE_TERABYTES_STR ";\n",
  "2,0,0,0.0,0.0\n:rp=REPLICA;al=ONLINE;c="     CHKSUM_1_STR ";l=" FIVE_TERABYTES_STR ";\n",
  "2,0,0,0.0,0.0\n:al=NEARLINE;rp=CUSTODIAL;c=" CHKSUM_1_STR ";l=" FIVE_TERABYTES_STR ";\n",
  "2,0,0,0.0,0.0\n:rp=CUSTODIAL;al=NEARLINE;c=" CHKSUM_1_STR ";l=" FIVE_TERABYTES_STR ";\n",
  "2,0,0,0.0,0.0\n:al=ONLINE;c="    CHKSUM_1_STR ";rp=REPLICA;l="   FIVE_TERABYTES_STR ";\n",
  "2,0,0,0.0,0.0\n:rp=REPLICA;c="   CHKSUM_1_STR ";al=ONLINE;l="    FIVE_TERABYTES_STR ";\n",
  "2,0,0,0.0,0.0\n:al=NEARLINE;c="  CHKSUM_1_STR ";rp=CUSTODIAL;l=" FIVE_TERABYTES_STR ";\n",
  "2,0,0,0.0,0.0\n:rp=CUSTODIAL;c=" CHKSUM_1_STR ";al=NEARLINE;l="  FIVE_TERABYTES_STR ";\n",
  "2,0,0,0.0,0.0\n:al=ONLINE;c="    CHKSUM_1_STR ";l=" FIVE_TERABYTES_STR ";rp=REPLICA;\n",
  "2,0,0,0.0,0.0\n:rp=REPLICA;c="   CHKSUM_1_STR ";l=" FIVE_TERABYTES_STR ";al=ONLINE;\n",
  "2,0,0,0.0,0.0\n:al=NEARLINE;c="  CHKSUM_1_STR ";l=" FIVE_TERABYTES_STR ";rp=CUSTODIAL;\n",
  "2,0,0,0.0,0.0\n:rp=CUSTODIAL;c=" CHKSUM_1_STR ";l=" FIVE_TERABYTES_STR ";al=NEARLINE;\n",
  "2,0,0,0.0,0.0\n:c=" CHKSUM_1_STR ";al=ONLINE;l="    FIVE_TERABYTES_STR ";rp=REPLICA;\n",
  "2,0,0,0.0,0.0\n:c=" CHKSUM_1_STR ";rp=REPLICA;l="   FIVE_TERABYTES_STR ";al=ONLINE;\n",
  "2,0,0,0.0,0.0\n:c=" CHKSUM_1_STR ";al=NEARLINE;l="  FIVE_TERABYTES_STR ";rp=CUSTODIAL;\n",
  "2,0,0,0.0,0.0\n:c=" CHKSUM_1_STR ";rp=CUSTODIAL;l=" FIVE_TERABYTES_STR ";al=NEARLINE;\n",
  "2,0,0,0.0,0.0\n:c=" CHKSUM_1_STR ";l=" FIVE_TERABYTES_STR ";al=ONLINE;rp=REPLICA;\n",
  "2,0,0,0.0,0.0\n:c=" CHKSUM_1_STR ";l=" FIVE_TERABYTES_STR ";rp=REPLICA;al=ONLINE;\n",
  "2,0,0,0.0,0.0\n:c=" CHKSUM_1_STR ";l=" FIVE_TERABYTES_STR ";al=NEARLINE;rp=CUSTODIAL;\n",
  "2,0,0,0.0,0.0\n:c=" CHKSUM_1_STR ";l=" FIVE_TERABYTES_STR ";rp=CUSTODIAL;al=NEARLINE;\n",

  /* Test 87.. Check HSM flag is processed correctly */
  "2,0,0,0.0,0.0\n:h=yes;\n",
  "2,0,0,0.0,0.0\n:h=no;\n",
  /* Repeated values are OK, if the values are the same */
  "2,0,0,0.0,0.0\n:h=yes;h=yes;\n",
  "2,0,0,0.0,0.0\n:h=no;h=no;\n",
  /* Also check with other flags */
  "2,0,0,0.0,0.0\n:h=yes;c=" CHKSUM_1_STR ";l=" FIVE_TERABYTES_STR ";\n",
  "2,0,0,0.0,0.0\n:h=no;c="  CHKSUM_1_STR ";l=" FIVE_TERABYTES_STR ";\n",
  "2,0,0,0.0,0.0\n:c=" CHKSUM_1_STR ";h=yes;l=" FIVE_TERABYTES_STR ";\n",
  "2,0,0,0.0,0.0\n:c=" CHKSUM_1_STR ";h=no;l="  FIVE_TERABYTES_STR ";\n",
  "2,0,0,0.0,0.0\n:c=" CHKSUM_1_STR ";l=" FIVE_TERABYTES_STR ";h=yes;\n",
  "2,0,0,0.0,0.0\n:c=" CHKSUM_1_STR ";l=" FIVE_TERABYTES_STR ";h=no;\n",

  /* A Chistmas tree flag */
  "2,0,0,0.0,0.0\n:c=" CHKSUM_1_STR ";l=" FIVE_TERABYTES_STR ";rp=CUSTODIAL;al=NEARLINE;h=yes;s=*\n",

  /* A real-world example from NDGF */
  "2,0,0,0.0,0.0\n:al=ONLINE;rp=REPLICA;c=1:949950b2;l=979660604;h=no;\n"
};

static checksum_value_t checksum_1_value = {CHKSUM_1_TYPE_NUMBER,CHKSUM_1_VALUE};
static checksum_value_t checksum_2_value = {CHKSUM_2_TYPE_NUMBER,CHKSUM_2_VALUE};
static checksum_value_t checksum_3_value = {CHKSUM_3_TYPE_NUMBER,CHKSUM_3_VALUE};
static checksum_value_t checksum_4_value = {CHKSUM_4_TYPE_NUMBER,CHKSUM_4_VALUE};

static checksum_value_t checksum_12_value[] = {
  {CHKSUM_1_TYPE_NUMBER,CHKSUM_1_VALUE},
  {CHKSUM_2_TYPE_NUMBER,CHKSUM_2_VALUE},
};

static checksum_value_t checksum_13_value[] = {
  {CHKSUM_1_TYPE_NUMBER,CHKSUM_1_VALUE},
  {CHKSUM_3_TYPE_NUMBER,CHKSUM_3_VALUE},
};

static checksum_value_t checksum_23_value[] = {
  {CHKSUM_2_TYPE_NUMBER,CHKSUM_2_VALUE},
  {CHKSUM_3_TYPE_NUMBER,CHKSUM_3_VALUE},
};

static checksum_value_t checksum_123_value[] = {
  {CHKSUM_1_TYPE_NUMBER,CHKSUM_1_VALUE},
  {CHKSUM_2_TYPE_NUMBER,CHKSUM_2_VALUE},
  {CHKSUM_3_TYPE_NUMBER,CHKSUM_3_VALUE},
};

static checksum_value_t checksum_ndgf_value = {CHKSUM_ADLER32_C_NUMBER, "949950b2"};


lvl2_info_t expected_result[] = {
  /* Test 0.. */
  EMPTY_INFO,                                // Version 1

  EMPTY_INFO,

  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_LENGTH, FIVE_TERABYTES_VAL, NULL, 0, NULL, DUMMY_VALUE, LVL2_DEFAULT_AL, LVL2_DEFAULT_RP, LVL2_DEFAULT_HSM},

  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_CHKSUM, LVL2_DEFAULT_LENGTH, &checksum_1_value, 1, NULL, DUMMY_VALUE, LVL2_DEFAULT_AL, LVL2_DEFAULT_RP, LVL2_DEFAULT_HSM},
  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_CHKSUM, LVL2_DEFAULT_LENGTH, &checksum_2_value, 1, NULL, DUMMY_VALUE, LVL2_DEFAULT_AL, LVL2_DEFAULT_RP, LVL2_DEFAULT_HSM},

  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_LENGTH|LVL2_FLAG_HAVE_CHKSUM, FIVE_TERABYTES_VAL, &checksum_1_value, 1, NULL, DUMMY_VALUE, LVL2_DEFAULT_AL, LVL2_DEFAULT_RP, LVL2_DEFAULT_HSM},
  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_LENGTH|LVL2_FLAG_HAVE_CHKSUM, FIVE_TERABYTES_VAL, &checksum_2_value, 1, NULL, DUMMY_VALUE, LVL2_DEFAULT_AL, LVL2_DEFAULT_RP, LVL2_DEFAULT_HSM},

  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_LENGTH|LVL2_FLAG_HAVE_CHKSUM, FIVE_TERABYTES_VAL, &checksum_1_value, 1, NULL, DUMMY_VALUE, LVL2_DEFAULT_AL, LVL2_DEFAULT_RP, LVL2_DEFAULT_HSM},
  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_LENGTH|LVL2_FLAG_HAVE_CHKSUM, FIVE_TERABYTES_VAL, &checksum_2_value, 1, NULL, DUMMY_VALUE, LVL2_DEFAULT_AL, LVL2_DEFAULT_RP, LVL2_DEFAULT_HSM},

  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_LENGTH|LVL2_FLAG_HAVE_CHKSUM, FIVE_TERABYTES_VAL, &checksum_1_value, 1, NULL, DUMMY_VALUE, LVL2_DEFAULT_AL, LVL2_DEFAULT_RP, LVL2_DEFAULT_HSM},
  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_LENGTH|LVL2_FLAG_HAVE_CHKSUM, FIVE_TERABYTES_VAL, &checksum_2_value, 1, NULL, DUMMY_VALUE, LVL2_DEFAULT_AL, LVL2_DEFAULT_RP, LVL2_DEFAULT_HSM},

  // Test 11..  The C1 values

  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_CHKSUM, LVL2_DEFAULT_LENGTH, &checksum_1_value, 1, NULL, DUMMY_VALUE, LVL2_DEFAULT_AL, LVL2_DEFAULT_RP, LVL2_DEFAULT_HSM},
  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_CHKSUM, LVL2_DEFAULT_LENGTH, &checksum_2_value, 1, NULL, DUMMY_VALUE, LVL2_DEFAULT_AL, LVL2_DEFAULT_RP, LVL2_DEFAULT_HSM},

  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_CHKSUM, LVL2_DEFAULT_LENGTH, checksum_12_value,  2, NULL, DUMMY_VALUE, LVL2_DEFAULT_AL, LVL2_DEFAULT_RP, LVL2_DEFAULT_HSM},
  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_CHKSUM, LVL2_DEFAULT_LENGTH, checksum_12_value,  2, NULL, DUMMY_VALUE, LVL2_DEFAULT_AL, LVL2_DEFAULT_RP, LVL2_DEFAULT_HSM},
  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_CHKSUM, LVL2_DEFAULT_LENGTH, checksum_13_value,  2, NULL, DUMMY_VALUE, LVL2_DEFAULT_AL, LVL2_DEFAULT_RP, LVL2_DEFAULT_HSM},
  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_CHKSUM, LVL2_DEFAULT_LENGTH, checksum_13_value,  2, NULL, DUMMY_VALUE, LVL2_DEFAULT_AL, LVL2_DEFAULT_RP, LVL2_DEFAULT_HSM},
  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_CHKSUM, LVL2_DEFAULT_LENGTH, checksum_23_value,  2, NULL, DUMMY_VALUE, LVL2_DEFAULT_AL, LVL2_DEFAULT_RP, LVL2_DEFAULT_HSM},
  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_CHKSUM, LVL2_DEFAULT_LENGTH, checksum_23_value,  2, NULL, DUMMY_VALUE, LVL2_DEFAULT_AL, LVL2_DEFAULT_RP, LVL2_DEFAULT_HSM},
  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_CHKSUM, LVL2_DEFAULT_LENGTH, checksum_123_value, 3, NULL, DUMMY_VALUE, LVL2_DEFAULT_AL, LVL2_DEFAULT_RP, LVL2_DEFAULT_HSM},

  // Test 20..  Check repeated values
  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_CHKSUM, LVL2_DEFAULT_LENGTH, &checksum_1_value,  1, NULL, DUMMY_VALUE, LVL2_DEFAULT_AL, LVL2_DEFAULT_RP, LVL2_DEFAULT_HSM},
  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_CHKSUM, LVL2_DEFAULT_LENGTH, checksum_12_value,  2, NULL, DUMMY_VALUE, LVL2_DEFAULT_AL, LVL2_DEFAULT_RP, LVL2_DEFAULT_HSM},
  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_CHKSUM, LVL2_DEFAULT_LENGTH, checksum_12_value,  2, NULL, DUMMY_VALUE, LVL2_DEFAULT_AL, LVL2_DEFAULT_RP, LVL2_DEFAULT_HSM},
  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_CHKSUM, LVL2_DEFAULT_LENGTH, checksum_123_value, 3, NULL, DUMMY_VALUE, LVL2_DEFAULT_AL, LVL2_DEFAULT_RP, LVL2_DEFAULT_HSM},
  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_CHKSUM, LVL2_DEFAULT_LENGTH, checksum_123_value, 3, NULL, DUMMY_VALUE, LVL2_DEFAULT_AL, LVL2_DEFAULT_RP, LVL2_DEFAULT_HSM},
  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_CHKSUM, LVL2_DEFAULT_LENGTH, checksum_123_value, 3, NULL, DUMMY_VALUE, LVL2_DEFAULT_AL, LVL2_DEFAULT_RP, LVL2_DEFAULT_HSM},
  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_CHKSUM, LVL2_DEFAULT_LENGTH, checksum_123_value, 3, NULL, DUMMY_VALUE, LVL2_DEFAULT_AL, LVL2_DEFAULT_RP, LVL2_DEFAULT_HSM},

  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_LENGTH|LVL2_FLAG_HAVE_CHKSUM, FIVE_TERABYTES_VAL, &checksum_1_value, 1, NULL, DUMMY_VALUE, LVL2_DEFAULT_AL, LVL2_DEFAULT_RP, LVL2_DEFAULT_HSM},
  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_LENGTH|LVL2_FLAG_HAVE_CHKSUM, FIVE_TERABYTES_VAL, &checksum_2_value, 1, NULL, DUMMY_VALUE, LVL2_DEFAULT_AL, LVL2_DEFAULT_RP, LVL2_DEFAULT_HSM},

  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_LENGTH|LVL2_FLAG_HAVE_CHKSUM, FIVE_TERABYTES_VAL, checksum_12_value, 2, NULL, DUMMY_VALUE, LVL2_DEFAULT_AL, LVL2_DEFAULT_RP, LVL2_DEFAULT_HSM},
  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_LENGTH|LVL2_FLAG_HAVE_CHKSUM, FIVE_TERABYTES_VAL, checksum_12_value, 2, NULL, DUMMY_VALUE, LVL2_DEFAULT_AL, LVL2_DEFAULT_RP, LVL2_DEFAULT_HSM},
  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_LENGTH|LVL2_FLAG_HAVE_CHKSUM, FIVE_TERABYTES_VAL, checksum_13_value, 2, NULL, DUMMY_VALUE, LVL2_DEFAULT_AL, LVL2_DEFAULT_RP, LVL2_DEFAULT_HSM},
  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_LENGTH|LVL2_FLAG_HAVE_CHKSUM, FIVE_TERABYTES_VAL, checksum_123_value, 3, NULL, DUMMY_VALUE, LVL2_DEFAULT_AL, LVL2_DEFAULT_RP, LVL2_DEFAULT_HSM},

  // Test 33.. Multiple lines check
  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_LENGTH|LVL2_FLAG_HAVE_CHKSUM, FIVE_TERABYTES_VAL, &checksum_2_value, 1, NULL, DUMMY_VALUE, LVL2_DEFAULT_AL, LVL2_DEFAULT_RP, LVL2_DEFAULT_HSM},


  // Check with just uc flags specified
  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_NOTHING, LVL2_DEFAULT_LENGTH, NULL,               0, NULL, DUMMY_VALUE, LVL2_DEFAULT_AL, LVL2_DEFAULT_RP, LVL2_DEFAULT_HSM},
  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_CHKSUM,  LVL2_DEFAULT_LENGTH, &checksum_1_value,  1, NULL, DUMMY_VALUE, LVL2_DEFAULT_AL, LVL2_DEFAULT_RP, LVL2_DEFAULT_HSM},
  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_CHKSUM,  LVL2_DEFAULT_LENGTH, &checksum_2_value,  1, NULL, DUMMY_VALUE, LVL2_DEFAULT_AL, LVL2_DEFAULT_RP, LVL2_DEFAULT_HSM},
  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_CHKSUM,  LVL2_DEFAULT_LENGTH, &checksum_3_value,  1, NULL, DUMMY_VALUE, LVL2_DEFAULT_AL, LVL2_DEFAULT_RP, LVL2_DEFAULT_HSM},
  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_CHKSUM,  LVL2_DEFAULT_LENGTH, checksum_12_value,  2, NULL, DUMMY_VALUE, LVL2_DEFAULT_AL, LVL2_DEFAULT_RP, LVL2_DEFAULT_HSM},
  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_CHKSUM,  LVL2_DEFAULT_LENGTH, checksum_23_value,  2, NULL, DUMMY_VALUE, LVL2_DEFAULT_AL, LVL2_DEFAULT_RP, LVL2_DEFAULT_HSM},
  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_CHKSUM,  LVL2_DEFAULT_LENGTH, checksum_13_value,  2, NULL, DUMMY_VALUE, LVL2_DEFAULT_AL, LVL2_DEFAULT_RP, LVL2_DEFAULT_HSM},
  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_CHKSUM,  LVL2_DEFAULT_LENGTH, checksum_123_value, 3, NULL, DUMMY_VALUE, LVL2_DEFAULT_AL, LVL2_DEFAULT_RP, LVL2_DEFAULT_HSM},

  // Test 42.. Check with c and uc flags specified 
  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_CHKSUM, LVL2_DEFAULT_LENGTH, checksum_12_value,  2, NULL, DUMMY_VALUE, LVL2_DEFAULT_AL, LVL2_DEFAULT_RP, LVL2_DEFAULT_HSM},
  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_CHKSUM, LVL2_DEFAULT_LENGTH, checksum_13_value,  2, NULL, DUMMY_VALUE, LVL2_DEFAULT_AL, LVL2_DEFAULT_RP, LVL2_DEFAULT_HSM},
  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_CHKSUM, LVL2_DEFAULT_LENGTH, checksum_123_value, 3, NULL, DUMMY_VALUE, LVL2_DEFAULT_AL, LVL2_DEFAULT_RP, LVL2_DEFAULT_HSM},
  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_CHKSUM, LVL2_DEFAULT_LENGTH, checksum_123_value, 3, NULL, DUMMY_VALUE, LVL2_DEFAULT_AL, LVL2_DEFAULT_RP, LVL2_DEFAULT_HSM},

  /* Specific examples, to catch bugs */
  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_LENGTH|LVL2_FLAG_HAVE_CHKSUM|LVL2_FLAG_HAVE_HSM, 84195046, &checksum_4_value, 1, NULL, DUMMY_VALUE, LVL2_DEFAULT_AL, LVL2_DEFAULT_RP, l2_hsm_pool_with_hsm},

    /* Test 47.. Check access-latency and retention-policy by themselves are OK */
  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_AL, LVL2_DEFAULT_LENGTH, NULL, 0, NULL, DUMMY_VALUE, l2_al_online,   LVL2_DEFAULT_RP, LVL2_DEFAULT_HSM},
  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_AL, LVL2_DEFAULT_LENGTH, NULL, 0, NULL, DUMMY_VALUE, l2_al_nearline, LVL2_DEFAULT_RP, LVL2_DEFAULT_HSM},
  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_RP, LVL2_DEFAULT_LENGTH, NULL, 0, NULL, DUMMY_VALUE, LVL2_DEFAULT_AL, l2_rp_replica,   LVL2_DEFAULT_HSM},
  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_RP, LVL2_DEFAULT_LENGTH, NULL, 0, NULL, DUMMY_VALUE, LVL2_DEFAULT_AL, l2_rp_custodial, LVL2_DEFAULT_HSM},
  /* Duplicates are allowed, if they are the same value */
  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_AL, LVL2_DEFAULT_LENGTH, NULL, 0, NULL, DUMMY_VALUE, l2_al_online,   LVL2_DEFAULT_RP, LVL2_DEFAULT_HSM},
  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_AL, LVL2_DEFAULT_LENGTH, NULL, 0, NULL, DUMMY_VALUE, l2_al_nearline, LVL2_DEFAULT_RP, LVL2_DEFAULT_HSM},
  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_RP, LVL2_DEFAULT_LENGTH, NULL, 0, NULL, DUMMY_VALUE, LVL2_DEFAULT_AL, l2_rp_replica,   LVL2_DEFAULT_HSM},
  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_RP, LVL2_DEFAULT_LENGTH, NULL, 0, NULL, DUMMY_VALUE, LVL2_DEFAULT_AL, l2_rp_custodial, LVL2_DEFAULT_HSM},
  /* Check either access-latency or retention-policy with other flags */
  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_LENGTH|LVL2_FLAG_HAVE_CHKSUM|LVL2_FLAG_HAVE_AL, FIVE_TERABYTES_VAL, &checksum_1_value, 1, NULL, DUMMY_VALUE, l2_al_online,   LVL2_DEFAULT_RP, LVL2_DEFAULT_HSM},
  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_LENGTH|LVL2_FLAG_HAVE_CHKSUM|LVL2_FLAG_HAVE_AL, FIVE_TERABYTES_VAL, &checksum_1_value, 1, NULL, DUMMY_VALUE, l2_al_nearline, LVL2_DEFAULT_RP, LVL2_DEFAULT_HSM},
  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_LENGTH|LVL2_FLAG_HAVE_CHKSUM|LVL2_FLAG_HAVE_RP, FIVE_TERABYTES_VAL, &checksum_1_value, 1, NULL, DUMMY_VALUE, LVL2_DEFAULT_AL, l2_rp_replica,   LVL2_DEFAULT_HSM},
  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_LENGTH|LVL2_FLAG_HAVE_CHKSUM|LVL2_FLAG_HAVE_RP, FIVE_TERABYTES_VAL, &checksum_1_value, 1, NULL, DUMMY_VALUE, LVL2_DEFAULT_AL, l2_rp_custodial, LVL2_DEFAULT_HSM},
  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_LENGTH|LVL2_FLAG_HAVE_CHKSUM|LVL2_FLAG_HAVE_AL, FIVE_TERABYTES_VAL, &checksum_1_value, 1, NULL, DUMMY_VALUE, l2_al_online,   LVL2_DEFAULT_RP, LVL2_DEFAULT_HSM},
  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_LENGTH|LVL2_FLAG_HAVE_CHKSUM|LVL2_FLAG_HAVE_AL, FIVE_TERABYTES_VAL, &checksum_1_value, 1, NULL, DUMMY_VALUE, l2_al_nearline, LVL2_DEFAULT_RP, LVL2_DEFAULT_HSM},
  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_LENGTH|LVL2_FLAG_HAVE_CHKSUM|LVL2_FLAG_HAVE_RP, FIVE_TERABYTES_VAL, &checksum_1_value, 1, NULL, DUMMY_VALUE, LVL2_DEFAULT_AL, l2_rp_replica,   LVL2_DEFAULT_HSM},
  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_LENGTH|LVL2_FLAG_HAVE_CHKSUM|LVL2_FLAG_HAVE_RP, FIVE_TERABYTES_VAL, &checksum_1_value, 1, NULL, DUMMY_VALUE, LVL2_DEFAULT_AL, l2_rp_custodial, LVL2_DEFAULT_HSM},
  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_LENGTH|LVL2_FLAG_HAVE_CHKSUM|LVL2_FLAG_HAVE_AL, FIVE_TERABYTES_VAL, &checksum_1_value, 1, NULL, DUMMY_VALUE, l2_al_online,   LVL2_DEFAULT_RP, LVL2_DEFAULT_HSM},
  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_LENGTH|LVL2_FLAG_HAVE_CHKSUM|LVL2_FLAG_HAVE_AL, FIVE_TERABYTES_VAL, &checksum_1_value, 1, NULL, DUMMY_VALUE, l2_al_nearline, LVL2_DEFAULT_RP, LVL2_DEFAULT_HSM},
  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_LENGTH|LVL2_FLAG_HAVE_CHKSUM|LVL2_FLAG_HAVE_RP, FIVE_TERABYTES_VAL, &checksum_1_value, 1, NULL, DUMMY_VALUE, LVL2_DEFAULT_AL, l2_rp_replica,   LVL2_DEFAULT_HSM},
  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_LENGTH|LVL2_FLAG_HAVE_CHKSUM|LVL2_FLAG_HAVE_RP, FIVE_TERABYTES_VAL, &checksum_1_value, 1, NULL, DUMMY_VALUE, LVL2_DEFAULT_AL, l2_rp_custodial, LVL2_DEFAULT_HSM},


  /**
   *  Test 67.. Check combinations of AL and RP: four combinations (two providing
   *  redundant information) are repeated four times
   */
  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_LENGTH|LVL2_FLAG_HAVE_CHKSUM|LVL2_FLAG_HAVE_AL|LVL2_FLAG_HAVE_RP, FIVE_TERABYTES_VAL, &checksum_1_value, 1, NULL, DUMMY_VALUE, l2_al_online, l2_rp_replica, LVL2_DEFAULT_HSM},
  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_LENGTH|LVL2_FLAG_HAVE_CHKSUM|LVL2_FLAG_HAVE_AL|LVL2_FLAG_HAVE_RP, FIVE_TERABYTES_VAL, &checksum_1_value, 1, NULL, DUMMY_VALUE, l2_al_online, l2_rp_replica, LVL2_DEFAULT_HSM},
  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_LENGTH|LVL2_FLAG_HAVE_CHKSUM|LVL2_FLAG_HAVE_AL|LVL2_FLAG_HAVE_RP, FIVE_TERABYTES_VAL, &checksum_1_value, 1, NULL, DUMMY_VALUE, l2_al_nearline, l2_rp_custodial, LVL2_DEFAULT_HSM},
  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_LENGTH|LVL2_FLAG_HAVE_CHKSUM|LVL2_FLAG_HAVE_AL|LVL2_FLAG_HAVE_RP, FIVE_TERABYTES_VAL, &checksum_1_value, 1, NULL, DUMMY_VALUE, l2_al_nearline, l2_rp_custodial, LVL2_DEFAULT_HSM},
  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_LENGTH|LVL2_FLAG_HAVE_CHKSUM|LVL2_FLAG_HAVE_AL|LVL2_FLAG_HAVE_RP, FIVE_TERABYTES_VAL, &checksum_1_value, 1, NULL, DUMMY_VALUE, l2_al_online, l2_rp_replica, LVL2_DEFAULT_HSM},
  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_LENGTH|LVL2_FLAG_HAVE_CHKSUM|LVL2_FLAG_HAVE_AL|LVL2_FLAG_HAVE_RP, FIVE_TERABYTES_VAL, &checksum_1_value, 1, NULL, DUMMY_VALUE, l2_al_online, l2_rp_replica, LVL2_DEFAULT_HSM},
  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_LENGTH|LVL2_FLAG_HAVE_CHKSUM|LVL2_FLAG_HAVE_AL|LVL2_FLAG_HAVE_RP, FIVE_TERABYTES_VAL, &checksum_1_value, 1, NULL, DUMMY_VALUE, l2_al_nearline, l2_rp_custodial, LVL2_DEFAULT_HSM},
  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_LENGTH|LVL2_FLAG_HAVE_CHKSUM|LVL2_FLAG_HAVE_AL|LVL2_FLAG_HAVE_RP, FIVE_TERABYTES_VAL, &checksum_1_value, 1, NULL, DUMMY_VALUE, l2_al_nearline, l2_rp_custodial, LVL2_DEFAULT_HSM},
  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_LENGTH|LVL2_FLAG_HAVE_CHKSUM|LVL2_FLAG_HAVE_AL|LVL2_FLAG_HAVE_RP, FIVE_TERABYTES_VAL, &checksum_1_value, 1, NULL, DUMMY_VALUE, l2_al_online, l2_rp_replica, LVL2_DEFAULT_HSM},
  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_LENGTH|LVL2_FLAG_HAVE_CHKSUM|LVL2_FLAG_HAVE_AL|LVL2_FLAG_HAVE_RP, FIVE_TERABYTES_VAL, &checksum_1_value, 1, NULL, DUMMY_VALUE, l2_al_online, l2_rp_replica, LVL2_DEFAULT_HSM},
  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_LENGTH|LVL2_FLAG_HAVE_CHKSUM|LVL2_FLAG_HAVE_AL|LVL2_FLAG_HAVE_RP, FIVE_TERABYTES_VAL, &checksum_1_value, 1, NULL, DUMMY_VALUE, l2_al_nearline, l2_rp_custodial, LVL2_DEFAULT_HSM},
  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_LENGTH|LVL2_FLAG_HAVE_CHKSUM|LVL2_FLAG_HAVE_AL|LVL2_FLAG_HAVE_RP, FIVE_TERABYTES_VAL, &checksum_1_value, 1, NULL, DUMMY_VALUE, l2_al_nearline, l2_rp_custodial, LVL2_DEFAULT_HSM},
  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_LENGTH|LVL2_FLAG_HAVE_CHKSUM|LVL2_FLAG_HAVE_AL|LVL2_FLAG_HAVE_RP, FIVE_TERABYTES_VAL, &checksum_1_value, 1, NULL, DUMMY_VALUE, l2_al_online, l2_rp_replica, LVL2_DEFAULT_HSM},
  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_LENGTH|LVL2_FLAG_HAVE_CHKSUM|LVL2_FLAG_HAVE_AL|LVL2_FLAG_HAVE_RP, FIVE_TERABYTES_VAL, &checksum_1_value, 1, NULL, DUMMY_VALUE, l2_al_online, l2_rp_replica, LVL2_DEFAULT_HSM},
  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_LENGTH|LVL2_FLAG_HAVE_CHKSUM|LVL2_FLAG_HAVE_AL|LVL2_FLAG_HAVE_RP, FIVE_TERABYTES_VAL, &checksum_1_value, 1, NULL, DUMMY_VALUE, l2_al_nearline, l2_rp_custodial, LVL2_DEFAULT_HSM},
  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_LENGTH|LVL2_FLAG_HAVE_CHKSUM|LVL2_FLAG_HAVE_AL|LVL2_FLAG_HAVE_RP, FIVE_TERABYTES_VAL, &checksum_1_value, 1, NULL, DUMMY_VALUE, l2_al_nearline, l2_rp_custodial, LVL2_DEFAULT_HSM},
  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_LENGTH|LVL2_FLAG_HAVE_CHKSUM|LVL2_FLAG_HAVE_AL|LVL2_FLAG_HAVE_RP, FIVE_TERABYTES_VAL, &checksum_1_value, 1, NULL, DUMMY_VALUE, l2_al_online, l2_rp_replica, LVL2_DEFAULT_HSM},
  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_LENGTH|LVL2_FLAG_HAVE_CHKSUM|LVL2_FLAG_HAVE_AL|LVL2_FLAG_HAVE_RP, FIVE_TERABYTES_VAL, &checksum_1_value, 1, NULL, DUMMY_VALUE, l2_al_online, l2_rp_replica, LVL2_DEFAULT_HSM},
  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_LENGTH|LVL2_FLAG_HAVE_CHKSUM|LVL2_FLAG_HAVE_AL|LVL2_FLAG_HAVE_RP, FIVE_TERABYTES_VAL, &checksum_1_value, 1, NULL, DUMMY_VALUE, l2_al_nearline, l2_rp_custodial, LVL2_DEFAULT_HSM},
  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_LENGTH|LVL2_FLAG_HAVE_CHKSUM|LVL2_FLAG_HAVE_AL|LVL2_FLAG_HAVE_RP, FIVE_TERABYTES_VAL, &checksum_1_value, 1, NULL, DUMMY_VALUE, l2_al_nearline, l2_rp_custodial, LVL2_DEFAULT_HSM},

  /* Test 87.. Also check HSM flags */
  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_HSM, LVL2_DEFAULT_LENGTH, NULL, 0, NULL, DUMMY_VALUE, LVL2_DEFAULT_AL, LVL2_DEFAULT_RP, l2_hsm_pool_with_hsm},
  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_HSM, LVL2_DEFAULT_LENGTH, NULL, 0, NULL, DUMMY_VALUE, LVL2_DEFAULT_AL, LVL2_DEFAULT_RP, l2_hsm_pool_without_hsm},
  /* Repeated values are OK, if the values are the same */
  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_HSM, LVL2_DEFAULT_LENGTH, NULL, 0, NULL, DUMMY_VALUE, LVL2_DEFAULT_AL, LVL2_DEFAULT_RP, l2_hsm_pool_with_hsm},
  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_HSM, LVL2_DEFAULT_LENGTH, NULL, 0, NULL, DUMMY_VALUE, LVL2_DEFAULT_AL, LVL2_DEFAULT_RP, l2_hsm_pool_without_hsm},
  /* Also with other flags */
  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_HSM|LVL2_FLAG_HAVE_LENGTH|LVL2_FLAG_HAVE_CHKSUM, FIVE_TERABYTES_VAL, &checksum_1_value, 1, NULL, DUMMY_VALUE, LVL2_DEFAULT_AL, LVL2_DEFAULT_RP, l2_hsm_pool_with_hsm},
  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_HSM|LVL2_FLAG_HAVE_LENGTH|LVL2_FLAG_HAVE_CHKSUM, FIVE_TERABYTES_VAL, &checksum_1_value, 1, NULL, DUMMY_VALUE, LVL2_DEFAULT_AL, LVL2_DEFAULT_RP, l2_hsm_pool_without_hsm},
  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_HSM|LVL2_FLAG_HAVE_LENGTH|LVL2_FLAG_HAVE_CHKSUM, FIVE_TERABYTES_VAL, &checksum_1_value, 1, NULL, DUMMY_VALUE, LVL2_DEFAULT_AL, LVL2_DEFAULT_RP, l2_hsm_pool_with_hsm},
  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_HSM|LVL2_FLAG_HAVE_LENGTH|LVL2_FLAG_HAVE_CHKSUM, FIVE_TERABYTES_VAL, &checksum_1_value, 1, NULL, DUMMY_VALUE, LVL2_DEFAULT_AL, LVL2_DEFAULT_RP, l2_hsm_pool_without_hsm},
  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_HSM|LVL2_FLAG_HAVE_LENGTH|LVL2_FLAG_HAVE_CHKSUM, FIVE_TERABYTES_VAL, &checksum_1_value, 1, NULL, DUMMY_VALUE, LVL2_DEFAULT_AL, LVL2_DEFAULT_RP, l2_hsm_pool_with_hsm},
  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_HSM|LVL2_FLAG_HAVE_LENGTH|LVL2_FLAG_HAVE_CHKSUM, FIVE_TERABYTES_VAL, &checksum_1_value, 1, NULL, DUMMY_VALUE, LVL2_DEFAULT_AL, LVL2_DEFAULT_RP, l2_hsm_pool_without_hsm},


  /* The Christmas tree example */
  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_HSM|LVL2_FLAG_HAVE_AL|LVL2_FLAG_HAVE_RP|LVL2_FLAG_HAVE_LENGTH|LVL2_FLAG_HAVE_CHKSUM, FIVE_TERABYTES_VAL, &checksum_1_value, 1, NULL, DUMMY_VALUE, l2_al_nearline, l2_rp_custodial, l2_hsm_pool_with_hsm},


  /* The real-world example from NDGF */
  {NULL, DUMMY_LEN, LVL2_FLAG_HAVE_HSM|LVL2_FLAG_HAVE_AL|LVL2_FLAG_HAVE_RP|LVL2_FLAG_HAVE_LENGTH|LVL2_FLAG_HAVE_CHKSUM, 979660604, &checksum_ndgf_value, 1, NULL, DUMMY_VALUE, l2_al_online, l2_rp_replica, l2_hsm_pool_without_hsm},

};


lvl2_info_t expected_empty_info = EMPTY_INFO;



int check_expected_failures();
int check_expected_successes();
int check_default_values();
int test_expected_fail( const char *test_str);
int test_expected_success( const char *test_str, lvl2_info_t *info);
int cmp_results( lvl2_info_t *expected, lvl2_info_t *actual);
int cmp_checksum_value( checksum_value_t *expected, checksum_value_t *actual);

int main( int argc, char *argv[])
{
  int result=0;
  
  result |= check_default_values();
  result |= check_expected_successes();


  /* Redirect stderr so we don't see error messages during expected failures */
  if( freopen( "/dev/null", "w", stderr) == NULL) {
    perror( "freopen");
    return 1;
  }

  result |= check_expected_failures();  

  if( result == 0)
    printf( "All tests pass.\n");

  return result;
}


int check_default_values()
{
  lvl2_info_t info;

  lvl2_reset( &info);

  if( cmp_results( &expected_empty_info, &info)) {
    printf( "lvl2_reset() produced wrong result.\n");
    return 1;
  }
  
  return 0;
}



int check_expected_failures()
{
  int i, result=0;
  lvl2_info_t info;

  lvl2_reset( &info);

  for( i = 0; i < sizeof ( expected_fail) / sizeof( char *); i++)
    result |= test_expected_fail( expected_fail [i]);

  return result;
}


int check_expected_successes()
{
  int i, result = 0;
  lvl2_info_t info;

  lvl2_reset( &info);

  for( i = 0; i < sizeof( expected_success) / sizeof( char *); i++) {

    if( test_expected_success( expected_success [i], &info)) {
      result |= 1;
      continue;
    }


    if( i < sizeof( expected_result) / sizeof( lvl2_info_t)) {

      if( cmp_results( &expected_result [i], &info)) {
	printf( "Parser produced incorrect values for test %u: \"%s\"\n", i, expected_success[i]);
	result |= 1;
      }
    } else
      printf( "No corresponding reference value could be found for test %u\n", i);
  }

  if( i < sizeof( expected_result) / sizeof( lvl2_info_t))
    printf( "More expected results than test level-2 data (%u test strings, %u results)\n", i,
	    sizeof( expected_result) / sizeof( lvl2_info_t));

  lvl2_final_flush( &info);

  return result;
}


int test_expected_fail( const char *test_str)
{
  lvl2_info_t info;

  lvl2_reset( &info);

  if( lvl2_parse_data( &info, test_str, strlen( test_str), "")) {
    printf( "Unexpect success with \"%s\"\n", test_str);
    return 1;
  }

  lvl2_final_flush( &info);

  return 0;
}


int test_expected_success( const char *test_str, lvl2_info_t *info)
{
  lvl2_flush( info);

  if( !lvl2_parse_data( info, test_str, strlen( test_str), "test")) {
    printf( "Unexpect failure with \"%s\"\n", test_str);
    return 1;
  }

  return 0;
}


/**
 *  Compare two lvl2_info_t structures.
 *
 *  Returns 0 if they're the same, 1 otherwise.
 */
int cmp_results( lvl2_info_t *expected, lvl2_info_t *actual)
{
  int i, j, found, mismatch;
  int result = 0;

  if( expected->flags != actual->flags) {
    printf( "  flags mismatch: 0x%02x != 0x%02x\n", expected->flags, actual->flags);
    result = 1;
  }

  if( expected->length != actual->length) {
    printf( "  length mismatch: %llu != %llu\n", expected->length, actual->length);
    result = 1;
  }

  if( expected->access_latency != actual->access_latency) {
    printf( "  access latency mismatch: %u != %u\n", expected->access_latency, actual->access_latency);
    result = 1;
  }

  if( expected->retention_policy != actual->retention_policy) {
    printf( "  retention policy mismatch: %u != %u\n", expected->retention_policy, actual->retention_policy);
    result = 1;
  }

  if( expected->hsm_pool != actual->hsm_pool) {
    printf( "  HSM pool mismatch: %u != %u\n", expected->hsm_pool, actual->hsm_pool);
    result = 1;
  }

  if( !expected->checksums && actual->checksums) {
    printf( "  parsed data unexpectedly contains checksums\n");
    result = 1;
  }


  if( expected->checksums && !actual->checksums) {
    printf( "  parsed data unexpectedly bereft of checksums\n");
    result = 1;
  }

  if( expected->checksum_count != actual->checksum_count) {
    printf( "  no. checksums mismatch: expected %u, got %u\n", expected->checksum_count,
	    actual->checksum_count);
    result = 1;
  }


  // NB check that checksums match: order doesn't matter.
  if( expected->checksums && actual->checksums) {
    mismatch = 0;

    for( i = 0; i < expected->checksum_count; i++) {
      found = 0;

      for( j = 0; j < actual->checksum_count; j++)
	if( !cmp_checksum_value( &expected->checksums[i], &actual->checksums[j]))
	  found = 1;

      if( !found) {
 	printf( "  failed to find checksum: %u %s\n",
		expected->checksums [i].type,
		expected->checksums [i].value);
      }

      result |= found ? 0 : 1;
    }

    if( actual->checksums[i].value != NULL) {
      printf( "  checksum stack not terminated correctly\n");
      result = 1;
    }
  }

  return result;
}


/**
 *  Check whether two checksum values match.
 *
 *  Returns 0 if they match, 1 otherwise.
 */
int cmp_checksum_value( checksum_value_t *expected, checksum_value_t *actual)
{
  if( expected->type != actual->type)
    return 1;

  if( strcmp( expected->value, actual->value))
    return 1;
  
  return 0;
}
