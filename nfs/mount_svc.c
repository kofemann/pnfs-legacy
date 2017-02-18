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
/*
 * Please do not edit this file.
 * It was generated using rpcgen.
 */

#include <stdio.h>
#include <rpc/rpc.h>
#if defined(solaris) && ! defined(darwin)
#include <rpc/svc_soc.h>
#endif
#include "mount.h"

static void mountprog_1();

main(argc,argv) int argc; char **argv;
{
	register SVCXPRT *transp;

	(void) pmap_unset(MOUNTPROG, MOUNTVERS);

	transp = svcudp_create(RPC_ANYSOCK);
	if (transp == NULL) {
		fprintf(stderr, "cannot create udp service.");
		exit(1);
	}
	if (!svc_register(transp, MOUNTPROG, MOUNTVERS, mountprog_1, IPPROTO_UDP)) {
		fprintf(stderr, "unable to register (MOUNTPROG, MOUNTVERS, udp).");
		exit(1);
	}

	unfsmntd_init(argc,argv); svc_run();
	fprintf(stderr, "svc_run returned");
	exit(1);
	/* NOTREACHED */
}

static void
mountprog_1(rqstp, transp)
	struct svc_req *rqstp;
	register SVCXPRT *transp;
{
	union {
		dirpath mountproc_mnt_1_arg;
		dirpath mountproc_umnt_1_arg;
	} argument;
	char *result;
	bool_t (*xdr_argument)(), (*xdr_result)();
	char *(*local)();
	unsigned short remote_port;
	
	remote_port = ntohs(transp->xp_raddr.sin_port);

	if( (remote_port >= 1024) && ( rqstp->rq_proc != MOUNTPROC_NULL) ) {
		fprintf(stderr, "mount request from %s from unprivileged port", 
			inet_ntoa(transp->xp_raddr.sin_addr) );
		svcerr_weakauth(transp);
   		return;
	}


	switch (rqstp->rq_proc) {
	case MOUNTPROC_NULL:
		xdr_argument = xdr_void;
		xdr_result = xdr_void;
		local = (char *(*)()) mountproc_null_1;
		break;

	case MOUNTPROC_MNT:
		
		xdr_argument = xdr_dirpath;
		xdr_result = xdr_fhstatus;
		local = (char *(*)()) mountproc_mnt_1;
		break;

	case MOUNTPROC_DUMP:
		xdr_argument = xdr_void;
		xdr_result = xdr_mountlist;
		local = (char *(*)()) mountproc_dump_1;
		break;

	case MOUNTPROC_UMNT:
		xdr_argument = xdr_dirpath;
		xdr_result = xdr_void;
		local = (char *(*)()) mountproc_umnt_1;
		break;

	case MOUNTPROC_UMNTALL:
		xdr_argument = xdr_void;
		xdr_result = xdr_void;
		local = (char *(*)()) mountproc_umntall_1;
		break;

	case MOUNTPROC_EXPORT:
		xdr_argument = xdr_void;
		xdr_result = xdr_exports;
		local = (char *(*)()) mountproc_export_1;
		break;

	case MOUNTPROC_EXPORTALL:
		xdr_argument = xdr_void;
		xdr_result = xdr_exports;
		local = (char *(*)()) mountproc_exportall_1;
		break;

	default:
		svcerr_noproc(transp);
		return;
	}
	bzero((char *)&argument, sizeof(argument));
	if (!svc_getargs(transp, xdr_argument, (caddr_t)&argument)) {
		svcerr_decode(transp);
		return;
	}
	result = (*local)(&argument, rqstp);
	if (result != NULL && !svc_sendreply(transp, (xdrproc_t)xdr_result,(caddr_t) result)) {
		svcerr_systemerr(transp);
	}
	if (!svc_freeargs(transp, xdr_argument, (caddr_t)&argument)) {
		fprintf(stderr, "unable to free arguments");
		exit(1);
	}
	return;
}