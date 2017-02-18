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

#include "pnfsd.h"
/*
extern char	*malloc();
extern char	*realloc();
*/

#ifndef SYSERROR
#define SYSERROR	(-1)
#endif

#if defined(solaris) && ! defined(darwin)
#define svc_getcaller(x)  ((struct sockaddr_in*)(svc_getrpccaller(x)->buf))
#endif

int makesock(int port,int socksz)
{
	struct sockaddr_in	my_sock;
	int			s;
	extern int		errno;
        const int on = 1 ;

	memset((char *)&my_sock,0, sizeof(my_sock));
	my_sock.sin_addr.s_addr = INADDR_ANY;
	my_sock.sin_family = AF_INET;
	my_sock.sin_port = htons(port);

	if ((s = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
	{
		syslog(LOG_DAEMON|LOG_ERR, "could not make a socket: %d",
			errno);
		return SYSERROR;
	}
#ifdef SO_REUSEADDR
        setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *) &on, sizeof(on));
#endif
#ifdef SO_SNDBUF
	{
		int	sblen, rblen;

		sblen = rblen = socksz + 1024;
		/* 1024 for rpc & transport overheads */
		if (
		setsockopt(s, SOL_SOCKET, SO_SNDBUF, (char*)&sblen, sizeof sblen) < 0
		||
		setsockopt(s, SOL_SOCKET, SO_RCVBUF, (char *)&rblen, sizeof sblen) < 0
		)
		syslog(LOG_DAEMON|LOG_ERR, "setsockopt failed %d",
			errno);
	}
#endif

	if (bind(s, (struct sockaddr*)&my_sock, sizeof(my_sock)) == -1)
	{
		syslog(LOG_DAEMON|LOG_ERR, "could not bind name to socket%d\n",
			errno);
		return SYSERROR;
	}

	return s;
}

ftype ft_map[16];
unfsd_init( int argc, char	**argv)
{
 int i ;
#ifndef DEBUG
	{
		int fd;

		if (fork())
			exit(0);
		close(0);
		close(1);
		close(2);
#ifdef solaris
                setpgrp() ;
#else
		if ((fd = open("/dev/tty", 2)) >= 0)
		{
			ioctl(fd, TIOCNOTTY, (char *)0);
			(void) close(fd);
		}
#endif
	}
#endif /* DEBUG */

	fh_init( argc , argv );

	ft_map[0] = NFNON;
	for (i = 1; i < 16; i++)
		ft_map[i] = NFBAD;
#ifdef S_IFIFO
	ft_map[ft_extr(S_IFIFO)] = NFFIFO;
#endif
	ft_map[ft_extr(S_IFCHR)] = NFCHR;
	ft_map[ft_extr(S_IFDIR)] = NFDIR;
	ft_map[ft_extr(S_IFBLK)] = NFBLK;
	ft_map[ft_extr(S_IFREG)] = NFREG;
	ft_map[ft_extr(S_IFLNK)] = NFLNK;
	ft_map[ft_extr(S_IFSOCK)] = NFSOCK;

	umask(0);

}

