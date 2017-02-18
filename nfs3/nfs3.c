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
/*
 * $Id: nfs3.c,v 1.8 2005-10-27 14:00:30 tigran Exp $
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include "nfs3_prot.h"
#include "md2log.h"
#include "fh3.h"
#include "pnfs3d.h"

#define knownclient(x)      ((clnt_param*)(x))
#define  clear_result(r)   memset((char*)&r,0,sizeof(r))

#define md2pPrintfS  md2pPrintf
#define md2pPrintfE  md2pPrintf


/* ====================================================================== */

extern int use_hardlinks;

#ifdef linux
void *nfsproc3_null_3_svc(void *args, struct svc_req *svc)
#else
void *nfsproc3_null_3(void *args, struct svc_req *svc)
#endif
{
    static void *result = NULL;
    return &result;
}

#ifdef linux
GETATTR3res *nfsproc3_getattr_3_svc(GETATTR3args * args,
									struct svc_req * svc)
#else
GETATTR3res *nfsproc3_getattr_3(GETATTR3args * args, struct svc_req * svc)
#endif
{
	static GETATTR3res res;
	clnt_param *cp;
	md_auth auth;

	clear_result(res);

	if ((cp = knownclient(svc)) == NULL) {
		fprintf(stderr, " error in knownclients\n");
		res.status = NFS3ERR_ACCES;
		return &res;
	}
	make_auth(&auth, cp, svc);
	md2pPrintfS(md2pMOREINFO, "%s - getattr ", stringHeader(&auth));

	res.status = fh3_getattr(&(args->object),
							 &(res.GETATTR3res_u.resok.obj_attributes), &auth);

	md2pPrintfE(md2pMOREINFO, " -> %d\n", res.status);

	return (res.status == NFSERR_NOANSWER ? NULL : &res);
}


#ifdef linux
SETATTR3res *nfsproc3_setattr_3_svc(SETATTR3args * args,
									struct svc_req * svc)
#else
SETATTR3res *nfsproc3_setattr_3(SETATTR3args * args, struct svc_req * svc)
#endif
{
	static SETATTR3res res;

	clnt_param *cp;
	md_auth auth;

	clear_result(res);
	if ((cp = knownclient(svc)) == NULL) {
		res.status = NFS3ERR_ACCES;
		return (&res);
	}

	make_auth(&auth, cp, svc);

	md2pPrintfS(md2pMODINFO, "%s - setattr ", stringHeader(&auth));
	res.status = fh3_setattr(args, &res, &auth);
	md2pPrintfE(md2pMODINFO, " -> %d\n", res.status);
	return &res;


}

#ifdef linux
LOOKUP3res *nfsproc3_lookup_3_svc(LOOKUP3args * args, struct svc_req * svc)
#else
LOOKUP3res *nfsproc3_lookup_3(LOOKUP3args * args, struct svc_req * svc)
#endif
{
	static LOOKUP3res res;
	static mdFhandle resHandle;
	clnt_param *cp;
	md_auth auth;

	clear_result(res);
	if ((cp = knownclient(svc)) == NULL) {
		res.status = NFS3ERR_ACCES;
		return (&res);
	}
	make_auth(&auth, cp, svc);

	res.LOOKUP3res_u.resok.dir_attributes.attributes_follow = 1;
	res.LOOKUP3res_u.resok.obj_attributes.attributes_follow = 1;

	res.LOOKUP3res_u.resok.object.data.data_len = sizeof(resHandle);
	res.LOOKUP3res_u.resok.object.data.data_val = (void *) &resHandle;


	res.status = fh3_lookup(&(args->what),
							&(res.LOOKUP3res_u.resok.object),
							&(res.LOOKUP3res_u.resok.obj_attributes.
							  post_op_attr_u.attributes),
							&(res.LOOKUP3res_u.resok.dir_attributes.
							  post_op_attr_u.attributes), &auth);

	if (res.status != NFS3_OK) {
		res.LOOKUP3res_u.resok.dir_attributes.attributes_follow = 0;
		res.LOOKUP3res_u.resok.obj_attributes.attributes_follow = 0;

		res.LOOKUP3res_u.resok.object.data.data_len = 0;
		res.LOOKUP3res_u.resok.object.data.data_val = NULL;
	}

	return &res;
}

void fprint_access(FILE * stream, uint32 access)
{
	fprintf(stream, (access & ACCESS3_READ) == ACCESS3_READ ? "R" : "-");
	fprintf(stream,
			(access & ACCESS3_LOOKUP) == ACCESS3_LOOKUP ? "L" : "-");
	fprintf(stream,
			(access & ACCESS3_MODIFY) == ACCESS3_MODIFY ? "M" : "-");
	fprintf(stream,
			(access & ACCESS3_EXTEND) == ACCESS3_EXTEND ? "E" : "-");
	fprintf(stream,
			(access & ACCESS3_DELETE) == ACCESS3_DELETE ? "D" : "-");
	fprintf(stream,
			(access & ACCESS3_EXECUTE) == ACCESS3_EXECUTE ? "X" : "-");
}

#ifdef linux
ACCESS3res *nfsproc3_access_3_svc(ACCESS3args * args, struct svc_req *svc)
#else
ACCESS3res *nfsproc3_access_3(ACCESS3args * args, struct svc_req *svc)
#endif
{

	static ACCESS3res res;
	uint32 access = args->access;
	clnt_param *cp;
	md_auth auth;
	fattr3 *attr =
		&(res.ACCESS3res_u.resok.obj_attributes.post_op_attr_u.attributes);

	clear_result(res);
	if ((cp = knownclient(svc)) == NULL) {
		res.status = NFS3ERR_ACCES;
		return (&res);
	}
	make_auth(&auth, cp, svc);

	res.ACCESS3res_u.resok.obj_attributes.attributes_follow = 1;
	res.status = fh3_getattr(&(args->object), attr, &auth);

	if (res.status != NFS3_OK) {
		return &res;
	}

	res.ACCESS3res_u.resok.access = 0;
	/*
	   ACCESS3_READ   | ACCESS3_LOOKUP | ACCESS3_MODIFY |
	   ACCESS3_EXTEND | ACCESS3_DELETE | ACCESS3_EXECUTE ;
	 */
	if ((access & ACCESS3_READ) &&
		(((attr->mode & FH3_READ_OWNER) && (auth.uid == attr->uid)) ||
		 ((attr->mode & FH3_READ_GROUP) && (auth.gid == attr->gid)) ||
		 (attr->mode & FH3_READ_OTHER)
		)
		) {

		res.ACCESS3res_u.resok.access |= ACCESS3_READ;
	}

	if ((access & (ACCESS3_EXECUTE | ACCESS3_LOOKUP)) &&
		(((attr->mode & FH3_EXEC_OWNER) && (auth.uid == attr->uid)) ||
		 ((attr->mode & FH3_EXEC_GROUP) && (auth.gid == attr->gid)) ||
		 (attr->mode & FH3_EXEC_OTHER)
		)
		) {

		res.ACCESS3res_u.resok.access |= ACCESS3_LOOKUP | ACCESS3_EXECUTE;
	}

	if ((access & (ACCESS3_DELETE | ACCESS3_MODIFY | ACCESS3_EXTEND)) &&
		(((attr->mode & FH3_WRITE_OWNER) && (auth.uid == attr->uid)) ||
		 ((attr->mode & FH3_WRITE_GROUP) && (auth.gid == attr->gid)) ||
		 (attr->mode & FH3_WRITE_OTHER)
		)
		) {

		res.ACCESS3res_u.resok.access |=
			(ACCESS3_DELETE | ACCESS3_MODIFY | ACCESS3_EXTEND);
	}

	/* root can do every thing */
	if (auth.uid == 0) {
		access |= ACCESS3_READ | ACCESS3_MODIFY | ACCESS3_EXTEND;
	}


	if (attr->type == NF3DIR) {

		if (access & ACCESS3_READ) {
			res.ACCESS3res_u.resok.access |= ACCESS3_LOOKUP;
		}

		if (access & ACCESS3_MODIFY) {
			res.ACCESS3res_u.resok.access |= ACCESS3_DELETE;
		}
		res.ACCESS3res_u.resok.access &= ~ACCESS3_EXECUTE;
	}

	res.ACCESS3res_u.resok.access &= access;

	return &res;
}

#ifdef linux
READLINK3res *nfsproc3_readlink_3_svc(READLINK3args * args,
									  struct svc_req * svc)
#else
READLINK3res *nfsproc3_readlink_3(READLINK3args * args,
								  struct svc_req * svc)
#endif
{
	static READLINK3res res;
	clnt_param *cp;
	md_auth auth;

	clear_result(res);
	if ((cp = knownclient(svc)) == NULL) {
		res.status = NFS3ERR_ACCES;
		return (&res);
	}

	make_auth(&auth, cp, svc);
	md2pPrintfS(md2pINFO, "%s - readlink ", stringHeader(&auth));
	res.status = fh3_readlink(args, &res, &auth);
	md2pPrintfE(md2pINFO, " -> %d\n", res.status);

	return &res;
}


#ifdef linux
READ3res *nfsproc3_read_3_svc(READ3args * args, struct svc_req * rqstp)
#else
READ3res *nfsproc3_read_3(READ3args * args, struct svc_req * rqstp)
#endif
{
	static READ3res res;
	clnt_param *cp;
	md_auth auth;

	clear_result(res);
	if ((cp = knownclient(rqstp)) == NULL) {
		res.status = NFS3ERR_ACCES;
		return (&res);
	}

	make_auth(&auth, cp, rqstp);

	md2pPrintfS(md2pINFO, "%s - read ", stringHeader(&auth));
	res.status = fh3_read_data(args, &res, &auth);
	md2pPrintfE(md2pINFO, " -> %d\n", res.status);

	return &res;

}

#ifdef linux
WRITE3res *nfsproc3_write_3_svc(WRITE3args * args, struct svc_req * svc)
#else
WRITE3res *nfsproc3_write_3(WRITE3args * args, struct svc_req * svc)
#endif
{
	static WRITE3res res;
	clnt_param *cp;
	md_auth auth;

	clear_result(res);
	if ((cp = knownclient(svc)) == NULL) {
		res.status = NFS3ERR_ACCES;
		return (&res);
	}

	make_auth(&auth, cp, svc);
	md2pPrintfS(md2pMODINFO, "%s - write ", stringHeader(&auth));
	res.status = fh3_write_data(args, &res, &auth);
	md2pPrintfE(md2pMODINFO, " -> %d\n", res.status);

	return &res;
}


#ifdef linux
CREATE3res *nfsproc3_create_3_svc(CREATE3args * args, struct svc_req * svc)
#else
CREATE3res *nfsproc3_create_3(CREATE3args * args, struct svc_req * svc)
#endif
{
	static CREATE3res res;
	clnt_param *cp;
	md_auth auth;


	if ((cp = knownclient(svc)) == NULL) {
		fprintf(stderr, " error in knownclients\n");
		res.status = NFS3ERR_ACCES;
		return &res;
	}
	make_auth(&auth, cp, svc);

	md2pPrintfS(md2pMODINFO, "%s - create ", stringHeader(&auth));
	args->how.createhow3_u.obj_attributes.uid.set_uid3_u.uid =
		credit_uid(svc);
	res.status = fh3_createFile(args, &res, &auth);
	md2pPrintfE(md2pMODINFO, " -> %d\n", res.status);
	fprint_nfs_fh3(stderr,
				   &(res.CREATE3res_u.resok.obj.post_op_fh3_u.handle));
	return &res;

}

#ifdef linux
MKDIR3res *nfsproc3_mkdir_3_svc(MKDIR3args * args, struct svc_req * rqstp)
#else
MKDIR3res *nfsproc3_mkdir_3(MKDIR3args * args, struct svc_req * rqstp)
#endif
{

	static MKDIR3res res;
	clnt_param *cp;
	md_auth auth;

	clear_result(res);
	if ((cp = knownclient(rqstp)) == NULL) {
		res.status = NFS3ERR_ACCES;
		return (&res);
	}

	make_auth(&auth, cp, rqstp);
	md2pPrintfS(md2pMODINFO, "%s - mkdir ", stringHeader(&auth));
	args->attributes.uid.set_uid3_u.uid = credit_uid(rqstp);
	res.status = fh3_mkdir(args, &res, &auth);
	md2pPrintfE(md2pMODINFO, " -> %d\n", res.status);

	return &res;
}

#ifdef linux
SYMLINK3res *nfsproc3_symlink_3_svc(SYMLINK3args * args,
									struct svc_req * svc)
#else
SYMLINK3res *nfsproc3_symlink_3(SYMLINK3args * args, struct svc_req * svc)
#endif
{

	static SYMLINK3res res;
	md_auth auth;
	clnt_param *cp;


	clear_result(res);
	if ((cp = knownclient(svc)) == NULL) {
		res.status = NFS3ERR_ACCES;
		return (&res);
	}

	make_auth(&auth, cp, svc);
	md2pPrintfS(md2pMODINFO, "%s - symlink ", stringHeader(&auth));
	res.status = fh3_symlink(args, &auth);
	md2pPrintfE(md2pMODINFO, " -> %d\n", res.status);

	return &res;
}

#ifdef linux
MKNOD3res *nfsproc3_mknod_3_svc(MKNOD3args * args, struct svc_req * svc)
#else
MKNOD3res *nfsproc3_mknod_3(MKNOD3args * args, struct svc_req * svc)
#endif
{
	static MKNOD3res res;

	clear_result(res);
	res.status = NFS3ERR_NOTSUPP;

	return &res;
}


#ifdef linux
REMOVE3res *nfsproc3_remove_3_svc(REMOVE3args * args, struct svc_req * svc)
#else
REMOVE3res *nfsproc3_remove_3(REMOVE3args * args, struct svc_req * svc)
#endif
{
	static REMOVE3res res;
	clnt_param *cp;
	md_auth auth;

	clear_result(res);
	if ((cp = knownclient(svc)) == NULL) {
		res.status = NFS3ERR_ACCES;
		return (&res);
	}

	make_auth(&auth, cp, svc);
	md2pPrintfS(md2pMODINFO, "%s - remove ", stringHeader(&auth));
	res.status = fh3_rmfile(args, &auth);
	md2pPrintfE(md2pMODINFO, " -> %d\n", res.status);
	return &res;
}

#ifdef linux
RMDIR3res *nfsproc3_rmdir_3_svc(RMDIR3args * args, struct svc_req * svc)
#else
RMDIR3res *nfsproc3_rmdir_3(RMDIR3args * args, struct svc_req * svc)
#endif
{
	static RMDIR3res res;
	clnt_param *cp;

	md_auth auth;
	clear_result(res);
	if ((cp = knownclient(svc)) == NULL) {
		res.status = NFS3ERR_ACCES;
		return (&res);
	}

	make_auth(&auth, cp, svc);
	md2pPrintfS(md2pMODINFO, "%s - rmdir ", stringHeader(&auth));
	res.status = fh3_rmdir(args, &auth);
	md2pPrintfE(md2pMODINFO, " -> %d\n", res.status);

	return (void *) &res;
}

#ifdef linux
RENAME3res *nfsproc3_rename_3_svc(RENAME3args * args, struct svc_req *svc)
#else
RENAME3res *nfsproc3_rename_3(RENAME3args * args, struct svc_req *svc)
#endif
{
	static RENAME3res res;
	clnt_param *cp;

	md_auth auth;
	clear_result(res);
	if ((cp = knownclient(svc)) == NULL) {
		res.status = NFS3ERR_ACCES;
		return (&res);
	}
	make_auth(&auth, cp, svc);
	md2pPrintfS(md2pMODINFO, "%s - rename ", stringHeader(&auth));
	res.status = fh3_rename(args, &auth);
	md2pPrintfE(md2pMODINFO, " -> %d\n", res.status);

	return &res;

}

#ifdef linux
LINK3res *nfsproc3_link_3_svc(LINK3args * args, struct svc_req * svc)
#else
LINK3res *nfsproc3_link_3(LINK3args * args, struct svc_req * svc)
#endif
{
	static LINK3res res;
	clnt_param *cp;
	md_auth auth;

	clear_result(res);
	if ((cp = knownclient(svc)) == NULL) {
		res.status = NFS3ERR_ACCES;
		return (&res);
	}
	make_auth(&auth, cp, svc);

	md2pPrintfS(md2pMODINFO, "%s - link ", stringHeader(&auth));

#ifdef NO_HARD_LINKS
	res.status = NFS3ERR_PERM;
#else
	res.status =
		use_hardlinks ? fh3_addToDirectory(args, &auth) : NFS3ERR_PERM;
#endif
	md2pPrintfE(md2pMODINFO, " -> %d\n", res.status);


	return &res;
}

#define MAX_ENTRY_SPACE 128 

#ifdef linux
READDIR3res *nfsproc3_readdir_3_svc(READDIR3args * args,
									struct svc_req * svc)
#else
READDIR3res *nfsproc3_readdir_3(READDIR3args * args, struct svc_req * svc)
#endif
{
	static READDIR3res res;
	int string_space = 0, space_used = 0, rc;
	uint32 packet_number = 0;
	entry3 *e;
	entry3 *previous_entry = NULL;
	clnt_param *cp;
	md_auth auth;
	md_dir_item item;
	long dloc = 0;


	/*
	 * Free previous result
	 */
	xdr_free((xdrproc_t) xdr_READDIR3res, (char *) &res);

	clear_result(res);
	if ((cp = knownclient(svc)) == NULL) {
		res.status = NFS3ERR_ACCES;
		md2pPrintfE(md2pINFO, " -> %d\n", res.status);
		return (&res);
	}
	make_auth(&auth, cp, svc);

	md2pPrintfS(md2pINFO, "%s - readdir ", stringHeader(&auth));
	md2pPrintf(md2pINFO, "%s %x",
			   mdStringFhandle(*((mdFhandle *) & (args->dir))),
			   args->cookie);

	res.READDIR3res_u.resok.dir_attributes.attributes_follow = 1;
	res.status = fh3_getattr(&(args->dir),
							 &(res.READDIR3res_u.resok.dir_attributes.
							   post_op_attr_u.attributes), &auth);

	if (res.status != NFS3_OK) {
		return &res;
	}

	memcpy((char *) &dloc, &(args->cookie), sizeof(dloc));

	rc = fh3_opendir(&(args->dir), &(dloc), &auth);
	if (rc) {
		res.status = rc;
		md2pPrintfE(md2pINFO, " -> %d\n", res.status);
		return &res;
	}

	/*
	 * increment the cookie(verf)
	 */
	memcpy((void *) &packet_number, (void *) (args->cookieverf),
		   sizeof(packet_number));
	packet_number++;
	memcpy((void *) (args->cookieverf), (void *) &packet_number,
		   sizeof(packet_number));

	while (((space_used + MAX_ENTRY_SPACE) < args->count) &&
		   (!(rc = fh3_readdir(&(dloc), &item)))) {

		/*
		 *  prepare the entry
		 */
		if ((e = (entry3 *) malloc(sizeof(entry3))) == NULL) {
			res.status = NFS3ERR_SERVERFAULT;
			res.READDIR3res_u.resfail.dir_attributes.attributes_follow = 0;
			md2pPrintfE(md2pINFO, " -> %d\n", res.status);
			return &res;
		}
		space_used += sizeof(entry3);
		memset((void *) e, sizeof(entry3), 0);

		string_space = strlen(item.name) + 1;
		space_used += string_space;

		if ((e->name = (char *) malloc(string_space)) == NULL) {
			res.status = NFS3ERR_SERVERFAULT;
			res.READDIR3res_u.resfail.dir_attributes.attributes_follow = 0;
			md2pPrintfE(md2pINFO, " -> %d\n", res.status);
			return &res;
		}
		/*
		 * set the entry3 ...
		 */
		memcpy(e->name, item.name, string_space);
		e->fileid = mdInodeID(item.ID);
		e->nextentry = NULL;
		memcpy(&(e->cookie), &dloc, sizeof(dloc));

		if (previous_entry != NULL) {
			previous_entry->nextentry = e;
		} else {
			res.READDIR3res_u.resok.reply.entries = e;
		}
		previous_entry = e;

	}
	res.READDIR3res_u.resok.reply.eof = rc != 0;
	fh3_closedir();
	res.status = rc < 0 ? rc : NFS3_OK;

	md2pPrintfE(md2pINFO, " -> %d\n", res.status);
	return (res.status == NFSERR_NOANSWER ? NULL : &res);

}

#ifdef linux
READDIRPLUS3res *nfsproc3_readdirplus_3_svc(READDIRPLUS3args * args,
											struct svc_req * svc)
#else
READDIRPLUS3res *nfsproc3_readdirplus_3(READDIRPLUS3args * args,
										struct svc_req * svc)
#endif
{
	static READDIRPLUS3res result;

	/* NOT SUPPORTED */

	result.status = NFS3ERR_NOTSUPP;
	result.READDIRPLUS3res_u.resfail.dir_attributes.attributes_follow =
		FALSE;

	return &result;
}

#ifdef linux
FSSTAT3res *nfsproc3_fsstat_3_svc(FSSTAT3args * args, struct svc_req * svc)
#else
FSSTAT3res *nfsproc3_fsstat_3(FSSTAT3args * args, struct svc_req * svc)
#endif
{
	static FSSTAT3res res;
	clnt_param *cp;
	md_auth auth;

	if ((cp = knownclient(svc)) == NULL) {
		fprintf(stderr, " error in knownclients\n");
		res.status = NFS3ERR_ACCES;
		return &res;
	}
	make_auth(&auth, cp, svc);
	md2pPrintfS(md2pMOREINFO, "%s - fsstat ", stringHeader(&auth));

	res.FSSTAT3res_u.resok.obj_attributes.attributes_follow = 1;
	res.status = fh3_getattr(&(args->fsroot),
							 &(res.FSSTAT3res_u.resok.obj_attributes.
							   post_op_attr_u.attributes), &auth);

	md2pPrintfE(md2pMOREINFO, " -> %d\n", res.status);
	if (res.status != NFS3_OK)
		return &res;


	res.FSSTAT3res_u.resok.tbytes = 100000000L;
	res.FSSTAT3res_u.resok.fbytes = 50000000L;
	res.FSSTAT3res_u.resok.abytes = 20000000L;
	res.FSSTAT3res_u.resok.tfiles = 1000;
	res.FSSTAT3res_u.resok.ffiles = 500;
	res.FSSTAT3res_u.resok.afiles = 500;
	res.FSSTAT3res_u.resok.invarsec = 0;

	return &res;
}

#ifdef linux
FSINFO3res *nfsproc3_fsinfo_3_svc(FSINFO3args * args, struct svc_req * svc)
#else
FSINFO3res *nfsproc3_fsinfo_3(FSINFO3args * args, struct svc_req * svc)
#endif
{
	static FSINFO3res res;
	clnt_param *cp;
	md_auth auth;

	if ((cp = knownclient(svc)) == NULL) {
		fprintf(stderr, " error in knownclients\n");
		res.status = NFS3ERR_ACCES;
		return &res;
	}
	make_auth(&auth, cp, svc);
	md2pPrintfS(md2pMOREINFO, "%s - fsinfo ", stringHeader(&auth));

	res.FSINFO3res_u.resok.obj_attributes.attributes_follow = 1;
	res.status = fh3_getattr(&(args->fsroot),
							 &(res.FSINFO3res_u.resok.obj_attributes.
							   post_op_attr_u.attributes), &auth);

	md2pPrintfE(md2pMOREINFO, " -> %d\n", res.status);

	if (res.status != NFS3_OK)
		return &res;

	res.status = NFS3_OK;
	res.FSINFO3res_u.resok.rtmax = 8192;
	res.FSINFO3res_u.resok.rtpref = 8192;
	res.FSINFO3res_u.resok.rtmult = 4096;
	res.FSINFO3res_u.resok.wtmax = 8192;
	res.FSINFO3res_u.resok.wtpref = 8192;
	res.FSINFO3res_u.resok.wtmult = 4096;
	res.FSINFO3res_u.resok.dtpref = 4096;
	res.FSINFO3res_u.resok.maxfilesize = ~0ULL;
	res.FSINFO3res_u.resok.time_delta.seconds = 1;
	res.FSINFO3res_u.resok.time_delta.nseconds = 0;
	res.FSINFO3res_u.resok.properties =
		FSF3_LINK | FSF3_SYMLINK | FSF3_HOMOGENEOUS | FSF3_CANSETTIME;

	return &res;

}

#ifdef linux
PATHCONF3res *nfsproc3_pathconf_3_svc(PATHCONF3args * args,
									  struct svc_req * svc)
#else
PATHCONF3res *nfsproc3_pathconf_3(PATHCONF3args * args,
								  struct svc_req * svc)
#endif
{
	static PATHCONF3res res;
	clnt_param *cp;
	md_auth auth;

	if ((cp = knownclient(svc)) == NULL) {
		fprintf(stderr, " error in knownclients\n");
		res.status = NFS3ERR_ACCES;
		return &res;
	}

	make_auth(&auth, cp, svc);

	md2pPrintfS(md2pMOREINFO, "%s - pathconf ", stringHeader(&auth));

	res.status = fh3_getattr(&(args->object),
							 &(res.PATHCONF3res_u.resok.obj_attributes.
							   post_op_attr_u.attributes), &auth);

	md2pPrintfE(md2pMOREINFO, " -> %d\n", res.status);

	if (res.status != NFS3_OK)
		return &res;


	res.status = NFS3_OK;
	res.PATHCONF3res_u.resok.linkmax = 0xFFFFFFFF;
	res.PATHCONF3res_u.resok.name_max = NFS_MAXPATHLEN;
	res.PATHCONF3res_u.resok.no_trunc = TRUE;
	res.PATHCONF3res_u.resok.chown_restricted = FALSE;
	res.PATHCONF3res_u.resok.case_insensitive = FALSE;
	res.PATHCONF3res_u.resok.case_preserving = TRUE;

	return &res;

}


#ifdef linux
COMMIT3res *nfsproc3_commit_3_svc(COMMIT3args * args, struct svc_req * svc)
#else
COMMIT3res *nfsproc3_commit_3(COMMIT3args * args, struct svc_req * svc)
#endif
{
	static COMMIT3res res;

	clnt_param *cp;
	md_auth auth;

	clear_result(res);

	if ((cp = knownclient(svc)) == NULL) {
		res.status = NFS3ERR_ACCES;
		return (&res);
	}

	make_auth(&auth, cp, svc);
	res.status = fh3_commit(args, &res, &auth);

	return &res;

}
