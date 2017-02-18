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
#include <ctype.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/file.h>
#include <rpc/rpc.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <string.h>
#include <syslog.h>

/* mask SUNOS/BSD4.3 syslog incompatibilities */
#ifndef LOG_DAEMON
#define	LOG_DAEMON	0
#endif /* LOG_DAEMON */
#ifndef LOG_TIME
#define	LOG_TIME	0
#endif /* LOG_TIME */

#ifdef solaris
#include <limits.h>
#define NGROUPS NGROUPS_MAX
#endif


#include "nfs_prot.h"
#include "fh.h"

typedef struct options
{
	/* uid/gid mapping functions */
	enum {map_daemon, identity}	uidmap;
	int	root_squash;
	/* client options */
	int	secure_port;
	int	read_only;
	int	link_relative;
}
	options;

typedef struct clnt_param
{
	struct clnt_param	*next;

	struct in_addr	clnt_addr;
	char	*clnt_name;
	char	*mount_point;
	options	o;
}
	clnt_param;

extern ftype ft_map[16];
extern int svc_euid;
extern int svc_egid;
extern int cur_gid;
extern int svc_ngids;
extern int svc_gids[NGROUPS+2];

#define ft_extr(x)	((x & S_IFMT) >> 12)
#define in_gid_set(gid)	((gid) == cur_gid || _in_gid_set(gid))

extern int _in_gid_set();
extern clnt_param *knownclient();
