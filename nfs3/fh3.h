/*
 * Copyrighted as an unpublished work.
 * (c) Copyright 1996-2004 DESY Hamburg DMG-Division
 * All rights reserved.
 *
 *
 * RESTRICTED RIGHTS
 *
 * These programs are supplied under a license.  They may be used,
 * disclosed, and/or copied only as permitted under such license
 * agreement.  Any copy must contain the above copyright notice and
 * this restricted rights notice.  Use, copying, and/or disclosure
 * of the programs is strictly prohibited unless otherwise provided
 * in the license agreement.
 */
#ifndef __FH__H__
#define __FH__H__
/*
 *    some helper for nfs -> nfs3 migration
 */
 
#define NFS3_MAXDATA   (8*1024)

#include <sys/time.h>

#include "mount3_prot.h"
#include "nfs3_prot.h"

#include "md2types.h"
 
#define NFSERR_NOANSWER   (-222)
#ifdef solaris
#define svc_getcaller(x)  ((struct sockaddr_in*)(svc_getrpccaller(x)->buf))
#endif

#define  credit_uid(r)   (((struct authunix_parms*)r->rq_clntcred)->aup_uid)
#define  credit_gid(r)   (((struct authunix_parms*)r->rq_clntcred)->aup_gid)
#define  credit_host(r)  (svc_getcaller(r->rq_xprt)->sin_addr.s_addr)
#define  credit_aup(r)   ((struct authunix_parms*)r->rq_clntcred)

#ifdef honour_gids
#define  credit_copyGids(r,a)  {int i,l;l=credit_aup(r)->aup_len;\
                                l=l>15?15:l;\
                                for(i=0;i<l;i++)(a)->gids[i]=\
                                credit_aup(r)->aup_gids[i];\
                                (a)->gidsLen=l;}
#else
#define  credit_copyGids(r,a)   {(a)->gidsLen=0;}
#endif

#define  REAL_AUTH
#ifdef REAL_AUTH
#define  make_auth(a,c,r)   {memset((char*)(a),0,sizeof(*a));\
                             (a)->uid=credit_uid(r);(a)->gid=credit_gid(r);\
                             (a)->host=credit_host(r);\
			     gettimeofday(&((a)->timestamp),NULL);\
                             credit_copyGids(r,a);\
                             fh3_make_root_trusted(a);}
#else
#define  make_auth(a,c,r)  memset((char*)a,0,sizeof(*a))
#endif
 
#define FH3_READ_OWNER   (0x00100)
#define FH3_WRITE_OWNER  (0x00080)
#define FH3_EXEC_OWNER   (0x00040)
#define FH3_READ_GROUP   (0x00020)
#define FH3_WRITE_GROUP  (0x00010)
#define FH3_EXEC_GROUP   (0x00008)
#define FH3_READ_OTHER   (0x00004)
#define FH3_WRITE_OTHER  (0x00002)
#define FH3_EXEC_OTHER   (0x00001)


#ifdef NFS_MAXPATHLEN
#undef NFS_MAXPATHLEN
#endif
#define NFS_MAXPATHLEN 256

void    fh3_init( int argc , char *argv[] );
void    fh3_exit();
int     fh3_create( md_auth *auth , fhandle3 *fh, char *path);
nfsstat3 fh3_getattr( nfs_fh3 * fh  , fattr3 *attr, md_auth *auth ) ;
nfsstat3 fh3_opendir( nfs_fh3 *fh , long *cookie ,md_auth *auth) ;
nfsstat3 fh3_readdir(long *cookie , md_dir_item *item ) ; 
nfsstat3 fh3_closedir( )  ;
nfsstat3 fh3_lookup( diropargs3 * fh  , 
                    nfs_fh3 *resFh , 
                    fattr3 *resAttr ,
                    fattr3 *dirAttr ,
                    md_auth *auth) ;
nfsstat3 fh3_read_data( READ3args *argp , READ3res *res , md_auth *auth ) ;
nfsstat3 fh3_setattr( SETATTR3args * argp, SETATTR3res *res , md_auth  *auth);
nfsstat3 fh3_s2u_attr(  sattr3 *a , md_unix *attr );
nfsstat3 fh3_u2f_attr( md_unix *s , fattr3 *attr );
nfsstat3 fh3_command(  md_auth *auth , md_id_t id , md_permission perm,
                        md_long  com , md_long  arg  ) ;

nfsstat3 fh3_mkdir( MKDIR3args * argp  , MKDIR3res *res , md_auth *auth ) ;
nfsstat3 fh3_write_data( WRITE3args *argp , WRITE3res *res , md_auth *auth );
  
/*
int     fh_canShow( nfs_fh *fh , md_dir_item *item , md_auth *auth ) ;
int     fh_mapHandle( md_permission *p , md_permission perm , md_auth *auth) ;
nfsstat fh_setattr( sattrargs * argp, attrstat *res , md_auth  *auth) ;
nfsstat fh_getattrLow( mdFhandle * fh , fattr *attr );
nfsstat fh_addToDirectory( linkargs * argp , md_auth *auth  ); 
nfsstat fh_rmdir( diropargs * argp , md_auth *auth  ) ;
nfsstat fh_createFile( createargs * argp  , diropres *res , md_auth *auth ) ;
nfsstat fh_write_data( writeargs *argp , attrstat *res , md_auth *auth );
nfsstat fh_read_data( readargs *argp , readres *res , md_auth *auth ) ;
nfsstat fh_rename( renameargs * argp  , md_auth *auth);
nfsstat fh_symlink( symlinkargs * argp , md_auth *auth );
nfsstat fh_nfs_commands( mdFhandle *fHandle, char *fName,char *tPath);
nfsstat fh_do_commands( mdFhandle *dir, char *name, int argc , char *argv[] );
nfsstat fh_readlink( mdFhandle *m , readlinkres *res, md_auth *auth ) ;
nfsstat fh_command(  md_auth *auth , md_id_t id , md_permission perm,
                     md_long  com , md_long  arg  ) ;
void    fh_make_root_trusted(  md_auth *auth ) ;
nfsstat fh_s2u_attr(  sattr *a , md_unix *attr ) ;
nfsstat fh_u2f_attr( md_unix *s , fattr *attr ) ;


int mallocfailed();
int pseudo_inode(u_long inode,u_short dev);

char *  stringAuth(  md_auth *a ) ;
char *  stringHeader(  md_auth *a ) ;

*/

#endif
