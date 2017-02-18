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
#include <stdio.h>
#include <stdlib.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <rpc/rpc.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <syslog.h>
#include <signal.h>

/* mask SUNOS/BSD4.3 syslog incompatibilities */
#ifndef LOG_DAEMON
#define	LOG_DAEMON	0
#endif							/* LOG_DAEMON */

#define MDL_ROOT "/usr/etc/pnfsSetup"

#include "mount3_prot.h"
#include "nfs3_prot.h"

#include "md2ptypes.h"
#include "md2types.h"
#include "md2log.h"
#include "md2scan.h"
#include "shmcom.h"
#include "sdef.h"
#include "sclib.h"
#include "fh3.h"

static md_id_t exportsDirID;
static md_id_t trustedDirID;
static md_id_t mountgroupDirID;

#define RD_READ_AHEAD   (8)

#ifdef SUN_BUG
static defErrMap[] = {
	NFS3ERR_STALE, NFS3ERR_STALE, NFS3ERR_NOENT, NFS3ERR_NOSPC,
	NFS3ERR_STALE, NFS3ERR_ISDIR, NFS3ERR_STALE, NFS3ERR_STALE,
	NFS3ERR_NOTDIR, NFS3ERR_NOTEMPTY, NFS3ERR_NOSPC, NFS3ERR_NOTDIR,
	NFS3ERR_EXIST, NFS3ERR_PERM, NFS3ERR_NODEV, NFS3ERR_STALE,
	NFS3ERR_DQUOT, NFS3ERR_STALE, NFS3ERR_STALE, NFS3ERR_STALE
};
#else
static defErrMap[] = {
	NFS3ERR_STALE, NFS3ERR_STALE, NFS3ERR_NOENT, NFS3ERR_NOSPC,
	NFS3ERR_IO, NFS3ERR_ISDIR, NFS3ERR_IO, NFS3ERR_STALE,
	NFS3ERR_NOTDIR, NFS3ERR_NOTEMPTY, NFS3ERR_NOSPC, NFS3ERR_NOTDIR,
	NFS3ERR_EXIST, NFS3ERR_PERM, NFS3ERR_NODEV, NFS3ERR_IO,
	NFS3ERR_DQUOT, NFS3ERR_STALE, NFS3ERR_STALE, NFS3ERR_STALE
};
#endif
#ifdef NO_ANSWER_ON_TIMEOUT
#define mapDefErrCodes(n)   (  (n)==0?NFS3_OK:\
    (((n)==SCL_NOSERVER)||((n)==SCL_TIMEOUT))?NFSERR_NOANSWER:\
    ((((n)>MDEbase)||((n)<MDElast))?NFS3ERR_STALE:defErrMap[MDEbase-(n)]))
#else
#define mapDefErrCodes(n)   (  (n)==0?NFS3_OK:\
    (((n)==SCL_NOSERVER)||((n)==SCL_TIMEOUT))?NFS3ERR_NOENT:\
    ((((n)>MDEbase)||((n)<MDElast))?NFS3ERR_STALE:defErrMap[MDEbase-(n)]))
#endif
static char iobuf[NFS3_MAXDATA];

static writeverf3 wverf;

int use_hardlinks;
int maxMaskBits = 8;

void termSignal(int signal);
void usr1Signal(int signal);
int mdmScanLine(char *string, char *cptr[], int maxPtr);
int mdmFindMountpoint(md_id_t id, char *mpoint, char *cptr[], int maxPtr);
int fh3_isPnfsLink(char *path, char **dir, char **base);
void mdmIpNumberToString(unsigned long host, char *hostIP);
void mdmIpNumberToStringMask(unsigned long host, char *hostIP, int pos);
int scanAttributeLine(char *line, md_unix * attr);


/*
 *   our pointer to the pnfs database
 */

SCL *scl = NULL;


/*
 * make us a server
 */
int unfsd_init(int argc, char *argv[])
{
	return unfsmntd_init(argc, argv);
}
int unfsmntd_init(int argc, char *argv[])
{

#ifndef DEBUG
	int fd;

	if (fork())
		exit(0);

	close(0);
	close(1);
	close(2);

#ifdef solaris
	setpgrp();
#else
	if ((fd = open("/dev/tty", 2)) >= 0) {
		ioctl(fd, TIOCNOTTY, (char *) 0);
		(void) close(fd);
	}
#endif

#endif							/* DEBUG */
	fh3_init(argc, argv);
	return 0;
}

/* ====================================================================== */
void fh3_exit()
/* ====================================================================== */
{
	if (scl)
		sclClientClose(scl);
	exit(0);
}

/* ====================================================================== */
void fh3_init(int argc, char *argv[])
/* ====================================================================== */
{
	int i, rc, len;
	md_id_t res, id;
	char dbName[128], string[128], *environment, *env;
	md_cookie cookie;
	sc_fs_format pnfsFile;
	FILE *f;
	key_t shmkey;
	char *error, *myName;

	myName = (myName =
			  strrchr(argv[0], '/')) == NULL ? argv[0] : myName + 1;

/*
 * read the basic parameters from /usr/etc/pnfsSetup 
 *
 *  syntax :
 *    #
 *    #    the database file
 *    #  
 *    shmkey=1122
 *    #
 *    #    the pnfs local configuration file
 *    #
 *    environment=/0/root/fs/admin/etc/environment
 */
	if (((f = fopen(MDL_ROOT, "r")) == NULL)) {
		fprintf(stderr, " Sorry, %s not found : %d\n", MDL_ROOT, errno);
		exit(4);
	} else {
		while (fgets(string, 128, f) != NULL) {
			if ((*string == '#') || ((len = strlen(string)) < 2))
				continue;
			string[len - 1] = '\0';
			if ((env = malloc(len + 16)) == NULL) {
				fprintf(stderr, " Malloc failed(exit)\n");
				exit(1);
			}
			strcpy(env, string);
			putenv(env);
		}
	}
	fclose(f);
/* ............................................................................. */
	/* 
	 * get the shm id from the argument list, the environment
	 */
	if (argc > 1) {
		env = argv[1];
	} else {
		if ((env = getenv("shmkey")) == NULL)
			env = "1122";
	}
	sscanf(env, "%x", &shmkey);
	/* 
	 * get the db environment from the argument list, the environment
	 */
	if (argc > 2) {
		env = argv[2];
	} else {
		if ((env = getenv("environment")) == NULL)
			env = "/0/root/fs/admin/etc/environment";
	}
	environment = env;
	/*
	 * get the hardlink flag
	 */
	if ((env = getenv("hardlinks")) == NULL)
		env = "on";
	use_hardlinks = strcmp(env, "on") == 0;
	/*
	 * get the max host mask
	 */
	if ((env = getenv("netmask")) == NULL)
		env = "8";
	sscanf(env, "%d", &maxMaskBits);
/*
 * do the fork
 */
	{
		int copies;
		if ( (!strcmp(myName, "pnfsd") ) || (!strcmp(myName, "pnfs3d") ) ) {
			if ((env = getenv("pnfscopies")) == NULL)
				env = "3";
			sscanf(env, "%d", &copies);
			copies = copies < 0 ? 0 : copies;
			copies = copies > 6 ? 6 : copies;

			for (i = 0; i < copies; i++)
				if (fork() == 0)
					break;
		}
	}
/* 
 * open the database and read the config file
 */
	if (!(scl = sclClientOpen(shmkey, 12 * 1024, &rc))) {
		error = sclError("sclClientOpen", rc);
		fprintf(stderr, " Problem : %s(%d)\n", error, errno);
		exit(2);
	}


	if (rc = mdmGetObjectID(scl, environment, &res)) {
		fprintf(stderr, " Sorry, can't get ID of %s\n", environment);
	} else {
		if (rc = scFs_FOpen(scl, &pnfsFile, res, 0, "r")) {
			fprintf(stderr, " Sorry, can't open ID %s\n", mdStringID(res));
		} else {
			while (scFs_Ffgets(string, 128, &pnfsFile) != NULL) {
				if ((*string == '#') || ((len = strlen(string)) < 2))
					continue;
				string[len - 1] = '\0';
				if ((env = malloc(len + 16)) == NULL) {
					fprintf(stderr, " Malloc failed\n");
					continue;
				}
				strcpy(env, string);
				putenv(env);
			}
		}
	}
/* ............................................................................. */

	if ((env = getenv("exports")) == NULL) {
		fprintf(stderr, " Sorry, can't find 'exports' in environment\n");
		env = "/0/root/fs/admin/etc/exports";
		fprintf(stderr, " Using %s\n", env);
	}
	if (rc = mdmGetObjectID(scl, env, &exportsDirID)) {
		fprintf(stderr, " Sorry, can't get ID of exports directory : %s\n",
				env);
		sclClientClose(scl);
		exit(2);
	}
	
   if( rc =  mdmLookupAuth_0( scl , NULL , exportsDirID , "trusted" , &trustedDirID ) ){
      mdSetNullID( trustedDirID ) ;
   }
   if( rc =  mdmLookupAuth_0( scl , NULL , exportsDirID , "mountpoints" , &mountgroupDirID ) ){
      mdSetNullID( mountgroupDirID ) ;
   }
/* ............................................................................. */
/* ............................................................................. */
/*
 * install the term signal handler
 */
	{
		struct sigaction newAction;

		memset((char *) &newAction, 0, sizeof(newAction));
		newAction.sa_handler = termSignal;

		sigaction(SIGTERM, &newAction, NULL);
		sigaction(SIGINT, &newAction, NULL);

		memset((char *) &newAction, 0, sizeof(newAction));
		newAction.sa_handler = usr1Signal;

		sigaction(SIGUSR1, &newAction, NULL);
	}
/* ............................................................................. */
/*
 * install the log file facility
 */
	{
		char string[128], envName[128];
		struct tm *tms;
		time_t clock;
		int level;

		sprintf(envName, "%sLog", myName);
		if ((env = getenv(envName)) == NULL) {

			sprintf(string, "/tmp/%s-%d", myName, getpid());
			env = string;
		}
		fprintf(stderr, " Writing to %s\n", env);
		md2pOpen(env, 0);

		sprintf(envName, "%sLevel", myName);
		if ((env = getenv(envName)) == NULL) {
			md2pNewLevel(md2pMODINFO);	/* only interested in modifications */
		} else {
			sscanf(env, "%d", &level);
			md2pNewLevel(level);
		}
		time(&clock);
		tms = localtime(&clock);
		rc = strftime(string, 128, " %D %T ", tms);
		if (rc <= 0)
			string[0] = '\0';
		md2pPrintf(md2pMaxLevel | md2pW, "%s 0.0.0.0-0-0 - start \n",
				   string);



		/* init NFS3 write cookie */

		*(wverf + 0) = (uint32) getpid();
		*(wverf + 4) = (uint32) time(NULL);

		fprintf(stderr, "fh3 init done\n");
	}
	return;
}

/* ====================================================================== */
void termSignal(int signal)
/* ====================================================================== */
{
	char string[128];
	struct tm *tms;
	time_t clock;
	int rc;

	time(&clock);
	tms = localtime(&clock);
	rc = strftime(string, 128, " %D %T ", tms);
	if (rc <= 0)
		string[0] = '\0';
	md2pPrintf(md2pMaxLevel | md2pW, "%s 0.0.0.0-0-0 - stop\n", string);
	fh3_exit();
}


void fprintf_id(FILE * f, const char *prefix, md_id_t * id)
{
	/*fprintf(f, "%s db=%d ext=%d high=%d low=%d (%s)\n", 
	   prefix, id->db, id->ext, id->high, id->low, mdStringID(*id) );
	   fflush(f); */
}

/* ====================================================================== */
void fprint_nfs_fh3(FILE * stream, nfs_fh3 * fh)
{
/* ====================================================================== */
	int l = fh->data.data_len;
	int i;
	mdFhandle *m;

	fprintf(stream, "fhandle3=(%d)[", l);
	for (i = 0; i < l; i++)
		fprintf(stream, "%2.2X", (unsigned char) fh->data.data_val[i]);
	fprintf(stream, "]\n");

	m = (mdFhandle *) fh->data.data_val;

	fprintf_id(stream, "mountid: ", &m->mountID);


	return;
}

/* ====================================================================== */
void usr1Signal(int signal)
/* ====================================================================== */
{
	int rc, level;
	char string[128];
	struct tm *tms;
	time_t clock;

	level = md2pNewLevel(md2pIncLevel);
	time(&clock);
	tms = localtime(&clock);
	rc = strftime(string, 128, " %D %T ", tms);
	if (rc <= 0)
		string[0] = '\0';
	md2pPrintf(md2pMaxLevel, "%s 0.0.0.0-0-0 - newlevel %d\n", string,
			   level);
	return;
}


/* ====================================================================== */
void fh3_make_root_trusted(md_auth * auth)
/* ====================================================================== */
{
	sc_fs_format pnfsFile;
	md_id_t res;
	int rc, len;
	char trustFile[128], *string;

	if (auth->uid) {
		auth->priv = 0;
		return;
	}
	if (auth->host == 0x7f000001) {
		auth->priv = 15;
		return;
	}

	string = trustFile;
#ifdef _LITTLE_ENDIAN
	sprintf(trustFile, "%s/%u.%u.%u.%u",
			"/0/root/fs/admin/etc/exports/trusted", auth->host & 0xFF,
			(auth->host >> 8) & 0xFF, (auth->host >> 16) & 0xFF,
			auth->host >> 24);
#else
	sprintf(trustFile, "%s/%u.%u.%u.%u",
			"/0/root/fs/admin/etc/exports/trusted",
			auth->host >> 24, (auth->host >> 16) & 0xFF,
			(auth->host >> 8) & 0xFF, auth->host & 0xFF);
#endif

	if (rc = mdmGetObjectID(scl, trustFile, &res)) {
		/*md2pPrintf(md2pMOREINFO," denied : %s",trustFile ); */
		auth->priv = 0;
	} else {
		if (rc = scFs_FOpen(scl, &pnfsFile, res, 0, "r")) {
			/*md2pPrintf(md2pMOREINFO," Sorry, can't open ID %s " ,mdStringID( res) ) ; */
			auth->priv = 0;
		} else {
			while (scFs_Ffgets(string, 128, &pnfsFile) != NULL) {
				if ((*string == '#') || ((len = strlen(string)) < 2))
					continue;
				string[len - 1] = '\0';
				sscanf(string, "%d", &auth->priv);
				break;
			}
			auth->priv =
				auth->priv < 0 ? 0 : auth->priv > 15 ? 15 : auth->priv;
		}
	}
	return;

}

/* ====================================================================== */
int fh3_create(md_auth * auth, fhandle3 * fh, char *path)
/* ====================================================================== */
{

	mdFhandle *m = (mdFhandle *) malloc(sizeof(mdFhandle));
	int i, rc, items, len;
	char hostIP[128], string[256], *item[8], *in;
	md_id_t res;
	sc_fs_format pnfsFile;
	long ip;
	int maskBits;

	ip = auth->host;

	mdmIpNumberToString(ip, hostIP);
	md2pPrintf(md2pMODINFO, " lookup %s %s ", hostIP, path);

  if( mdmLookupAuth_0( scl , auth , exportsDirID , hostIP , &res ) ){
		md2pPrintf(md2pMODINFO, " (SN: ");
		for (maskBits = 1; maskBits <= maxMaskBits; maskBits++) {
			md2pPrintf(md2pMODINFO, "%d:", maskBits);
			mdmIpNumberToStringMask(ip, hostIP, maskBits);
			if( ! mdmLookupAuth_0( scl , auth ,exportsDirID , hostIP , &res ) ) {
				break;
			}
		}
		md2pPrintf(md2pMODINFO, "%s)", hostIP);
		if (maskBits > maxMaskBits)
			return NFS3ERR_PERM;
	}
	rc = mdmFindMountpoint(res, path, item,
						   sizeof(item) / sizeof(item[0]));
	if (rc < 0) {
		md2pPrintf(md2pMODINFO, " mdmFindMountpoint : Can't find %s -> %d",
				   path, rc);
		return NFS3ERR_PERM;
	}

	if (rc = mdmGetObjectID(scl, item[1], &res)) {
		md2pPrintf(md2pMODINFO, " mdmGetObjectID : Can't find %s\n",
				   item[1]);
		return NFS3ERR_PERM;
	}

	m->mountID = m->id = res;

	md2ScanPermission(item[2], &(m->permission));

	fprintf(stderr, "handle : %s %s\n", mdStringID(res),
			mdStringFhandle(*m));

	fh->fhandle3_len = sizeof(mdFhandle);
	fh->fhandle3_val = (char *) m;

	fprintf(stderr, "fhandle : ");
	fprint_nfs_fh3(stderr, (nfs_fh3 *) fh);

	return NFS3_OK;
}
void mdmIpNumberToString(unsigned long host, char *hostIP)
{
#ifdef _LITTLE_ENDIAN
	sprintf(hostIP, "%u.%u.%u.%u", host & 0xFF,
			(host >> 8) & 0xFF, (host >> 16) & 0xFF, host >> 24);
#else
	sprintf(hostIP, "%u.%u.%u.%u",
			host >> 24, (host >> 16) & 0xFF,
			(host >> 8) & 0xFF, host & 0xFF);
#endif
	return;
}
void mdmIpNumberToStringMask(unsigned long inhost, char *hostIP, int pos)
{
	unsigned long mask = 0xFFFFFFFF, x;
	unsigned long host;
	int i;

	for (i = 0; i < pos; i++) {
		x = 1 << i;
		mask &= ~x;
	}
#ifdef _LITTLE_ENDIAN
	host = ((inhost >> 24) & 0xFF) |
		(((inhost & 0xFF0000) >> 8) & 0xFFFF) |
		(((inhost & 0x00FF00) << 8) & 0xFFFFFF) |
		(((inhost & 0x0000FF) << 24));
#else
	host = inhost;
#endif
	host &= mask;
	sprintf(hostIP, "%u.%u.%u.%u..%u.%u.%u.%u",
			mask >> 24,
			(mask >> 16) & 0xFF,
			(mask >> 8) & 0xFF,
			mask & 0xFF,
			host >> 24,
			(host >> 16) & 0xFF, (host >> 8) & 0xFF, host & 0xFF);
	return;
}

int mdmFindMountpoint(md_id_t id, char *mpoint, char *cptr[], int maxPtr)
{
	static char outString[1024];
	static int levelCounter = 0;
	char string[1024];
	sc_fs_format pnfsFile;
	int rc, len, items;
	char *in, *item[4];

	if (levelCounter++ > 8) {
		levelCounter--;
		return -22;
	}
	if (maxPtr < 4) {
		levelCounter--;
		return -2;
	}
	if (rc = scFs_FOpen(scl, &pnfsFile, id, 0, "rl")) {
		md2pPrintf(md2pMODINFO, " mdmFindMountpoint : Can't open %s\n",
				   mdStringID(id));
		{
			levelCounter--;
			return -1;
		}
	}
	while ((in = scFs_Ffgets(string, 256, &pnfsFile)) != NULL) {
		if ((*string == '#') || ((len = strlen(string)) < 2))
			continue;
		string[len - 1] = '\0';
		/*   <virtualMountpoint>  <realPath> <permission> <flags>   */
		/*   <virtualMountpoint>                                    */
		/*   [<mountgroup>:]<virtualMountpoint>                     */
		items = mdmScanLine(string, item, sizeof(item) / sizeof(item[0]));
		if (items == 4) {
			if (!strcmp(mpoint, item[0])) {
				/*
				 * the only valid output
				 */
				int i;
				char *x = outString;
				for (i = 0; i < 4; i++) {
					strcpy(x, item[i]);
					cptr[i] = x;
					x += strlen(item[i]) + 1;
				}
				{
					levelCounter--;
					return 0;
				}
			}
		} else if (items == 2) {
			char *del = strchr(item[1], ':');
			char *mountGroup;
			char *newMount;
			char *mountPoint = item[0];
			int isWildcard = !strcmp(mountPoint, "*");
			md_id_t res;

			if ((!isWildcard) && (strcmp(mpoint, mountPoint)))
				continue;

			if (del == NULL) {
				if ((item[1])[0] == '/') {
					newMount = item[1];
					mountGroup = NULL;
				} else {
					newMount = mpoint;
					mountGroup = item[1];
				}
			} else {
				*del = '\0';
				del++;
				mountGroup = item[1];
				newMount = del;
			}
			if ((mountGroup != NULL) && mdIsNullID(mountgroupDirID)) {
				md2pPrintf(md2pMODINFO,
						   " mdmFindMountpoint : mountgroupDirID = null\n");
				{
					levelCounter--;
					return -3;
				}
			}
			if (mountGroup == NULL) {
				res = id;
			} else {
              if( mdmLookupAuth_0( scl , NULL ,  mountgroupDirID , mountGroup , &res ) ){
					md2pPrintf(md2pMODINFO,
							   " mdmFindMountpoint : Not found %s %s ",
							   mdStringID(mountgroupDirID), mountGroup);
					{
						levelCounter--;
						return -4;
					}
				}
			}
			md2pPrintf(md2pMODINFO,
					   "(%s:%s) ",
					   mountGroup == NULL ? "this" : mountGroup, newMount);
			if (!mdmFindMountpoint(res, newMount, cptr, maxPtr)) {
				levelCounter--;
				return 0;
			}

		}

	}
	{
		levelCounter--;
		return -6;
	}

}
int mdmScanLine(char *string, char *cptr[], int maxPtr)
{
	char *x;
	int state, cursor;
	for (state = 0, x = string, cursor = 0; *x != '\0'; x++) {

		switch (state) {

		case 0:
			if (*x == ' ')
				continue;
			cptr[cursor++] = x;
			state = 1;
			break;

		case 1:
			if (*x != ' ')
				continue;
			*x = '\0';
			state = 0;
			break;
		}
		if (cursor >= maxPtr)
			break;
	}
	return cursor;
}


int uType2Nfs(int uType)
{
	int ret = 0;

	int mask = 0170000;

	switch (uType & mask) {

	case S_IFREG:
		ret = NF3REG;
		break;
	case S_IFDIR:
		ret = NF3DIR;
		break;
	case S_IFLNK:
		ret = NF3LNK;
		break;
	case S_IFSOCK:
		ret = NF3SOCK;
		break;
	case S_IFBLK:
		ret = NF3BLK;
		break;
	case S_IFCHR:
		ret = NF3CHR;
		break;
	case S_IFIFO:
		ret = NF3FIFO;
		break;
	default:
		ret = 0;

	}

	return ret;
}

/* ====================================================================== */
nfsstat3 fh3_u2f_attr(md_unix * s, fattr3 * attr)
/* ====================================================================== */
{

	attr->type = uType2Nfs(s->mst_mode);
	attr->mode = s->mst_mode & 0170777;
	attr->nlink = s->mst_nlink;
	attr->uid = s->mst_uid;
	attr->gid = s->mst_gid;
	
	attr->size = s->mst_sizeHigh;
	attr->size = (attr->size  << 32) | s->mst_size ;
	
	attr->used = s->mst_blocks * 512;
	attr->rdev.specdata1 = (s->mst_rdev >> 8) & 0xFF;
	attr->rdev.specdata2 = s->mst_rdev & 0xFF;;
	attr->fsid = s->mst_dev;
	attr->fileid = s->mst_ino;
	attr->atime.seconds = s->mst_atime;
	attr->atime.nseconds = 0;
	attr->mtime.seconds = s->mst_mtime;
	attr->mtime.nseconds = 0;
	attr->ctime.seconds = s->mst_ctime;
	attr->ctime.nseconds = 0;

	return NFS3_OK;
}

void nfs_fh3_2_mdFhandle(nfs_fh3 * fh, mdFhandle * m)
{
	int len = fh->data.data_len > sizeof(mdFhandle) ?
		sizeof(mdFhandle) : fh->data.data_len;

	memcpy(m, fh->data.data_val, len);

	return;
}


void mdFhandle_2_nfs_fh3(nfs_fh3 * fh, mdFhandle * m)
{

	if (fh->data.data_val != NULL) {
		fh->data.data_len = sizeof(mdFhandle);
		memcpy(fh->data.data_val, m, fh->data.data_len);
	}
	return;
}


/* ====================================================================== */
nfsstat3 fh3_getattr(nfs_fh3 * fh, fattr3 * attr, md_auth * auth)
/* ====================================================================== */
{
	md_unix uattr;
	int rc;
	mdFhandle M;
	mdFhandle *m = &M;

	nfs_fh3_2_mdFhandle(fh, m);

	md2pPrintf(md2pMOREINFO, " %s ", mdStringFhandle(*m));
    rc = mdmGetAttrAuth( scl , auth , m->id ,  m->permission , &uattr ) ;

	if (rc) {
		md2pPrintf(md2pMOREINFO, " (%d) ", rc);
		return mapDefErrCodes(rc);
	}
	fh3_u2f_attr(&uattr, attr);
	md2pPrintf(md2pMOREINFO, " t=%d;m=%o;s=%d;i=%x;cmat=%x-%x-%x ",
			   attr->type, attr->mode, attr->size, attr->fileid,
			   attr->ctime.seconds,
			   attr->mtime.seconds, attr->atime.seconds);
	md2pPrintf(md2pMOREINFO, " (%d) ", rc);
	return NFS3_OK;
}


/* ====================================================================== */
nfsstat3 fh3_read_data(READ3args * argp, READ3res * res, md_auth * auth)
/* ====================================================================== */
{

	mdFhandle M;
	mdFhandle *m = &M;
	int rc, oType;
	md_unix uattr;
	mdRecord record;
	md_id_t id;

	nfs_fh3_2_mdFhandle(&argp->file, m);
	fprint_nfs_fh3(stderr, &(argp->file));

	memcpy((char *) &(auth->mountID), (char *) &(m->mountID),
		   sizeof(md_id_t));

	md2pPrintf(md2pINFO, " %s %X %X ",
			   mdStringFhandle(*m), (long) argp->offset, argp->count);
	oType = mdpGetSpecial(m->permission);
	if (oType == MDO_REQ_NAMEOF) {
		id = m->id;
		mdClearSpecialID(id);
		rc = mdmGetRecord(scl, id, &record);
		if (rc < 0)
			goto weiter;
		rc = mdmFindIdAuth(scl, auth, id, m->permission,
						   record.head.parentID, argp->count, iobuf,
						   &uattr);
		if (rc < 0)
			goto weiter;

	} else {
		rc = mdmReadDataAuth(scl, auth, m->id, m->permission,
							 (long) argp->offset, (int) argp->count, iobuf,
							 &uattr);
	}
  weiter:
	md2pPrintf(md2pINFO, " (%d) ", rc);
	if (rc < 0)
		return mapDefErrCodes(rc);

	res->READ3res_u.resok.count = rc;
	res->READ3res_u.resok.data.data_len = rc;
	res->READ3res_u.resok.data.data_val = iobuf;

	res->READ3res_u.resok.file_attributes.attributes_follow = 1;
	fh3_u2f_attr(&uattr,
				 &(res->READ3res_u.resok.file_attributes.post_op_attr_u.
				   attributes));
	return NFS3_OK;
}



/* ====================================================================== */
nfsstat3 fh3_createFile(CREATE3args * argp, CREATE3res * res,
						md_auth * auth)
/* ====================================================================== */
{

	mdFhandle M1;
	mdFhandle *dir = &M1;
	mdFhandle M2;
	mdFhandle *out = &M2;


	char *name = argp->where.name;
	sattr3 *a = &argp->how.createhow3_u.obj_attributes;
	md_id_t resID;
	int rc;
	md_unix uattr;
	md_permission resPerm;
	md_object *object;
	int64_t newSize = 0;

	nfs_fh3_2_mdFhandle(&argp->where.dir, dir);

	md2pPrintf(md2pMODINFO, " dir %s name %s ", mdStringFhandle(*dir),
			   name);
	md2pPrintf(md2pMODINFO, " uid=%d;gid=%d;size=%d;mode=%o;a=%x;m=%x",
			   a->uid, a->gid, a->size, a->mode, a->atime, a->mtime);

	if (strlen(name) >= MD_MAX_NAME_LENGTH)
		return NFS3ERR_NAMETOOLONG;

	fh3_s2u_attr(&argp->how.createhow3_u.obj_attributes, &uattr);

	uattr.mst_uid = auth->uid;
	uattr.mst_gid = auth->gid;


	if ((object = md2IsObject(name)) &&
		(md2ObjectType(object) == MDO_REQ_PSET) && (object->argc > 2)) {

		md2pPrintf(md2pMODINFO, " command pset (%s) ", object->argv[1]);

		/*        ----------------------             */
		/* touch .(pset)(<pnfsid>)(<commmand>)( ...) */
		/*        ----------------------             */
		if (object->argc < 2) {
			rc = MDEnotFound;
			goto abort;
		}
		if (!strcmp(object->argv[1], "size")) {
			/* touch .(pset)(<pnfsid>)(size)(<size>) */
			char *c, *d;
			md_id_t id;
			int level = mdGetLevel(dir->permission);
			md_unix unixAttr;

			if (object->argc < 3) {
				rc = MDEnotFound;
				goto abort;
			}
			c = md2ObjectName(object);
			md2ScanId(c, &id);
			md2pPrintf(md2pMODINFO, ";id=%s", mdStringID(id));
			sscanf(object->argv[2], "%lld", &newSize);
			md2pPrintf(md2pMODINFO, ";new size=%lld", newSize);						
			unixAttr.mst_size = (unsigned long)newSize;
			unixAttr.mst_sizeHigh = (unsigned long)(newSize >> 32 );
			md2pPrintf(md2pMODINFO, ";attr size=%lu", unixAttr.mst_size);						
			md2pPrintf(md2pMODINFO, ";attr sizeHigh=%lu", unixAttr.mst_sizeHigh);									
			unixAttr.mst_mode = md_no_mode;
			unixAttr.mst_uid = md_no_uid;
			unixAttr.mst_gid = md_no_gid;
			unixAttr.mst_atime = md_no_time;
			unixAttr.mst_mtime = md_no_time;

			rc = mdmSetAttr(scl, auth, id, dir->permission, &unixAttr);
			if (rc)
				goto abort;
	
            rc = mdmGetExtAttrAuth( scl , NULL ,id , dir->permission , &uattr , NULL ) ;			
			resID = id;
			resPerm = dir->permission;
			mdpSetSpecial(resPerm, MDO_REQ_IGNORE);
			mdSetSpecialID(resID);

		} else if (!strcmp(object->argv[1], "attr")) {
			/* touch .(pset)(<pnfsid>)(attr)(<level>)(<attr>) */
			char *c, *d;
			md_id_t id;
			int level;
			md_unix unixAttr;

			if (object->argc < 4) {
				rc = MDEnotFound;
				goto abort;
			}
			c = md2ObjectName(object);
			md2ScanId(c, &id);
			md2pPrintf(md2pMODINFO, ";id=%s;", mdStringID(id));
			md2pPrintf(md2pMODINFO, ";level=%s;", object->argv[2]);
			md2pPrintf(md2pMODINFO, ";line=%s;", object->argv[3]);

			sscanf(object->argv[2], "%d", &level);
			if ((level < 0) || (level > 7)) {
				rc = MDEnotFound;
				goto abort;
			}
			rc = scanAttributeLine(object->argv[3], &unixAttr);
			if (rc)
				goto abort;

			unixAttr.mst_size = md_no_size;
			unixAttr.mst_sizeHigh = md_no_size;
			
			mdpSetLevel(dir->permission, level);

			rc = mdmSetAttrs(scl, auth, id, dir->permission, &unixAttr);
			if (rc) {
				goto abort;
			}

            rc = mdmGetExtAttrAuth( scl , NULL ,id , dir->permission , &uattr , NULL ) ;
			resID = id;
			resPerm = dir->permission;
		} else {
			rc = MDEnotFound;
			goto abort;
		}
	} else {

		rc = mdmCreateFile(scl, auth, dir->id, dir->permission,
						   name, &uattr, &resPerm, &resID);
	}

  abort:

	if (!rc) {
		memcpy((char *) out, (char *) dir, sizeof(*dir));
		mdFhandle_2_nfs_fh3(&res->CREATE3res_u.resok.obj.post_op_fh3_u.
							handle, out);
		out->id = resID;
		out->permission = resPerm;
		res->CREATE3res_u.resok.obj_attributes.attributes_follow = 1;
		res->CREATE3res_u.resok.dir_wcc.before.attributes_follow = 0;
		res->CREATE3res_u.resok.dir_wcc.after.attributes_follow = 0;
		fh3_u2f_attr(&uattr,
					 &(res->CREATE3res_u.resok.obj_attributes.
					   post_op_attr_u.attributes));
		md2pPrintf(md2pMODINFO, " : %s ", mdStringFhandle(*out));

	}
	md2pPrintf(md2pMODINFO, " (%d) ", rc);

	return mapDefErrCodes(rc);

}


/* ====================================================================== */
nfsstat3 fh3_s2u_attr(sattr3 * a, md_unix * attr)
/* ====================================================================== */
{
	int rc;


	if ( a->uid.set_it == TRUE ) {
		attr->mst_uid = a->uid.set_uid3_u.uid;
	} else {
		attr->mst_uid = md_no_uid;
	}

	if ( a->gid.set_it == TRUE ) {
		attr->mst_gid = a->gid.set_gid3_u.gid;
	} else {
		attr->mst_gid = md_no_gid;
	}

	if ( a->size.set_it == TRUE ) {
		attr->mst_sizeHigh = a->size.set_size3_u.size >> 32;
		attr->mst_size = a->size.set_size3_u.size;		
	} else {
		attr->mst_size = md_no_size;
		attr->mst_sizeHigh = md_no_size;		
	}

	if (a->atime.set_it != DONT_CHANGE) {

		if (a->atime.set_it == SET_TO_SERVER_TIME) {
			attr->mst_atime = time(NULL);
		} else {
			attr->mst_atime = a->atime.set_atime_u.atime.seconds;
		}
	} else {
		attr->mst_atime = md_no_time;
	}

	if (a->mtime.set_it != DONT_CHANGE) {
		if (a->mtime.set_it == SET_TO_SERVER_TIME) {
			attr->mst_mtime = time(NULL);
		} else {
			attr->mst_mtime = a->mtime.set_mtime_u.mtime.seconds;
		}
	} else {
		attr->mst_mtime = md_no_time;
	}

	if ((int) a->mode.set_it == TRUE) {
		attr->mst_mode =
			(attr->mst_mode & S_IFMT) | a->mode.set_mode3_u.mode;
	} else {
		attr->mst_mode = md_no_mode;
	}

	return 0;
}


int scanAttributeLine(char *line, md_unix * attr)
{
	char l[128];
	char *pos, *x = l;

	strncpy(l, line, 127);
	l[127] = '\0';

	if ((pos = strchr(x, ':')) == NULL)
		return -1;
	*pos = '\0';
	{
		int tmp;
		sscanf(x, "%o", &tmp);
		attr->mst_mode = tmp;
	}
	x = pos + 1;

	if ((pos = strchr(x, ':')) == NULL)
		return -1;
	*pos = '\0';
	sscanf(x, "%d", &(attr->mst_uid));
	x = pos + 1;

	if ((pos = strchr(x, ':')) == NULL)
		return -1;
	*pos = '\0';
	sscanf(x, "%d", &(attr->mst_gid));
	x = pos + 1;

	if ((pos = strchr(x, ':')) == NULL)
		return -1;
	*pos = '\0';
	sscanf(x, "%x", &(attr->mst_atime));
	x = pos + 1;

	if ((pos = strchr(x, ':')) == NULL)
		return -1;
	*pos = '\0';
	sscanf(x, "%x", &(attr->mst_mtime));
	x = pos + 1;

	if ((pos = strchr(x, ':')) != NULL)
		*pos = '\0';

	sscanf(x, "%x", &(attr->mst_ctime));

	return 0;

}


/* ====================================================================== */
nfsstat3 fh3_mkdir(MKDIR3args * argp, MKDIR3res * res, md_auth * auth)
/* ====================================================================== */
{


	mdFhandle M1;
	mdFhandle *dir = &M1;
	mdFhandle M2;
	mdFhandle *out = &M2;


	char *name = argp->where.name;
	md_id_t resID;
	int rc;
	md_unix uattr;
	md_object *object;

	nfs_fh3_2_mdFhandle(&argp->where.dir, dir);


	md2pPrintf(md2pMODINFO, " dir %s name %s ", mdStringFhandle(*dir),
			   name);

	if (strlen(name) >= MD_MAX_NAME_LENGTH)
		return NFS3ERR_NAMETOOLONG;

	fh3_s2u_attr(&argp->attributes, &uattr);

	uattr.mst_uid = auth->uid;
	uattr.mst_gid = auth->gid;


	if ((object = md2IsObject(name)) &&
		(md2ObjectType(object) == MDO_REQ_DIGITS) && (object->argc == 1)) {

		char string[128];
		int dbid;
		md_id_t res;
		md_dir_item item;

        rc =  mdmLookupAuth_0( scl , auth , dir -> id , object->argv[0] , &res );
		if (!rc) {
			rc = MDEexists;
			goto weiter;
		}
		sscanf(object->type, "%d", &dbid);
		sprintf(string, "/%d/root/fs", dbid);
		md2pPrintf(md2pMODINFO, "dbr=%s;", string);
		if (mdmGetObjectID(scl, string, &res))
			return NFS3ERR_NOENT;
		rc = mdmMkDir(scl, auth, res, dir->permission,
					  object->argv[0], &uattr, &resID);
		md2pPrintf(md2pMODINFO, "m=%d;", rc);
		if (rc)
			goto weiter;
		rc = mdmRmDirEntry(scl, auth, res, object->argv[0], &item);
		md2pPrintf(md2pMODINFO, "r=%d;", rc);
		if (rc)
			goto weiter;
		rc = mdmAddDirEntry(scl, auth,
							dir->id,
							dir->permission, object->argv[0], &item);
		md2pPrintf(md2pMODINFO, "a=%d;", rc);
		if (rc)
			goto weiter;
		rc = mdmChParent(scl, auth, item.ID, dir->id);
		md2pPrintf(md2pMODINFO, "c=%d;", rc);
		if (rc)
			goto weiter;

	} else {

		rc = mdmMkDir(scl, auth, dir->id, dir->permission,
					  name, &uattr, &resID);
	}
  weiter:

	md2pPrintf(md2pMODINFO, " : %s (%d) ", mdStringID(resID), rc);

	if (rc)
		return mapDefErrCodes(rc);

	fh3_u2f_attr(&uattr,
				 &(res->MKDIR3res_u.resok.obj_attributes.post_op_attr_u.
				   attributes));

	memcpy((char *) out, (char *) dir, sizeof(*dir));

	return NFS3_OK;

}


/* ====================================================================== */
nfsstat3 fh3_setattr(SETATTR3args * argp, SETATTR3res * res,
					 md_auth * auth)
/* ====================================================================== */
{
	md_unix uattr;
	sattr3 *a = &argp->new_attributes;
	mdFhandle M;
	mdFhandle *m = &M;
	int rc;

	nfs_fh3_2_mdFhandle(&argp->object, m);

	md2pPrintf(md2pMODINFO,
			   "%s-%s uid=%d;gid=%d;size=%d;mode=%o;a=%x;m=%x ",
			   mdStringID(m->id), mdStringPermission(m->permission),
			   a->uid.set_uid3_u.uid, a->gid.set_gid3_u.gid,
			   a->size.set_size3_u.size, a->mode.set_mode3_u.mode,
			   a->atime.set_atime_u.atime.seconds,
			   a->mtime.set_mtime_u.mtime.seconds);

	fh3_s2u_attr(a, &uattr);

	if ((uattr.mst_mtime != md_no_time) &&
		(uattr.mst_atime != md_no_time) && (uattr.mst_atime < 0x1000)) {

		res->SETATTR3res_u.resok.obj_wcc.after.attributes_follow = 1;
		fh3_u2f_attr(&uattr,
					 &(res->SETATTR3res_u.resok.obj_wcc.after.
					   post_op_attr_u.attributes));

		return fh3_command(auth, m->id, m->permission,
						   (md_long) uattr.mst_atime,
						   (md_long) uattr.mst_mtime);
	}

	rc = mdmSetAttr(scl, auth, m->id, m->permission, &uattr);
	md2pPrintf(md2pMODINFO, " (%d) ", rc);
	if (rc < 0)
		return mapDefErrCodes(rc);

	res->SETATTR3res_u.resok.obj_wcc.before.attributes_follow = 0;
	res->SETATTR3res_u.resok.obj_wcc.after.attributes_follow = 1;
	fh3_u2f_attr(&uattr,
				 &(res->SETATTR3res_u.resok.obj_wcc.after.post_op_attr_u.
				   attributes));
	return NFS3_OK;
}


/* ====================================================================== */
nfsstat3 fh3_command(md_auth * auth, md_id_t id, md_permission perm,
					 md_long com, md_long arg)
/* ====================================================================== */
{
	int rc;

	md2pPrintf(md2pMODINFO, " command %s-%s 0x%x 0x%x ",
			   mdStringID(id), mdStringPermission(perm), com, arg);

	if (com == 10) {
		if (!mdpIsNoIO(perm))
			return NFS3ERR_PERM;

		if (rc = mdmForceSize(scl, auth, id, perm, arg))
			return mapDefErrCodes(rc);
	}

	return NFS3_OK;
}


#ifdef NO_HARD_LINKS
/* ====================================================================== */
nfsstat3 fh3_rmfile(REMOVE3args * argp, md_auth * auth)
/* ====================================================================== */
{
	mdFhandle D;
	mdFhandle *dir = &D;
	char *name = argp->object.name;
	int rc;

	nfs_fh3_2_mdFhandle(&argp->object.dir, dir);
	md2pPrintf(md2pMODINFO, " dir %s name %s ", mdStringFhandle(*dir),
			   name);

	if (strlen(name) >= MD_MAX_NAME_LENGTH)
		return NFS3ERR_NAMETOOLONG;

	rc = mdmRmFile(scl, auth, dir->id, dir->permission, name);
	md2pPrintf(md2pMODINFO, " (%d) ", rc);

	return mapDefErrCodes(rc);;
}
#else
/* ====================================================================== */
nfsstat3 fh3_rmfile(REMOVE3args * argp, md_auth * auth)
/* ====================================================================== */
{
	mdFhandle D;
	mdFhandle *dir = &D;

	char *name = argp->object.name;
	int rc = 0, diff;
	md_unix attr;
	md_dir_item item;
	md_object *object;

	nfs_fh3_2_mdFhandle(&argp->object.dir, dir);

	md2pPrintf(md2pMODINFO, " dir %s name %s ", mdStringFhandle(*dir),
			   name);

	if (strlen(name) >= MD_MAX_NAME_LENGTH)
		return NFS3ERR_NAMETOOLONG;

	if ((object = md2IsObject(name)) &&
		(md2ObjectType(object) == MDO_REQ_PSET) && (object->argc > 2)) {

		/*        ----------------------             */
		/* touch .(pset)(<pnfsid>)(<commmand>)( ...) */
		/*        ----------------------             */
		if (!strcmp(object->argv[1], "size")) {
			char *c;
			md_id_t id;
			md_unix unixAttr;

			c = md2ObjectName(object);
			md2ScanId(c, &id);	
            rc = mdmGetExtAttrAuth( scl , NULL , id , dir->permission , &unixAttr , NULL ) ;
			if (rc)
				goto theEnd;
			unixAttr.mst_mode = md_no_mode;
			unixAttr.mst_uid = md_no_uid;
			unixAttr.mst_gid = md_no_gid;
			unixAttr.mst_atime = md_no_time;
			unixAttr.mst_mtime = md_no_time;
			unixAttr.mst_size = 0;
			rc = mdmSetAttr(scl, auth, id, dir->permission, &unixAttr);
		} else {
			rc = MDEnotFound;
		}
		goto theEnd;
	}
	/*
	 * This is a quick hack to get hard links working.
	 * First we simply try to use the old method (mdmRmFile).
	 * We have to do that because the request may concern a
	 * tag or a special file ( 'set' or 'io' ) and it calls
	 * the external 'remove' trigger if nlink==0 ;
	 * In addition it checks if we are allowed to perform the
	 * operation.
	 * Returning 'missmatch' gives us a hint that we hit
	 * a nlink>1. In this case we ... do all the rest.
	 */	
    rc = mdmLookupAuth_1( scl , NULL , dir->id ,  dir->permission ,name , &item , &attr  ) ;
	if (rc)
		goto theEnd;

	md2pPrintf(md2pMODINFO, " : %s ", mdStringID(item.ID));

	/*   mdmRmFile checks security  */
	rc = mdmRmFile(scl, auth, dir->id, dir->permission, name);
	md2pPrintf(md2pMODINFO, ";mdmRmFile=%d;", rc);
	if (rc != MDEdbMissmatch)
		goto theEnd;

	rc = mdmRmDirEntry(scl, auth, dir->id, name, &item);
	if (rc)
		goto theEnd;

	diff = -1;
	rc = mdmModLink(scl, NULL, item.ID, item.perm, &diff);
	if (rc)
		goto theEnd;

	md2pPrintf(md2pMODINFO, " nlinks %d ", diff);
	/*
	 * this code should not trigger as long as mdmRmFile is
	 * called.
	 */
	if ((diff < 1) || (diff > 20000)) {
		rc = mdmDelFile(scl, NULL, item.ID);
	}

  theEnd:
	md2pPrintf(md2pMODINFO, " (%d) ", rc);
	return mapDefErrCodes(rc);
}
#endif

/* ====================================================================== */
nfsstat3 fh3_write_data(WRITE3args * argp, WRITE3res * res, md_auth * auth)
/* ====================================================================== */
{
	mdFhandle M;
	mdFhandle *m = &M;
	int rc;
	md_unix uattr;


	nfs_fh3_2_mdFhandle(&argp->file, m);

	md2pPrintf(md2pMODINFO, " %s %X %X ",
			   mdStringFhandle(*m), (long) argp->offset,
			   argp->data.data_len);


	/* write data only if count is not zero */

	if (argp->count) {

		rc = mdmWriteDataAuth(scl, auth, m->id, m->permission,
							  (long) argp->offset, argp->count,
							  argp->data.data_val, &uattr);

		md2pPrintf(md2pMODINFO, " (%d) ", rc);

		if (rc < 0)
			return mapDefErrCodes(rc);

		if (argp->stable == DATA_SYNC)
			argp->stable = FILE_SYNC;
	}

	res->WRITE3res_u.resok.count = argp->data.data_len;
	res->WRITE3res_u.resok.committed = argp->stable;
	memcpy(res->WRITE3res_u.resok.verf, wverf, NFS3_WRITEVERFSIZE);


	fh3_u2f_attr(&uattr,
				 &(res->WRITE3res_u.resok.file_wcc.after.post_op_attr_u.
				   attributes));

	return NFS3_OK;

}

/* ====================================================================== */
nfsstat3 fh3_commit(COMMIT3args * args, COMMIT3res * res, md_auth * auth)
/* ====================================================================== */
{

	memcpy(res->COMMIT3res_u.resok.verf, wverf, NFS3_WRITEVERFSIZE);

	return NFS3_OK;
}



/* ====================================================================== */
nfsstat3 fh3_rmdir(RMDIR3args * argp, md_auth * auth)
/* ====================================================================== */
{

	mdFhandle D;
	mdFhandle *dir = &D;
	char *name = argp->object.name;
	int rc;

	nfs_fh3_2_mdFhandle(&argp->object.dir, dir);

	md2pPrintf(md2pMODINFO, " dir %s name %s ", mdStringFhandle(*dir),
			   name);

	if (strlen(name) >= MD_MAX_NAME_LENGTH)
		return NFS3ERR_NAMETOOLONG;

	rc = mdmRmDir(scl, auth, dir->id, dir->permission, name);
	md2pPrintf(md2pMODINFO, " (%d) ", rc);

	return mapDefErrCodes(rc);;
}



/* ====================================================================== */
nfsstat3 fh3_rename(RENAME3args * argp, md_auth * auth)
/* ====================================================================== */
{


	mdFhandle From;
	mdFhandle *from_m = &From;
	mdFhandle To;
	mdFhandle *to_m = &To;


	char *from_name = argp->from.name;
	char *to_name = argp->to.name;
	char tmp[64];
	int rc;

	nfs_fh3_2_mdFhandle(&argp->from.dir, from_m);
	nfs_fh3_2_mdFhandle(&argp->to.dir, to_m);

	strcpy(tmp, mdStringFhandle(*to_m));
	md2pPrintf(md2pMODINFO, " from %s %s to %s %s",
			   mdStringFhandle(*from_m), from_name, tmp, to_name);

	if (strlen(from_name) >= MD_MAX_NAME_LENGTH)
		return NFS3ERR_NAMETOOLONG;
	if (strlen(to_name) >= MD_MAX_NAME_LENGTH)
		return NFS3ERR_NAMETOOLONG;

	if (!(md2IsTagName(to_name) && md2IsTagName(from_name)))
		return NFS3ERR_PERM;

	rc = mdmRename(scl, auth,
				   from_m->id, from_m->permission, from_name,
				   to_m->id, to_m->permission, to_name);
	md2pPrintf(md2pMODINFO, " (%d) ", rc);

	return mapDefErrCodes(rc);
}



/* ====================================================================== */
nfsstat3 fh3_addToDirectory(LINK3args * argp, md_auth * auth)
/* ====================================================================== */
{
	int rc, diff;
	mdFhandle D1;
	mdFhandle *dir = &D1;
	mdFhandle D2;
	mdFhandle *dest = &D2;
	char *name = argp->link.name;
	md_dir_item item;
	md_id_t res;

	nfs_fh3_2_mdFhandle(&argp->file, dir);
	nfs_fh3_2_mdFhandle(&argp->link.dir, dest);

	memset((char *) &item, 0, sizeof(item));
	item.ID = dest->id;
	md2pPrintf(md2pMODINFO, "%s %s -> ", mdStringFhandle(*dir), name);
	md2pPrintf(md2pMODINFO, "%s", mdStringID(dest->id));

	if (strlen(name) >= MD_MAX_NAME_LENGTH)
		return NFS3ERR_NAMETOOLONG;

	rc = mdmLookupAuth_0( scl, auth, dir->id, name, &res);
	if (!rc)
		return mapDefErrCodes(MDEexists);

	mdpSetEntryOnly(item.perm);
	rc = mdmAddDirEntry(scl, auth, dir->id, dir->permission, name, &item);
	if (rc) {
		md2pPrintf(md2pMODINFO, " mdmAddDirEntry=%d;", rc);
		return mapDefErrCodes(rc);
	}
	diff = 1;
	rc = mdmModLink(scl, NULL, item.ID, item.perm, &diff);
	if (rc) {
		md2pPrintf(md2pMODINFO, " mdmModLink=%d;", rc);
	} else {
		md2pPrintf(md2pMODINFO, " links=%d;", diff);
	}
	return mapDefErrCodes(rc);
}


/* ====================================================================== */
nfsstat3 fh3_readlink(READLINK3args * args, READLINK3res * res,
					  md_auth * auth)
/* ====================================================================== */
{


	mdFhandle M;
	mdFhandle *m = &M;
	static char linkbuf[NFS_MAXPATHLEN];
	int rc;

	nfs_fh3_2_mdFhandle(&args->symlink, m);

	res->READLINK3res_u.resok.data = linkbuf;
	md2pPrintf(md2pINFO, " %s ", mdStringFhandle(*m));
	rc = mdmGetLink(scl, auth, m->id, m->permission, linkbuf);
	md2pPrintf(md2pINFO, " %s (%d)", linkbuf, rc);
	return mapDefErrCodes(rc);;
}

/*
 * one of those nasty utility routines.
 *  a path is scaned and the routine decides wether 
 *  the path is an internal pnfs path or not.
 *  the key is   /<number>/root/
 *   the return code is negative if the path seems not to be 
 *   internal and the dbid if the path is internal.
 *   dir and base are handles to the dir path and the base name.
 */
int fh3_isPnfsLink(char *path, char **dir, char **base)
{
	char scratch[64], *x, *cur;
	int dbid, state;
	static char spath[1024], *p;

	for (x = path, state = 0, p = spath; *x != 0; x++, p++) {
		*p = *x;
		if (state == 0) {
			if (*x == '/') {
				state++;
				cur = p + 1;
			} else
				break;
		} else if (state == 1) {
			if (*x == '/') {
				*p = '\0';
				sscanf(cur, "%d", &dbid);
				*p = '/';
				state++;
				cur = p + 1;
			} else {
				if (!isdigit(*x))
					break;
			}
		} else if (state == 2) {
			if (*x == '/') {
				*p = '\0';
				if (strcmp(cur, "root"))
					break;
				*p = '/';
				state++;
				cur = p + 1;

			}

		} else if (state == 3) {
			if (*x == '/')
				cur = p + 1;
		}
	}
	if (state == 3) {

		*(cur - 1) = '\0';
		*dir = spath;
		*base = cur;
		*p = '\0';
		return dbid;
	} else
		return -1;

}

/* ====================================================================== */
nfsstat3 fh3_symlink(SYMLINK3args * argp, md_auth * auth)
/* ====================================================================== */
{
	int rc = 0, dbid;
	md_id_t resID;

	mdFhandle From;
	mdFhandle *from_handle = &From;

	char *from_name = argp->where.name;
	char *to_path = argp->symlink.symlink_data;
	md_unix uattr;
	md_dir_item item;
	char *dir, *base;


	nfs_fh3_2_mdFhandle(&argp->where.dir, from_handle);

	fprintf(stderr, "link from %s to %s\n", from_name, to_path);

	md2pPrintf(md2pMODINFO, " dir %s name %s ",
			   mdStringFhandle(*from_handle), from_name);

	if (strlen(from_name) >= MD_MAX_NAME_LENGTH)
		return NFS3ERR_NAMETOOLONG;

	fh3_s2u_attr(&argp->symlink.symlink_attributes, &uattr);

	if ((dbid = fh3_isPnfsLink(to_path, &dir, &base)) < 0) {
		rc = mdmMkLink(scl, auth, from_handle->id, from_handle->permission,
					   from_name, &uattr, to_path, &resID);
	} else {
		char tmp[1024];
		md_dir_item saveItem;
		md2pPrintf(md2pMODINFO, " db=%d;dir=%s;base=%s;", dbid, dir, base);
		rc = mdmGetObjectID(scl, dir, &resID);
		md2pPrintf(md2pMODINFO, "ob=%s(%d);", mdStringID(resID), rc);
		if (rc)
			goto problem;
		rc = mdmRmDirEntry(scl, auth, resID, base, &item);
		md2pPrintf(md2pMODINFO, "rm=%d;", rc);
		if (rc)
			goto problem;
		saveItem = item;
		rc = mdmAddDirEntry(scl, auth,
							from_handle->id,
							from_handle->permission, from_name, &item);

		md2pPrintf(md2pMODINFO, "add=%d;", rc);
		if (rc)
			goto problem;
		rc = mdmChParent(scl, auth, item.ID, from_handle->id);
		md2pPrintf(md2pMODINFO, "cp=%d;", rc);
		if (rc)
			goto problem;
		sprintf(tmp, "/%d/root/links", dbid);
		if (!mdmGetObjectID(scl, tmp, &resID)) {
			strcpy(saveItem.name, mdStringID(saveItem.ID));
			(void) mdmAddDirEntry(scl, auth,
								  resID,
								  saveItem.perm, saveItem.name, &saveItem);

		}
	}
  problem:
	md2pPrintf(md2pMODINFO, " : %s (%d) ", mdStringID(resID), rc);

	return mapDefErrCodes(rc);;

}




/* ====================================================================== */
nfsstat3 fh3_lookup(diropargs3 * dir,
					nfs_fh3 * fh,
					fattr3 * resAttr, fattr3 * dirAttr, md_auth * auth)
/* ====================================================================== */
{
	mdFhandle R, *r = &R;
	mdFhandle M, *m = &M;
	md_dir_item item;
	md_unix resUAttr, dirUAttr;
	mdRecord dRecord;
	int rc;

	nfs_fh3_2_mdFhandle(&(dir->dir), m);

	md2pPrintf(md2pINFO, " dir %s name %s ", mdStringFhandle(*m),
			   dir->name);

	memcpy((char *) r, (char *) m, sizeof(*r));

	rc = mdmGetExtAttrAuth(scl, auth, m->id, m->permission, &dirUAttr, &dRecord);
	md2pPrintf(md2pINFO, " mdmGetExtAttr (%d) ", rc);
	if (rc)
		return mapDefErrCodes(rc);

	if (!strcmp(dir->name, ".")) {
		r->id = m->id;
		r->permission = m->permission;
		resUAttr = dirUAttr;
	} else if (!strcmp(dir->name, "..")) {
		md_id_t first;

		if (mdpIsNoWayBack(m->permission))
			return NFS3ERR_NOENT;
		/* mdSetFirstID( first , 0 ) ; */
		if (mdIsEqualID(m->mountID, m->id)) {

			r->id = m->id;
			r->permission = m->permission;
			resUAttr = dirUAttr;

		} else {

			r->id = dRecord.head.parentID;
			r->permission = m->permission;
			rc = mdmGetExtAttrAuth(scl,auth, dRecord.head.parentID, m->permission,  &resUAttr, NULL);
			md2pPrintf(md2pINFO, " mdmGetExtAttr(res) (%d) ", rc);
			if (rc)
				return mapDefErrCodes(rc);
		}
	} else {
		md_object *object;

		if ((object = md2IsObject(dir->name)) &&
			(md2ObjectType(object) == MDO_REQ_PSET) &&
			(object->argc > 2)) {

			/*        ----------------------             */
			/* touch .(pset)(<pnfsid>)(<commmand>)( ...) */
			/*        ----------------------             */
			if (!strcmp(object->argv[1], "size")) {
				char *c, *d;
				md_id_t id;

				c = md2ObjectName(object);
				md2ScanId(c, &id);
				rc = mdmGetExtAttrAuth(scl, auth, id, m->permission, &resUAttr,   NULL);
				if (rc)
					goto abort;

				if (resUAttr.mst_size == 0) {
					rc = MDEnotFound;
					goto abort;
				}

				r->id = id;
				r->permission = m->permission;
			} else {
				rc = MDEnotFound;
				goto abort;
			}
		} else {
			char *c;
			object = md2IsObject(dir->name);

			if ((object != NULL) &&
				((c = md2ObjectName(object)) != NULL) &&
				(strlen(c) >= MD_MAX_NAME_LENGTH)) {
				return NFS3ERR_NAMETOOLONG;
			}

			if (rc = mdmLookupAuth_1( scl , auth , 
                               m->id , m -> permission ,
                               dir->name , &item , &resUAttr ) ) {
				goto abort;
            }

			r->id = item.ID;
			r->permission = item.perm;
		}
	}
  abort:

	if (!rc) {
		md2pPrintf(md2pINFO, " : %s ", mdStringFhandle(*r));
		fh3_u2f_attr(&dirUAttr, dirAttr);
		fh3_u2f_attr(&resUAttr, resAttr);
		mdFhandle_2_nfs_fh3(fh, r);
		md2pPrintf(md2pINFO, " t=%d;m=%o;i=%lx; ",
				   resAttr->type, resAttr->mode, resAttr->fileid);
	} else {
		fh3_u2f_attr(&dirUAttr, dirAttr);
	}
	md2pPrintf(md2pINFO, " (%d) ", rc);

	return mapDefErrCodes(rc);

}

#ifdef xxx
/* ====================================================================== */
int fh_canShow(nfs_fh * fh, md_dir_item * item, md_auth * auth)
/* ====================================================================== */
{
	mdFhandle *m = (mdFhandle *) fh;
	if (mdpIsPrivileged(m->permission))
		return 1;
	return !mdpIsDirInvisible(item->perm, mdpGetLevel(m->permission));
}

/* ====================================================================== */
int fh_mapHandle(md_permission * p, md_permission perm, md_auth * auth)
/* ====================================================================== */
{

	if ((!mdpIsPrivileged(*p)) && mdpIsLupInvisible(perm, mdpGetLevel(*p)))
		return -1;

	mdpDoModification(*p, perm, mdpLevelBits, mdpModifyLevelBit);
	mdpDoModification(*p, perm, mdpNoIOBit, mdpModifyNoIOBit);
	mdpDoModification(*p, perm, mdpNoWayBackBit, mdpModifyNoWayBackBit);

	p->high = perm.high;
	return 0;
}
#endif
static struct {
	reqExtItem ext_item[RD_READ_AHEAD];
	int valid, cursor;
	md_auth auth;
	md_id_t id;
	md_permission perm;
	md_long lastCookie;
} fh_dir_mind;
char *stringHeader(md_auth * a);
/* ====================================================================== */
nfsstat3 fh3_opendir(nfs_fh3 * fh, long *cookie, md_auth * auth)
/* ====================================================================== */
{

	mdFhandle M, *m = &M;
	int rc;

	nfs_fh3_2_mdFhandle(fh, m);

	rc = mdmReadDirAuth(scl, auth, m->id, m->permission,
						*cookie, RD_READ_AHEAD, fh_dir_mind.ext_item);

	fh_dir_mind.valid = rc;
	fh_dir_mind.cursor = 0;

	md2pPrintf(md2pINFO, " open (%d) ", rc);
	if (rc < 0)
		return mapDefErrCodes(rc);
	fh_dir_mind.auth = *auth;
	fh_dir_mind.id = m->id;
	fh_dir_mind.perm = m->permission;
	fh_dir_mind.lastCookie = *cookie;

	return 0;

}

/* ====================================================================== */
nfsstat3 fh3_readdir(long *cookie, md_dir_item * item)
/* ====================================================================== */
{
	int rc;

	if (fh_dir_mind.valid > 0) {
		*item = fh_dir_mind.ext_item[fh_dir_mind.cursor].item;
		*cookie = fh_dir_mind.ext_item[fh_dir_mind.cursor++].cookie;
		fh_dir_mind.lastCookie = *cookie;
		fh_dir_mind.valid--;
		return 0;
	} else if (fh_dir_mind.valid == 0) {
		rc = mdmReadDirAuth(scl, &fh_dir_mind.auth, fh_dir_mind.id,
							fh_dir_mind.perm, fh_dir_mind.lastCookie,
							RD_READ_AHEAD, fh_dir_mind.ext_item);
		fh_dir_mind.valid = rc;
		fh_dir_mind.cursor = 0;

		md2pPrintf(md2pINFO, " readdir (%d) ", rc);
		if (rc == 0)
			return 1;
		if (rc < 0)
			return mapDefErrCodes(rc);
		return fh3_readdir(cookie, item);
	} else
		return mapDefErrCodes(MDEnotDirectory);
}

/* ====================================================================== */
nfsstat3 fh3_closedir()
/* ====================================================================== */
{
	return 0;
}

int mallocfailed()
{
	exit(1);
}
int fd_idle(int fd)
{

	return 0;
}

int pseudo_inode(u_long inode, u_short dev)
{
	return 0;

}

/* ====================================================================== */
char *stringAuth(md_auth * a)
/* ====================================================================== */
{
	static char string[128];

#ifdef _LITTLE_ENDIAN
	sprintf(string, "%d.%d.%d.%d-%d-%d",
			a->host & 0xFF, (a->host >> 8) & 0xFF,
			(a->host >> 16) & 0xFF, a->host >> 24, a->uid, a->gid);
#else
	sprintf(string, "%d.%d.%d.%d-%d-%d",
			a->host >> 24, (a->host >> 16) & 0xFF,
			(a->host >> 8) & 0xFF, a->host & 0xFF, a->uid, a->gid);
#endif
	return 0;
}

/* ====================================================================== */
char *stringHeader(md_auth * a)
/* ====================================================================== */
{
	static char string[1024];
	struct tm *tms;
	int rc;
	time_t clock;

	time(&clock);
	tms = localtime(&clock);
	rc = strftime(string, 128, " %D %T ", tms);
	if (rc <= 0) {
		string[0] = '\0';
		return string;
	}

	if (a == NULL) {
		sprintf(string + rc, "0.0.0.0-0-0");
	} else {
#ifdef _LITTLE_ENDIAN
		sprintf(string + rc, "%d.%d.%d.%d-%d-%d",
				a->host & 0xFF, (a->host >> 8) & 0xFF,
				(a->host >> 16) & 0xFF, a->host >> 24, a->uid, a->gid);
#else
		sprintf(string + rc, "%d.%d.%d.%d-%d-%d",
				a->host >> 24, (a->host >> 16) & 0xFF,
				(a->host >> 8) & 0xFF, a->host & 0xFF, a->uid, a->gid);
#endif
		if (a->gidsLen > 0) {
			char *cursor = string + strlen(string);
			int i;
			sprintf(cursor, "(");
			cursor += strlen(cursor);
			for (i = 0; i < a->gidsLen; i++) {
				sprintf(cursor, "%d,", a->gids[i]);
				cursor += strlen(cursor);
			}
			sprintf(cursor, ")");

		}
	}

	return string;
}


#ifdef very_new
/* ====================================================================== */
nfsstat fh_getattrLow(mdFhandle * m, fattr * attr)
/* ====================================================================== */
{
	md_unix sattr, *s = &sattr;
	int rc;

	rc = mdmGetAttr(scl, m->id, m->permission, s);
	if (rc)
		return NFSERR_NOENT;

	attr->type = S_ISDIR(s->mst_mode) ? NFDIR :
		S_ISREG(s->mst_mode) ? NFREG :
		S_ISLNK(s->mst_mode) ? NFLNK : NFNON;
	attr->mode = s->mst_mode;
	attr->nlink = s->mst_nlink;
	attr->uid = s->mst_uid;
	attr->gid = s->mst_gid;
	attr->size = s->mst_size;
	attr->blocksize = s->mst_blksize;
	attr->rdev = s->mst_rdev;
	attr->blocks = s->mst_blocks;
	attr->fsid = 1;
	attr->fileid = s->mst_ino;
	attr->atime.seconds = s->mst_atime;
	attr->mtime.seconds = s->mst_mtime;
	attr->ctime.seconds = s->mst_ctime;

	return NFS_OK;
}



/* ====================================================================== */
nfsstat fh_nfs_commands(mdFhandle * dir, char *name, char *command)
/* ====================================================================== */
{
	char cmd[1024], *argv[64], *c;
	int argc, i;

	strcpy(cmd, command);
	for (c = cmd, i = 0, argc = 0, argv[0] = c;
		 (i < 1024) && (argc < 64) && (*c != '\0'); i++, c++) {

		if (*c == '/') {
			*c = '\0';
			argc++;
			argv[argc] = c + 1;
		}
	}
	if (*c != '\0')
		return NFSERR_NAMETOOLONG;
	if (strlen(argv[argc]) > 0)
		argc++;
	/* ------------------------------------------------------------ */

	if (argc < 3)
		return NFSERR_ACCES;

	return fh_do_commands(dir, name, argc, argv);
}

/* ====================================================================== */
nfsstat fh_do_commands(mdFhandle * dir, char *name, int argc, char *argv[])
/* ====================================================================== */
{
	md_permission perm;
	md_id_t id;
	if (!strcmp(argv[2], "permission")) {
		if (argc != 4)
			return NFSERR_ACCES;
		sscanf(argv[3], "%x", &perm.low);
		perm.high = 0;
		return NFS_OK;
	} else if (!strcmp(argv[2], "addtodirectory")) {
		if (argc != 4)
			return NFSERR_ACCES;
		sscanf(argv[3], "%x", &id.low);
		id.high = 0;
		return NFS_OK;
	} else if (!strcmp(argv[2], "removefromdirectory")) {
		if (argc != 3)
			return NFSERR_ACCES;
		return NFS_OK;
	} else
		return NFSERR_PERM;
}
#endif
