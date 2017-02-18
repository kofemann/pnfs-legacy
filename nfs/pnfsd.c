/*
 * Copyrighted as an unpublished work.
 * (c) Copyright 1996,1997,1998 DESY Hamburg DMG-Division
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
#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>

#include "md2log.h"


#include "pnfsd.h"

#define knownclient(x)      ((clnt_param*)(x))
static struct timeval TIMEOUT = { 25, 0 };

#define  clear_result(r)   memset((char*)&r,0,sizeof(r))

#define md2pPrintfS  md2pPrintf
#define md2pPrintfE  md2pPrintf

/* ====================================================================== */

static char *pname = "pnfsd";
static char argbuf[1024];
extern int  use_hardlinks  ;
extern int  use_trust  ;

/* ====================================================================== */
   void * nfsproc_null_2( void * argp, struct svc_req * rqstp)
/* ====================================================================== */
{
    static char res;

    clear_result( res ) ;

    return ((void *)&res);
}

/* ====================================================================== */
   attrstat * nfsproc_getattr_2( nfs_fh * argp,  struct svc_req * rqstp)
/* ====================================================================== */
{
    static attrstat res;
    clnt_param  *cp;
    md_auth      auth ;

    clear_result( res ) ;
    if( ( cp = knownclient(rqstp) ) == NULL){
        res.status = NFSERR_ACCES;
        return (&res);
    }
    make_auth( &auth , cp , rqstp)  ;
    md2pPrintfS(md2pMOREINFO,"%s - getattr ", stringHeader(&auth) );

    res.status = fh_getattr( argp , &(res.attrstat_u.attributes) , &auth ) ;
    md2pPrintfE(md2pMOREINFO," -> %d\n",res.status );
    return (res.status==NFSERR_NOANSWER?NULL:&res);
}

/* ====================================================================== */
   attrstat * nfsproc_setattr_2( sattrargs * argp, struct svc_req * rqstp)
/* ====================================================================== */
{
    static attrstat res;
    clnt_param  *cp;
    mdFhandle   *m  = (mdFhandle *)&argp->file ;
    md_auth      auth ;
    
    clear_result( res ) ;
    if( ( cp = knownclient(rqstp) ) == NULL){
        res.status = NFSERR_ACCES;
        return (&res);
    }

    make_auth( &auth , cp , rqstp)  ;
    md2pPrintfS(md2pMODINFO,"%s - setattr ", stringHeader(&auth) );
    						 
    res.status = fh_setattr( argp , &res , &auth ) ;
    
    md2pPrintfE(md2pMODINFO," -> %d\n",res.status );

    return (res.status==NFSERR_NOANSWER?NULL:&res);

}


/* ====================================================================== */
void * nfsproc_root_2( void * argp, struct svc_req * rqstp)
/* ====================================================================== */
{
    static char res;
    clnt_param  *cp;
    clear_result( res ) ;
    if( ( cp = knownclient(rqstp) ) == NULL){
        return (&res);
    }
    return ((void *)&res);
}

/* ====================================================================== */
diropres * nfsproc_lookup_2( diropargs * argp, struct svc_req * rqstp)
/* ====================================================================== */
{
    static diropres res;
    clnt_param  *cp;
    md_auth      auth ;

    clear_result( res ) ;
    if( ( cp = knownclient(rqstp) ) == NULL){
        res.status = NFSERR_ACCES;
        return (&res);
    }
    make_auth( &auth , cp , rqstp)  ;
    md2pPrintfS(md2pINFO,"%s - lookup ", stringHeader(&auth) );
    res.status = fh_lookup( argp , 
                            &(res.diropres_u.diropres.file) ,
                            &(res.diropres_u.diropres.attributes),
                            &auth  ) ;

    md2pPrintfE(md2pINFO," -> %d\n",res.status );
    return (res.status==NFSERR_NOANSWER?NULL:&res);
}


/* ====================================================================== */
   readlinkres * nfsproc_readlink_2( nfs_fh * argp, struct svc_req * rqstp)
/* ====================================================================== */
{
    static readlinkres res;
    clnt_param  *cp;
    md_auth      auth ;

    clear_result( res ) ;
    if( ( cp = knownclient(rqstp) ) == NULL){
        res.status = NFSERR_ACCES;
        return (&res);
    }
    make_auth( &auth , cp , rqstp)  ;
    md2pPrintfS(md2pINFO,"%s - readlink ", stringHeader(&auth) );
    res.status = fh_readlink( (mdFhandle*)argp , &res , &auth ) ;
    md2pPrintfE(md2pINFO," -> %d\n",res.status );
    return (res.status==NFSERR_NOANSWER?NULL:&res);
}


static char iobuf[NFS_MAXDATA];

/* ====================================================================== */
   readres * nfsproc_read_2( readargs * argp, struct svc_req * rqstp)
/* ====================================================================== */
{
    static readres res;
    clnt_param  *cp;
    mdFhandle *m = (mdFhandle *)&argp->file ;
    md_auth      auth ;

    clear_result( res ) ;
    if( ( cp = knownclient(rqstp) ) == NULL){
        res.status = NFSERR_ACCES;
        return (&res);
    }
  
    make_auth( &auth , cp , rqstp)  ;
    memcpy( (char*)&auth.mountID , (char*)&(m->mountID) , sizeof(md_id_t) );
    md2pPrintfS(md2pINFO,"%s - read ", stringHeader(&auth) );
    res.status = fh_read_data( argp , &res , &auth ) ;
    md2pPrintfE(md2pINFO," -> %d\n",res.status );
    return (res.status==NFSERR_NOANSWER?NULL:&res);
}

/* ====================================================================== */
    void * nfsproc_writecache_2( void * argp, struct svc_req * rqstp)
/* ====================================================================== */
{
    static char res;

    clear_result( res ) ;
    return ((void *)&res);
}

/* ====================================================================== */
   attrstat * nfsproc_write_2( writeargs * argp, struct svc_req * rqstp)
/* ====================================================================== */
{
    static attrstat res;
    clnt_param  *cp;
    mdFhandle *m = (mdFhandle *)&argp->file ;
    md_auth    auth ;
    
    clear_result( res ) ;
    if( ( cp = knownclient(rqstp) ) == NULL){
        res.status = NFSERR_ACCES;
        return (&res);
    }
    
    make_auth( &auth , cp , rqstp)  ;
    md2pPrintfS(md2pMODINFO,"%s - write ", stringHeader(&auth) );
    res.status = fh_write_data( argp , &res , &auth ) ;
    md2pPrintfE(md2pMODINFO," -> %d\n",res.status );
   
    return (res.status==NFSERR_NOANSWER?NULL:&res);
}

/* ====================================================================== */
   diropres * nfsproc_create_2( createargs * argp,struct svc_req * rqstp)
/* ====================================================================== */
{
    static diropres res;
    clnt_param  *cp;
    md_auth      auth ;
    clear_result( res ) ;
    if( ( cp = knownclient(rqstp) ) == NULL){
        res.status = NFSERR_ACCES;
        return (&res);
    }

    make_auth( &auth , cp , rqstp)  ;
    md2pPrintfS(md2pMODINFO,"%s - create ", stringHeader(&auth) );
    argp->attributes.uid = credit_uid(rqstp);
    res.status = fh_createFile( argp , &res , &auth ) ;
    md2pPrintfE(md2pMODINFO," -> %d\n",res.status );

    return (res.status==NFSERR_NOANSWER?NULL:&res);
}
/* ====================================================================== */
   nfsstat * nfsproc_remove_2( diropargs * argp, struct svc_req * rqstp)
/* ====================================================================== */
{
    static nfsstat res;
    clnt_param  *cp;
    md_auth      auth ;
    
    clear_result( res ) ;
    if( ( cp = knownclient(rqstp) ) == NULL){
        res = NFSERR_ACCES;
        return (&res);
    }

    make_auth( &auth , cp , rqstp)  ;
    md2pPrintfS(md2pMODINFO,"%s - remove ", stringHeader(&auth) );
    res = fh_rmfile( argp , &auth ) ;
    md2pPrintfE(md2pMODINFO," -> %d\n",res );
    return (res==NFSERR_NOANSWER?NULL:&res);
}
/* ====================================================================== */
   nfsstat * nfsproc_rename_2( renameargs * argp, struct svc_req * rqstp)
/* ====================================================================== */
{
    static nfsstat res;
    clnt_param  *cp;
    md_auth      auth ;
    clear_result( res ) ;
    if( ( cp = knownclient(rqstp) ) == NULL){
        res = NFSERR_ACCES;
        return (&res);
    }
    make_auth( &auth , cp , rqstp)  ;
    md2pPrintfS(md2pMODINFO,"%s - rename ", stringHeader(&auth) );
    res = fh_rename( argp , &auth );
    md2pPrintfE(md2pMODINFO," -> %d\n",res );
    return (res==NFSERR_NOANSWER?NULL:&res);
}
/* ====================================================================== */
   nfsstat * nfsproc_link_2( linkargs * argp, struct svc_req * rqstp)
/* ====================================================================== */
{
    static nfsstat res;
    clnt_param  *cp;
    mdFhandle   *dir = (mdFhandle *)&argp->to.dir ;
    char        *name= argp -> to.name ;
    mdFhandle   *dest= (mdFhandle *)&argp->from ;
    md_auth      auth ;
	
    clear_result( res ) ;
    if( ( cp = knownclient(rqstp) ) == NULL){
        res = NFSERR_ACCES;
        return (&res);
    }

    make_auth( &auth , cp , rqstp)  ;
    md2pPrintfS(md2pMODINFO,"%s - link ", stringHeader(&auth) );
#ifdef NO_HARD_LINKS
    res = NFSERR_PERM ;
#else   
    res = use_hardlinks ? fh_addToDirectory(argp ,&auth ) :NFSERR_PERM ;
#endif
    md2pPrintfE(md2pMODINFO," -> %d\n",res );
    return (res==NFSERR_NOANSWER?NULL:&res);
}
/* ====================================================================== */
   nfsstat * nfsproc_symlink_2( symlinkargs * argp, struct svc_req * rqstp)
/* ====================================================================== */
{
    static nfsstat res;
    clnt_param  *cp;
    md_auth      auth ;
    char        *to_path       = argp -> to ;
    
    clear_result( res ) ;
    if( ( cp = knownclient(rqstp) ) == NULL){
        res = NFSERR_ACCES;
        return (&res);
    }

    make_auth( &auth , cp , rqstp)  ;
    md2pPrintfS(md2pMODINFO,"%s - symlink ", stringHeader(&auth) );
    if( ! strncmp( to_path , "/.-./" , 5 ) ){
       /*res = fh_nfs_commands( from_handle , from_name , to_path ) ;*/
    }else{
       res = fh_symlink( argp , &auth ) ;
    }
    md2pPrintfE(md2pMODINFO," -> %d\n",res );
    return (res==NFSERR_NOANSWER?NULL:&res);
}
/* ====================================================================== */
   diropres * nfsproc_mkdir_2( createargs * argp, struct svc_req * rqstp)
/* ====================================================================== */
{
    static diropres res;
    clnt_param  *cp;
    md_auth      auth ;
    clear_result( res ) ;
    if( ( cp = knownclient(rqstp) ) == NULL){
        res.status = NFSERR_ACCES;
        return (&res);
    }
    
    make_auth( &auth , cp , rqstp)  ;
    md2pPrintfS(md2pMODINFO,"%s - mkdir ", stringHeader(&auth) );
    argp->attributes.uid = credit_uid(rqstp);
    res.status = fh_mkdir( argp , &res , &auth ) ;
    md2pPrintfE(md2pMODINFO," -> %d\n",res.status );
    
    return (res.status==NFSERR_NOANSWER?NULL:&res);
}

/* ====================================================================== */
   nfsstat * nfsproc_rmdir_2( diropargs * argp, struct svc_req * rqstp)
/* ====================================================================== */
{
    static nfsstat res;
    clnt_param  *cp;
    md_auth      auth ;
    clear_result( res ) ;
    if( ( cp = knownclient(rqstp) ) == NULL){
        res = NFSERR_ACCES;
        return (&res);
    }
    
    make_auth( &auth , cp , rqstp)  ;
    md2pPrintfS(md2pMODINFO,"%s - rmdir ", stringHeader(&auth) );
    res = fh_rmdir( argp , &auth ) ;
    md2pPrintfE(md2pMODINFO," -> %d\n",res );
    return (res==NFSERR_NOANSWER?NULL:&res);
}
/* ====================================================================== */
   static int dpsize( md_dir_item * item)
/* ====================================================================== */
{
#define DP_SLOP 16
#define MAX_E_SIZE sizeof(entry) + MAXNAMLEN + DP_SLOP
    return sizeof(entry) + strlen(item->name) + DP_SLOP;
}
/* ====================================================================== */
   readdirres * nfsproc_readdir_2( readdirargs * argp, struct svc_req * rqstp)
/* ====================================================================== */
{
    static readdirres res;
    entry        **e;
    clnt_param    *cp;
    long           dloc;
    mdFhandle     *dirp;
    int            res_size = 0 , rc ;
    md_dir_item    item ;
    md_auth        auth ;
    /*
     * Free previous result
     */
    xdr_free((xdrproc_t)xdr_readdirres, (char*)&res);

    clear_result( res ) ;
    if( ( cp = knownclient(rqstp) ) == NULL){
        res.status = NFSERR_ACCES;
        return (&res);
    }
    make_auth( &auth , cp , rqstp)  ;
    md2pPrintfS(md2pINFO,"%s - readdir ", stringHeader(&auth) );
    memcpy( (char*)&dloc ,  argp->cookie , sizeof(dloc) );
    md2pPrintf(md2pINFO,"%s %x", mdStringFhandle( *((mdFhandle*)&(argp->dir)) ),dloc );
    if ( rc = fh_opendir( &(argp->dir) , &dloc , &auth)  ){
#ifdef v3_1_2_b1
        res.status = NFSERR_NAMETOOLONG;
#else
        res.status = rc ;
#endif
    }else{

        res.status = NFS_OK;
                
        e = &(res.readdirres_u.reply.entries);
            
        while( ( ( res_size + MAX_E_SIZE) < argp->count   ||
                  e == &(res.readdirres_u.reply.entries     )   ) &&
                  ! ( rc = fh_readdir( &dloc , &item ) )             ) {                   
		        if( ! fh_canShow( &(argp->dir) , &item , &auth) )
				    continue ;
					
                if ((*e = (entry *) malloc(sizeof(entry))) == NULL)
                    mallocfailed();
		(*e)->fileid = mdInodeID( item.ID ) ;		
                if( (  (*e)->name = malloc(strlen( item.name ) + 1) ) == NULL)
                    mallocfailed();
					
                strcpy( (*e)->name, item.name );
                md2pPrintf(md2pINFO," %s(%x) ",item.name ,(*e)->fileid );
				
		memcpy( (*e)->cookie , &dloc , sizeof(nfscookie));

                e = &((*e)->nextentry);
                res_size += dpsize(  &item );
         }
         *e = NULL;
         res.readdirres_u.reply.eof = (rc != 0);
         fh_closedir();
         res.status = rc < 0 ? rc : NFS_OK ;
    }
    md2pPrintfE(md2pINFO," -> %d\n",res.status );
    return (res.status==NFSERR_NOANSWER?NULL:&res);
}
/* ====================================================================== */
   statfsres * nfsproc_statfs_2( nfs_fh * argp, struct svc_req * rqstp)
/* ====================================================================== */
{
    static statfsres res;
    clnt_param  *cp;
    md_auth        auth ;
    clear_result( res ) ;
    if( ( cp = knownclient(rqstp) ) == NULL){
        res.status = NFSERR_ACCES;
        return (&res);
    }
    make_auth( &auth , cp , rqstp)  ;
    md2pPrintfS(md2pINFO,"%s - statfs ", stringHeader(&auth) );
    /* no easy way to do this */
    res.status = NFS_OK;
    res.statfsres_u.reply.tsize = 4096;
    res.statfsres_u.reply.bsize = 4096;
    res.statfsres_u.reply.blocks = 100000;
    res.statfsres_u.reply.bfree = 80000;
    res.statfsres_u.reply.bavail = 71000;
    
    md2pPrintfE(md2pINFO," -> %d\n",res.status );
    return (res.status==NFSERR_NOANSWER?NULL:&res);
}
