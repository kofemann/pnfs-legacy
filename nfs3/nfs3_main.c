#include "nfs3_prot.h"
#include <stdio.h>
#include <stdlib.h>
#include <rpc/pmap_clnt.h>
#include <string.h>
#include <memory.h>
#include <sys/socket.h>
#include <netinet/in.h>


static int nfsDefaultPort = 2051;


extern void
nfs_program_3(struct svc_req *rqstp, register SVCXPRT *transp);

int
main (int argc, char **argv)
{

	SVCXPRT *transpUDP, *transpTCP;
	int  udpS, tcpS;
	char *port;	
	
	pmap_unset (NFS_PROGRAM, NFS_V3);

	port = getenv("PNFS_PORT");
	if( port != NULL ) {
		nfsDefaultPort = atoi(port);
	}


	udpS = makesock3udp(nfsDefaultPort);
	tcpS = makesock3tcp(nfsDefaultPort);
	
	if( (udpS < 0 ) || (tcpS < 0 )) {
		perror("unable to create socket");
		exit(1);
	}

	transpUDP = svcudp_create( udpS );
	if (transpUDP == NULL) {
		fprintf (stderr, "cannot create udp service.\n");
		exit(1);
	}
	if (!svc_register(transpUDP, NFS_PROGRAM, NFS_V3, nfs_program_3, IPPROTO_UDP)) {
		fprintf (stderr, "unable to register (NFS_PROGRAM, NFS_V3, udp).\n");
		exit(1);
	}


	transpTCP = svctcp_create( tcpS , 0, 0);
	if (transpTCP == NULL) {
		fprintf (stderr, "cannot create tcp service.\n");
		exit(1);
	}
	
	if (!svc_register(transpTCP, NFS_PROGRAM, NFS_V3, nfs_program_3, IPPROTO_TCP)) {
		fprintf (stderr, "unable to register (NFS_PROGRAM, NFS_V3, tcp).\n");
		exit(1);
	}


	unfsd_init(argc,argv) ;
	svc_run ();
	fprintf (stderr, "svc_run returned\n");
	exit (1);
	/* NOTREACHED */
}
