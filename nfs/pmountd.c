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

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <rpc/rpc.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <stdio.h>
#include "mount.h"
#include "nfs_prot.h"
#include "fh.h"
#include "md2log.h"

static struct timeval TIMEOUT = { 25, 0 };


int unfsmntd_init( int argc , char *argv[] )
{
#ifndef DEBUG
	int fd;

	if (fork()) exit(0);

	close(0);
	close(1);
	close(2);

#ifdef solaris
        setpgrp() ;
#else
	if ((fd = open("/dev/tty", 2)) >= 0) {
		ioctl(fd, TIOCNOTTY, (char *)0);
		(void) close(fd);
	}
#endif

#endif /* DEBUG */
	fh_init( argc , argv );
	return 0 ;
}

void * mountproc_null_1(  void *argp, struct svc_req *rqstp)
{
	static char res;

	bzero(&res, sizeof(res));
	return ((void *)&res);
}


fhstatus * mountproc_mnt_1(dirpath * argp, struct svc_req *rqstp)
{
	static fhstatus res;
	struct stat stbuf;
	extern int errno;
        md_auth      auth ;

	bzero(&res, sizeof(res));

        res.fhs_status = 0;
		
        make_auth( &auth , NULL , rqstp)  ;
        
        md2pPrintf(md2pMaxLevel|md2pW,"%s - mount ", stringHeader(&auth) );
	res.fhs_status = fh_create( &auth , 
	                            (nfs_fh *)&(res.fhstatus_u.fhs_fhandle),
	                            *argp    );
        md2pPrintf(md2pMaxLevel|md2pW," -> %d\n",res.fhs_status );
	return (&res);
}


mountlist * mountproc_dump_1(void *argp,struct svc_req * rqstp)
{
	static mountlist res;

	bzero(&res, sizeof(res));
	return (&res);
}


void * mountproc_umnt_1(dirpath *argp, struct svc_req *rqstp)
{
        md_auth      auth ;
	static char res;

	bzero(&res, sizeof(res));
        make_auth( &auth , NULL , rqstp)  ;
        md2pPrintf(md2pMaxLevel|md2pW,"%s - umount ", stringHeader(&auth) );
        md2pPrintf(md2pMaxLevel|md2pW," -> %d\n",res );

	return ((void *)&res);
}


void * mountproc_umntall_1( void *argp, struct svc_req *rqstp)
{
	static char res;

	bzero(&res, sizeof(res));
	return ((void *)&res);
}


exports * mountproc_export_1( void * argp, struct svc_req *rqstp)
{
	static exports res;
	static groupnode resgr;

	bzero(&res, sizeof(res));
	res.ex_dir = "/";
	res.ex_groups = &resgr;
	bzero(&resgr, sizeof(resgr));
	resgr.gr_name = "mollusca";
	resgr.gr_next = (groups *) 0;
	res.ex_next = (exports *) 0;
	return (&res);
}


exports * mountproc_exportall_1(void * argp,struct svc_req * rqstp)
{
	return (mountproc_export_1(argp, rqstp));
}
