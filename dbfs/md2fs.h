#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "md2types.h"

typedef unsigned long md_ip ;

typedef struct {

        md_id_t    id ;
           long    offset ;
           long    size ;
  unsigned long    mode ;
            int    level;
            MDL  * mdl ;

} md_fs_block ;

#define MD_FS_BUFFER_SIZE  (512) 

typedef struct {

    md_fs_block  block ;
	char         buffer[MD_FS_BUFFER_SIZE] ;
	long         maxSize ;
	long         size ;
	long         offset ;
	
} md_fs_format ;

#define mdFsREAD     (1)
#define mdFsWRITE    (2)

#define MDEfsACCESS   (-101)

/* --------------------------------------------------------------------- */
#define MD_MAX_DOMAIN_SIZE   (128)
#define MD_MAX_HN_SIZE   (64)
#define MD_MAX_IP_SIZE   (4)
#define MD_MAX_IF_SIZE   (8)
#define MD_MAX_HW_SIZE   (16)
#define MD_MAX_OS_SIZE   (16)
#define MD_MAX_VER_SIZE  (16)
#define MD_MAX_MASK_SIZE (32)
#define MD_MAX_MP_SIZE   (64)
#define MD_NUM_INTERFACES   (16)
#define MD_NUM_MOUNTPOINTS  (16)

typedef struct {
   char  name[MD_MAX_HN_SIZE] ;
   char  ifType[MD_MAX_IF_SIZE] ;
  md_ip  ip ;
   int   priority ;
} md_interface_info ;

typedef struct {
            char    mountpoint[MD_MAX_MP_SIZE] ;
   md_permission    permission ;
} md_mountpoint_info ;

typedef struct {
                long  md_version ;
                char  domain[MD_MAX_DOMAIN_SIZE] ;
   md_interface_info  generic ;
                char  hardware[MD_MAX_HW_SIZE] ;
                char  system[MD_MAX_OS_SIZE];
                char  version[MD_MAX_VER_SIZE] ;
   md_interface_info  interfaces[MD_NUM_INTERFACES] ;
  md_mountpoint_info  mountpoints[MD_NUM_MOUNTPOINTS] ;

} md_host_info ;

int   mdFs_FOpen( MDL * mdl , md_fs_format *f , md_id_t id , int level , char *mode );
int   mdFs_Ffputs(const char *s, md_fs_format *f);
int   mdFs_Ffputc( int c , md_fs_format *f );
int   mdFs_Ffgetc( md_fs_format *f );
char *mdFs_Ffgets(char *s, int n, md_fs_format *f);
int   mdFs_Open( MDL * mdl , md_fs_block *p , md_id_t id , int level , char *mode );
int   mdFs_Read( md_fs_block *p , char *buffer , long size );
int   mdFs_Write( md_fs_block *p , char *buffer , long size );

int    md2ObjectToId( MDL *mdl , md_id_t id , char *str , md_id_t *resID ) ;

int md2_GetConfLine( char *string , char *ptr[] , int ptrSize );

#define md_zero_struct(s)   memset((char*)s,0,sizeof(*s))
void    md_dstring_to_permission( md_permission *ip , char *s );
void    md_dotted_to_ip( md_ip *ip , char *string );
char *  md_ip_to_dotted(  md_ip *ip );
int     md2_GetHostInfo( md_fs_format *f , md_host_info *info );
int     md2_PrintHostInfo( FILE * f , md_host_info *info );

