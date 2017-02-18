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
#include <sys/socket.h>
#include <netinet/in.h>
#include <syslog.h>

#ifndef LOG_DAEMON
#define LOG_DAEMON  0
#endif /* LOG_DAEMON */


int makesock3udp(int port)
{

    struct sockaddr_in sin;
    int sock = 0;
    const int on = 1;

    if (port) {
		sin.sin_family = AF_INET;
		sin.sin_port = htons(port);
		sin.sin_addr.s_addr = INADDR_ANY;
		sock = socket(PF_INET, SOCK_DGRAM, 0);
		setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *) &on, sizeof(on));
		
		if ( bind(sock, (struct sockaddr *) &sin, sizeof(struct sockaddr)) < 0 ) {
			syslog(LOG_DAEMON|LOG_ERR, "could not bind name to socket %s(%d)\n",
			strerror(errno), errno);
			return -1;
		}
    }


	return sock;
}


int makesock3tcp(int port)
{

    struct sockaddr_in sin;
    int sock = 0;
    const int on = 1;

    if (port) {
		sin.sin_family = AF_INET;
		sin.sin_port = htons(port);
		sin.sin_addr.s_addr = INADDR_ANY;
		sock = socket(PF_INET, SOCK_STREAM, 0);
		setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *) &on, sizeof(on));
		
		if ( bind(sock, (struct sockaddr *) &sin, sizeof(struct sockaddr)) < 0) {
			syslog(LOG_DAEMON|LOG_ERR, "could not bind name to socket %s(%d)\n",
			strerror(errno), errno);
			return -1;
		}
    }


	return sock;
}
