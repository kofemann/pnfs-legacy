#include "mount3_prot.h"
#include <stdio.h>
#include <stdlib.h>
#include <rpc/pmap_clnt.h>
#include <string.h>
#include <memory.h>
#include <sys/socket.h>
#include <netinet/in.h>

extern void
mount_program_3(struct svc_req *rqstp, register SVCXPRT *transp);

int
main (int argc, char **argv)
{
	SVCXPRT *transpUDP, *transpTCP;    

	pmap_unset (MOUNT_PROGRAM, MOUNT_V3);

	transpUDP = svcudp_create(RPC_ANYSOCK);
	transpTCP = svctcp_create(RPC_ANYSOCK , 0, 0);
	
	if (transpTCP == NULL) {
		fprintf (stderr, "%s", "cannot create tcp service.\n");
		exit(1);
	}
	if (transpUDP == NULL) {
		fprintf (stderr, "%s", "cannot create udp service.\n");
		exit(1);
	}

	if (!svc_register(transpUDP, MOUNT_PROGRAM, MOUNT_V3, mount_program_3, IPPROTO_UDP)) {
		fprintf (stderr, "unable to register (MOUNT_PROGRAM, MOUNT_V3, udp).\n");
		exit(1);
	}

	if (!svc_register(transpTCP, MOUNT_PROGRAM, MOUNT_V3, mount_program_3, IPPROTO_TCP)) {
		fprintf (stderr, "unable to register (MOUNT_PROGRAM, MOUNT_V3, tcp).\n");
		exit(1);
	}


	unfsmntd_init(argc,argv);
	svc_run ();
	fprintf (stderr, "%s", "svc_run returned\n");
	exit (1);
	/* NOTREACHED */
}
