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
#include "mount3_prot.h"
#include "md2log.h"
#include "fh3.h"

#ifdef linux
void *mountproc3_null_3_svc(void *x, struct svc_req *svc)
#else
void *mountproc3_null_3(void *x, struct svc_req *svc)
#endif
{
    static void *result = NULL;
    return &result;
}

#ifdef linux
mountres3 *mountproc3_mnt_3_svc(dirpath * dp, struct svc_req *svc)
#else
mountres3 *mountproc3_mnt_3(dirpath * dp, struct svc_req *svc)
#endif
{

	static mountres3 res;
	md_auth auth;
	static int uauth = AUTH_UNIX;

	make_auth(&auth, NULL, svc);

	memset((void *) &res, 0, sizeof(res));

	res.mountres3_u.mountinfo.auth_flavors.auth_flavors_len = 1;
	res.mountres3_u.mountinfo.auth_flavors.auth_flavors_val = &uauth;

	md2pPrintf(md2pMaxLevel | md2pW, "%s - mount ", stringHeader(&auth));

	res.fhs_status = fh3_create(&auth,
								&(res.mountres3_u.mountinfo.fhandle), *dp);

	md2pPrintf(md2pMaxLevel | md2pW, " -> %d\n", res.fhs_status);

	return &res;
}

#ifdef linux
mountlist *mountproc3_dump_3_svc(void *v, struct svc_req * svc)
#else
mountlist *mountproc3_dump_3(void *v, struct svc_req * svc)
#endif
{
    static void *result = NULL;
    return (mountlist *)&result;
}

#ifdef linux
void *mountproc3_umnt_3_svc(dirpath * dp, struct svc_req *svc)
#else
void *mountproc3_umnt_3(dirpath * dp, struct svc_req *svc)
#endif
{
    static void *result = NULL;
    return &result;
}

#ifdef linux
void *mountproc3_umntall_3_svc(void *v, struct svc_req *svc)
#else
void *mountproc3_umntall_3(void *v, struct svc_req *svc)
#endif
{
    static void *result = NULL;
    return &result;
}


#ifdef linux
exports *mountproc3_export_3_svc(void *v, struct svc_req *svc)
#else
exports *mountproc3_export_3(void *v, struct svc_req *svc)
#endif
{
    static void *result = NULL;
    return (exports *)&result;
}
