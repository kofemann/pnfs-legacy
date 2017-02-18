/*
 *  $Id: db_oracle-hash.c,v 1.14 2005-10-31 17:26:00 tigran Exp $
 */

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
#include <sys/types.h>
/* #include <sys/stat.h>  */
#include <string.h>
#include <stdlib.h>

#include "dbglue.h"
#include <oci.h>

/* keep Fermi happy */
static const char defaultConnectString[] = "user=enstore";

static const char dbUser[] = "pnfs";    /* DB user */
static const char dbPass[] = "pnfs";    /* DB pass */
static const char dbID[] = "odysseus";  /* SSID */
static char *tbName = NULL;



#define CACHE 1                 /* define this to enable cache code */
#define CACHESIZE 400           /* number of records to cache */
#define RECSIZE 1012            /* max size of DB record */
#define HASHVSIZE 256           /* vertical size of HASH */
#define HASHAND 0xff            /* to generate index from 0 to 255 */


/* General FIFO manipulation macros */
#define ADDRING(head, tail, obj)                                        \
        {                                                               \
                if ((head) == NULL) {                                   \
                        (head) = (tail) = (obj);                        \
                        (obj)->prev = (obj)->next = NULL;               \
                } else {                                                \
                        (tail)->next = (obj);                           \
                        (obj)->prev = (tail);                           \
                        (obj)->next = NULL;                             \
                        (tail) = (obj);                                 \
                }                                                       \
        }
#define ADDERING(head, tail, obj)                                       \
{                                                                       \
  if ((head) == NULL) {                                                 \
    (head) = (tail) = (obj);                                            \
    (obj)->prev = (obj)->next = NULL;                                   \
  } else {                                                              \
    (head)->prev = (obj);                                               \
    (obj)->next = (head);                                               \
    (obj)->prev = NULL;                                                 \
    (head) = (obj);                                                     \
  }                                                                     \
}

#define DELRING(head, tail, obj)                                        \
        {                                                               \
                if ((obj) == (head)) {                                  \
                        if (((head) = (head)->next) != NULL)            \
                                (head)->prev = NULL;                    \
                        else                                            \
                                (tail) = NULL;                          \
                } else if ((obj) == (tail)) {                           \
                        if (((tail) = (tail)->prev) != NULL)            \
                                (tail)->next = NULL;                    \
                } else {                                                \
                        (obj)->prev->next = (obj)->next;                \
                        if ((obj)->next != NULL)                        \
                                (obj)->next->prev = (obj)->prev;        \
                }                                                       \
        }

#define INVALKEY(c) ((c)->lkey = (c)->lkey0 = (c)->lkey1 = 0)

/* #define CDEB 1    /* for debug output */
/* #define STATS 1   /* for cache hit statistics */
/* PSCI (Pnfs Simple Cache Implementation !!!!)  */

/* structure used in Hash ring buffers */
typedef struct HCache {
    /* the key we are looking for */
    unsigned long lkey;
    unsigned long lkey0;
    unsigned long lkey1;
    /* int pageIndex; */
    char *dataPtr;              /* ptr to gdbm's record */
    int dataSize;
    struct CCache *cc;          /* link back into usage queue */
    struct Hash *vh;            /* link back into Hash vertical table */
    struct HCache *prev;
    struct HCache *next;
} HCache;

/* vertical structure of Hash */
typedef struct Hash {
    struct HCache *head;
    struct HCache *tail;
} Hash;

typedef struct CCache {         /* sort pages according to last use */
    struct HCache *h;           /* link to HCache linked list */
    struct CCache *next;
    struct CCache *prev;
} CCache;

typedef struct cKey {
    unsigned long l;
    unsigned long l0;
    unsigned long l1;
} cKey;

#if 0
typedef struct CPage {
    double alignementDummy;
    unsigned char value[RECSIZE];
} CPage;
#endif

static CCache *ccHead = (CCache *) 0;
static CCache *ccTail = (CCache *) 0;
/* static CPage cPages[CACHESIZE];        /* that's the big chunk */

static Hash hash[HASHVSIZE];    /* vertical hash list */
/* corners for not used horizontal Hash entries */
static HCache *emptyHead = (HCache *) 0;
static HCache *emptyTail = (HCache *) 0;

#ifdef STATS
static unsigned long stats[RECSIZE + 2];
static unsigned long statsCounter = 0;
static unsigned long cachHits = 0;
static unsigned long cachMiss = 0;
#endif

typedef struct bla {
    unsigned short s1;
    unsigned short s2;
    unsigned long l1;
    unsigned long l2;
} bla;

static char *id2string(mdlDatum key);
static void string2id(const char *id, char *ptr);

static OCIEnv *p_env;
static OCIError *p_err;
static OCISvcCtx *p_svc;
static OCIStmt *p_sql;
static OCIStmt *p_cursor;
static OCIDefine *p_dfn = (OCIDefine *) 0;
static OCIBind *p_bnd = (OCIBind *) 0;
static OCILobLocator *p_blob;
static OCILobLocator *p_blobget;
static char p_sli[25];


static int isWriteLock = 0;


static char *printKey(bla * b)
{
    static char s[200];
    (void) sprintf(s, "%04x%04x%08x%08x", b->s1, b->s2, b->l1, b->l2);
    return (s);
}

int mdxInit(char *(*getenv) (const char *name),
            int (*prt) (int, char *, ...))
{
    return 0;
}

static void removeCache()
{
    int i;
    CCache *co, *c = ccHead;

    while (c) {
        co = c;
        c = c->next;
        if (co->h->dataSize)
            (void) free(co->h->dataPtr);
        (void) free(co->h);
        (void) free(co);
    }
    ccHead = ccTail = (CCache *) 0;
    for (i = 0; i < HASHVSIZE; i++)
        hash[i].head = hash[i].tail = (HCache *) 0;
    emptyHead = emptyTail = (HCache *) 0;
}

#ifdef STATS
void printStats()
{
    FILE *f;
    int i;

    if ((f = fopen("/tmp/gdbm.stats", "w")) == NULL) {
        fprintf(stderr, "can't open statistic file\n");
        return;
    }
    fprintf(f, "# Hits: %u  Misses: %u\n", cachHits, cachMiss);
    for (i = 0; i <= RECSIZE; i++) {
        fprintf(f, "%d %u\n", i, stats[i]);
    }
    (void) fclose(f);
}
#endif

static int initCache()
{
    int i;
    CCache *c;
    HCache *h;

#ifdef STATS
    memset((char *) stats, 0, RECSIZE + 2);
#endif
    if (ccHead)                 /* already done */
        return (0);
    for (i = 0; i < CACHESIZE; i++) {
        if ((c = (CCache *) malloc(sizeof(CCache))) == NULL) {
            fprintf(stderr, "initCache: out of memory\n");
            removeCache();
            return (-1);
        }
        if ((h = (HCache *) malloc(sizeof(HCache))) == NULL) {
            fprintf(stderr, "initCache: out of memory\n");
            removeCache();
            return (-1);
        }
        h->cc = c;
        h->vh = (Hash *) 0;
        /* h->pageIndex = i; */
        h->dataPtr = (char *) 0;
        h->dataSize = 0;
        /* INVALKEY(h); */

        c->h = h;
        ADDRING(emptyHead, emptyTail, h);
        ADDRING(ccHead, ccTail, c);
    }
    for (i = 0; i < HASHVSIZE; i++) {
        hash[i].head = hash[i].tail = (HCache *) 0;
    }
    return (0);
}

static HCache *locateCache(cKey * k)
{
    Hash *hv = &hash[(k->l1 >> 3) & HASHAND];
    HCache *hc = hv->head;

    while (hc) {
        if (hc->lkey1 == k->l1 && hc->lkey0 == k->l0 && hc->lkey == k->l)
            return (hc);
        hc = hc->next;
    }
    return ((HCache *) 0);
}

static int fetchCache(mdlDatum * k, mdlDatum * d)
{
    HCache *h;

    if ((h = locateCache((cKey *) k->dptr))) {
#if 0
        d->dsize = RECSIZE;
        d->dptr = cPages[h->pageIndex].value;
#endif
        d->dptr = h->dataPtr;
        d->dsize = h->dataSize;

        DELRING(ccHead, ccTail, h->cc); /* move to newest */
        ADDRING(ccHead, ccTail, h->cc);
        return (0);
    }
    return (-1);
}

static void deleteCache(mdlDatum * k)
{
    HCache *h;

    if ((h = locateCache((cKey *) k->dptr))) {
        if (h->vh) {
            DELRING(h->vh->head, h->vh->tail, h);
            ADDRING(emptyHead, emptyTail, h);
        }
        if (h->dataSize && h->dataPtr)
            (void) free(h->dataPtr);
        h->dataSize = 0;
        h->vh = (Hash *) 0;
        DELRING(ccHead, ccTail, h->cc); /* move to oldest */
        ADDERING(ccHead, ccTail, h->cc);
    }
}


static void checkerr(OCIError * errhp, sword status)
{

    text errbuf[512];
    ub4 errcode;
    
    switch (status) {
    case OCI_SUCCESS:
        break;
    case OCI_SUCCESS_WITH_INFO:
        fprintf(stderr, "Error - OCI_SUCCESS_WITH_INFO\n");
        break;
    case OCI_NEED_DATA:
        fprintf(stderr, "Error - OCI_NEED_DATA\n");
        break;
    case OCI_NO_DATA:
        fprintf(stderr, "Error - OCI_NO_DATA\n");
        break;
    case OCI_ERROR:
        OCIErrorGet((dvoid *) errhp, (ub4) 1, (text *) NULL,
                    (long *) &errcode, errbuf, (ub4) sizeof(errbuf),
                    (ub4) OCI_HTYPE_ERROR);

        fprintf(stderr, "Error - %s\n", errbuf);
        exit(0);
        break;
    case OCI_INVALID_HANDLE:
        fprintf(stderr, "Error - OCI_INVALID_HANDLE\n");
        exit(0);
        break;
    case OCI_STILL_EXECUTING:
        fprintf(stderr, "Error - OCI_STILL_EXECUTE\n");
        break;
    case OCI_CONTINUE:
        fprintf(stderr, "Error - OCI_CONTINUE\n");
        break;
    default:
        break;

    }

}

mdlDatum mdxFirst(MDX_FILE conn)
{
    mdlDatum val;

    int rc;
    char errbuf[100];
    int errcode;

   char query[256];

    val.dptr = NULL;
    val.dsize = 0;
    val.flags = 0;
        
    sprintf(query, "SELECT pnfsid FROM t_%s", tbName);

    rc = OCIHandleAlloc((dvoid *) p_env, (dvoid **) & p_cursor,
                        OCI_HTYPE_STMT, (size_t) 0, (dvoid **) 0);

    /* Start a transaction block */

    rc = OCIStmtPrepare(p_cursor, p_err, query,
                        (ub4) strlen((char *) query), (ub4) OCI_NTV_SYNTAX,
                        (ub4) OCI_DEFAULT);

    rc = OCIDefineByPos(p_cursor, &p_dfn, p_err, 1, (dvoid *) & p_sli,
                        (sword) 25, SQLT_STR, (dvoid *) 0, (ub2 *) 0,
                        (ub2 *) 0, OCI_DEFAULT);

    rc = OCIStmtExecute(p_svc, p_cursor, p_err, (ub4) 0, (ub4) 0,
                        (CONST OCISnapshot *) NULL, (OCISnapshot *) NULL,
                        OCI_STMT_SCROLLABLE_READONLY);

    if (rc != 0) {
        OCIErrorGet((dvoid *) p_err, (ub4) 1, (text *) NULL, &errcode,
                    errbuf, (ub4) sizeof(errbuf), OCI_HTYPE_ERROR);
        fprintf(stderr, "Error in mdxFirst (get cursor) - %.*s\n", 512,
                errbuf);
        return val;
    }

    /* Fetch row from pnfs */
    rc = OCIStmtFetch2(p_cursor, p_err, (ub4) 1, OCI_FETCH_FIRST, (sb4) 0,
                       OCI_DEFAULT);

    if (rc != 0) {
        OCIErrorGet((dvoid *) p_err, (ub4) 1, (text *) NULL, &errcode,
                    errbuf, (ub4) sizeof(errbuf), OCI_HTYPE_ERROR);
        fprintf(stderr, "Error in mdxFirst (FETCH FIRST) - %.*s\n", 512,
                errbuf);
        (void) OCIHandleFree((dvoid *) p_cursor, OCI_HTYPE_STMT);
        return val;
    }


    val.dptr = (char *) malloc(12);     /* Allocate memory for result */
    string2id(p_sli, val.dptr);

    val.dsize = 12;             /* Stopy the result length    */
    val.flags = MDX_FREE;       /* ??? */

    return val;
}

mdlDatum mdxNext(MDX_FILE conn, mdlDatum key)
{
    mdlDatum val;

    int rc;
    char errbuf[100];
    int errcode;

    val.dptr = NULL;
    val.dsize = 0;
    val.flags = 0;

    /* Fetch row from pnfs */
    rc = OCIStmtFetch2(p_cursor, p_err, (ub4) 1, (ub2) OCI_FETCH_NEXT,
                       (sb4) 0, OCI_DEFAULT);

    if (rc != 0) {
        if (rc != OCI_NO_DATA) {
            OCIErrorGet((dvoid *) p_err, (ub4) 1, (text *) NULL, &errcode,
                        errbuf, (ub4) sizeof(errbuf), OCI_HTYPE_ERROR);
            fprintf(stderr, "Error in mdxNext (FETCH NEXT) - %.*s\n", 512,
                    errbuf);
        }
        (void) OCIHandleFree((dvoid *) p_cursor, OCI_HTYPE_STMT);
        return val;
    }

    val.dptr = (char *) malloc(12);     /* Allocate memory for result */
    string2id(p_sli, val.dptr);

    val.dsize = 12;             /* Stopy the result length    */
    val.flags = MDX_FREE;       /* ??? */


    return val;
}

void mdxBreak(MDX_FILE conn)
{
    (void) OCIStmtFetch2(p_cursor, p_err, (ub4) 0, OCI_FETCH_NEXT, (sb4) 0,
                         OCI_DEFAULT);
    (void) OCIHandleFree((dvoid *) p_cursor, OCI_HTYPE_STMT);
}

int mdxScan(MDX_FILE mdx)
{
    int i;
    mdlDatum key;

    key = mdxFirst(mdx);
    while (key.dptr) {
        for (i = 0; i < 8; i++)
            fprintf(stderr, "%2.2x", key.dptr[i]);
        fprintf(stderr, "\n");
        if (key.dptr)
            free(key.dptr);
        key = mdxNext(mdx, key);
    };

    return 0;
}

MDX_FILE mdxOpen(char *name, int flags, int mode)
{
    char query[128];
    char *dbName;
	static int ociInit;

    int rc;
    char errbuf[100];
    int errcode;

    /* Make a connection to the database */
    dbName = rindex(name, '/');
    if (dbName == NULL) {
        dbName = name;
    } else {
        ++dbName;
    }

	if( ! ociInit  ) {
		rc = OCIEnvCreate(&p_env, OCI_THREADED|OCI_OBJECT, (dvoid *)0, 
                               0, 0, 0, (size_t) 0, (dvoid **)0);
        ++ociInit;
	}

    /* Initialize handles */
    rc = OCIHandleAlloc((dvoid *) p_env, (dvoid **) & p_err,
                        OCI_HTYPE_ERROR, (size_t) 0, (dvoid **) 0);
    rc = OCIHandleAlloc((dvoid *) p_env, (dvoid **) & p_svc,
                        OCI_HTYPE_SVCCTX, (size_t) 0, (dvoid **) 0);


    /* Connect to database server */
    rc = OCILogon(p_env, p_err, &p_svc, dbUser, strlen(dbUser), dbPass,
                  strlen(dbPass), dbID, strlen(dbID));
                  
    if (rc != 0) {
        OCIErrorGet((dvoid *) p_err, (ub4) 1, (text *) NULL, &errcode,
                    errbuf, (ub4) sizeof(errbuf), OCI_HTYPE_ERROR);
        fprintf(stderr, "Error mdxOpen - %.*s\n", 512, errbuf);
        return (MDX_FILE) NULL;
    }

    rc = OCIHandleAlloc((dvoid *) p_env, (dvoid **) & p_sql,
                        OCI_HTYPE_STMT, (size_t) 0, (dvoid **) 0);

	/* remember table name */
	tbName = strdup(dbName);

    if (initCache() == 0) {
        return (MDX_FILE) p_svc;
    }

    mdxClose((MDX_FILE) p_svc);

    return (MDX_FILE) NULL;
}


MDX_FILE mdxCreate(char *name, int flags, int mode)
{
	
	MDX_FILE f = NULL;
	char query[256];
    int rc;
    char errbuf[100];
    int errcode;
	
	
    if ( mdxOpen(name, flags, mode) != NULL ) {
    
	    /* change default user schema to actual one */
	    sprintf(query, "CREATE TABLE t_%s (pnfsid VARCHAR(24) PRIMARY KEY, data BLOB NOT NULL)", tbName);
	
	
	    rc = OCIStmtPrepare(p_sql, p_err, query, (ub4) strlen((char *) query),
	                        (ub4) OCI_NTV_SYNTAX, (ub4) OCI_DEFAULT);
	
	
	    rc = OCIStmtExecute(p_svc, p_sql, p_err, (ub4) 1, (ub4) 0,
	                        (CONST OCISnapshot *) 0, (OCISnapshot *) 0,
	                        (ub4) OCI_COMMIT_ON_SUCCESS);
	
	    if (rc != 0) {
	        OCIErrorGet((dvoid *) p_err, (ub4) 1, (text *) NULL, &errcode,
	                    errbuf, (ub4) sizeof(errbuf), OCI_HTYPE_ERROR);
	        fprintf(stderr, "Error mxdCreate - %.*s\n", 512, errbuf);
	   
		    mdxClose((MDX_FILE) p_svc);	
	    }
    
    	f = (MDX_FILE)p_svc;
    }
    
    return f;
}

int mdxCheck(char *name)
{
    return 0;
}

int mdxFetch(MDX_FILE conn, mdlDatum key, mdlDatum *val)
{
    int rc;
    char errbuf[100];
    int errcode;

    char query[256];

    char *pnfsid;
    ub4 amnt = 0;

    val->dptr = NULL;
    val->dsize = 0;
    val->flags = 0;


    if (fetchCache(&key, val) == 0) {
#ifdef CDEB
        fprintf(stderr, "FETCH '%s' IN CACHE\n",
                printKey((bla *) key.dptr));
#endif
#ifdef STATS
        cachHits++;
#endif
        val->flags = 0;
        return 0;
    } else {
        HCache *h;
        /* CPage *cp; */
        Hash *vh;
        CCache *c;
        unsigned long *p = (unsigned long *) key.dptr;

#ifdef STATS
        cachMiss++;
#endif
        /* allocate new Cache page */
        c = ccHead;
        h = c->h;
        if (h->dataSize && h->dataPtr) {
            (void) free(h->dataPtr);
            h->dataSize = 0;
        }
        DELRING(ccHead, ccTail, c);
        if (h->vh) {
            DELRING(h->vh->head, h->vh->tail, h);
        } else {
            DELRING(emptyHead, emptyTail, h);
        }
        h->vh = (Hash *) 0;


		sprintf(query, "SELECT data FROM t_%s WHERE pnfsid = :x", tbName);

        /* Fetch row from pnfs */
        pnfsid = id2string(key);


       rc = OCIDescriptorAlloc(p_env, (void **) &p_blobget, OCI_DTYPE_LOB,
                            (size_t) 0, (dvoid **) 0);


        rc = OCIStmtPrepare(p_sql, p_err, query,
                            (ub4) strlen((char *) query), OCI_NTV_SYNTAX,
                            OCI_DEFAULT);

        rc = OCIBindByName(p_sql, &p_bnd, p_err, (text *) ":x", -1,
                           (dvoid *) pnfsid, strlen(pnfsid) + 1, SQLT_STR,
                           (dvoid *) 0, (ub2 *) 0, (ub2 *) 0, (ub4) 0,
                           (ub4 *) 0, OCI_DEFAULT);


        rc = OCIDefineByPos(p_sql, &p_dfn, p_err, 1, (dvoid *) & p_blobget,
                            (sb4) - 1, (ub2) SQLT_BLOB, 0, 0, 0,
                            OCI_DEFAULT);


        rc = OCIStmtExecute(p_svc, p_sql, p_err, (ub4) 1, (ub4) 0,
                            (OCISnapshot *) NULL, (OCISnapshot *) NULL,
                            (ub4) OCI_DEFAULT);                                                        

        if (rc != 0) {        	
            OCIErrorGet((dvoid *) p_err, (ub4) 1, (text *) NULL, &errcode,
                        errbuf, (ub4) sizeof(errbuf), OCI_HTYPE_ERROR);
            fprintf(stderr, "Error in mdxFetch (select blob, pnfsid = %s) - %.*s\n", pnfsid,
                    512, errbuf);
            free(pnfsid);
            OCIDescriptorFree ( p_blobget,   OCI_DTYPE_LOB );
            goto bad;
        }

        rc = OCILobGetLength(p_svc, p_err, p_blobget, (ub4 *) &val->dsize);
        if (rc != 0) {        	
            OCIErrorGet((dvoid *) p_err, (ub4) 1, (text *) NULL, &errcode,
                        errbuf, (ub4) sizeof(errbuf), OCI_HTYPE_ERROR);
            fprintf(stderr, "Error in mdxFetch (get blobl size) - %.*s\n",
                    512, errbuf);
            free(pnfsid);
            OCIDescriptorFree ( p_blobget,   OCI_DTYPE_LOB );
            goto bad;
        }

        val->dptr = malloc(val->dsize);
        memset(val->dptr, 0, val->dsize);
		amnt = val->dsize;
		
        rc = OCILobRead(p_svc, p_err, p_blobget, &amnt, (ub4) 1, val->dptr,
                        val->dsize, (dvoid *) 0, NULL, (ub2) 0,
                        (ub1) SQLCS_IMPLICIT);

        if (rc != 0) {        	
            OCIErrorGet((dvoid *) p_err, (ub4) 1, (text *) NULL, &errcode,
                        errbuf, (ub4) sizeof(errbuf), OCI_HTYPE_ERROR);
            fprintf(stderr, "Error in mdxFetch (read blob) - %.*s\n",
                    512, errbuf);
            free(pnfsid);
            OCIDescriptorFree ( p_blobget,   OCI_DTYPE_LOB );
            goto bad;
        }
#ifdef CDEB
        fprintf(stderr, "FETCH '%s' (%d) NOT IN CACHE\n",
                printKey((bla *) key.dptr), vIndex);
#endif
        h->dataPtr = val->dptr;
        h->dataSize = val->dsize;
        val->flags = 0;
        
        h->lkey = p[0];
        h->lkey0 = p[1];
        h->lkey1 = p[2];
        /* include HCache into vertical Hash list */
        vh = &hash[(h->lkey1 >> 3) & HASHAND];
        h->vh = vh;
        ADDRING(vh->head, vh->tail, h);
        ADDRING(ccHead, ccTail, c);     /* move to newest */
        OCIDescriptorFree ( p_blobget,   OCI_DTYPE_LOB );        
        free(pnfsid);
        return 0;
      bad:

        /* put zombies back into list */
        ADDERING(ccHead, ccTail, c);
        ADDRING(emptyHead, emptyTail, h);
    }

	if( val->dptr != NULL ) {		
		free( val->dptr);
    	val->dptr = NULL;
	}
    val->dsize = 0;
    val->flags = 0;
    return 0;
}


int mdxStore(MDX_FILE conn, mdlDatum key, mdlDatum val)
{
    HCache *h;
    CCache *c;
    Hash *vh;
    unsigned long *p = (unsigned long *) key.dptr;
    char query[128];
    char *pnfsid;
    int rc;
    
    char errbuf[100];
    int errcode;
    int amnt;
    
    int isExist = 1; /* fake: let say record always exist :) */
    
#ifdef CDEB
    fprintf(stderr, "STORE '%s' '%x' (%d)\n",
            printKey((bla *) key.dptr), p[2], val.dsize);
#endif
    if ((h = locateCache((cKey *) key.dptr)) == (HCache *) 0) {
#ifdef CDEB
        fprintf(stderr, "NOT IN CACHE ");
#endif
        /* grab new Hash entry here */
        c = ccHead;             /* grab oldest */
        h = c->h;
        /* override with new key data */
        h->lkey = p[0];
        h->lkey0 = p[1];
        h->lkey1 = p[2];
    } else {                    /* found in active Cache list */
        c = h->cc;
#ifdef CDEB
        fprintf(stderr, "IN CACHE ");
#endif
    }

    /* check if entry was inuse before */
    if (h->vh) {
        DELRING(h->vh->head, h->vh->tail, h);
    } else {
        DELRING(emptyHead, emptyTail, h);
    }
    h->vh = (Hash *) 0;         /* mark as NOT IN USE */
    /* temp. remove from usage list */
    DELRING(ccHead, ccTail, c);
    if (h->dataPtr && h->dataSize) {
        if (val.dsize > h->dataSize) {
            (void) free(h->dataPtr);
            h->dataSize = 0;
            if ((h->dataPtr = (char *) malloc(RECSIZE)) == NULL)
                goto bad;
            h->dataSize = RECSIZE;
        }
    } else {
        if ((h->dataPtr = (char *) malloc(RECSIZE)) == NULL)
            goto bad;
        h->dataSize = RECSIZE;
    }
    memset(h->dataPtr, 0, h->dataSize);
    memcpy(h->dataPtr, val.dptr, val.dsize);
#if 0
    cp = &cPages[h->pageIndex];
    memset((char *) cp, 0, sizeof(CPage));
    memcpy((char *) cp->value, val.dptr, val.dsize);
#endif

    pnfsid = id2string(key);

    /* Start a transaction block */       

insert:	

	if ( ! isExist ) {

	    sprintf(query, "INSERT INTO t_%s VALUES ( :x , EMPTY_BLOB() )", tbName);
	    rc = OCIStmtPrepare(p_sql, p_err, query,
	                        (ub4) strlen((char *) query),
	                        (ub4) OCI_NTV_SYNTAX, (ub4) OCI_DEFAULT);
	
	    rc = OCIBindByName(p_sql, &p_bnd, p_err, (text *) ":x", -1,
	                       (dvoid *) pnfsid, strlen(pnfsid) + 1,
	                       SQLT_STR, (dvoid *) 0, (ub2 *) 0,
	                       (ub2 *) 0, (ub4) 0, (ub4 *) 0, OCI_DEFAULT);
	
	    rc = OCIStmtExecute(p_svc, p_sql, p_err, (ub4) 1, (ub4) 0,
	                        (CONST OCISnapshot *) NULL,
	                        (OCISnapshot *) NULL, (ub4) OCI_DEFAULT);
	
	    if (rc != 0) {
	        OCIErrorGet((dvoid *) p_err, (ub4) 1, (text *) NULL,
	                    &errcode, errbuf, (ub4) sizeof(errbuf),
	                    OCI_HTYPE_ERROR);
	        fprintf(stderr,
	                "Error in mdxStore (insert new blob) - %.*s\n",
	                512, errbuf);
	                free(pnfsid);
	        goto bad;
	    }

	}

    sprintf(query, "SELECT data FROM t_%s WHERE pnfsid = :x FOR UPDATE", tbName);
    
    rc = OCIDescriptorAlloc(p_env, (void **) &p_blob, OCI_DTYPE_LOB,
                            (size_t) 0, (dvoid **) 0);
    
    
    rc = OCIStmtPrepare(p_sql, p_err, query,
                        (ub4) strlen((char *) query),
                        (ub4) OCI_NTV_SYNTAX, (ub4) OCI_DEFAULT);
    rc = OCIBindByName(p_sql, &p_bnd, p_err, (text *) ":x", -1,
                       (dvoid *) pnfsid, strlen(pnfsid) + 1,
                       SQLT_STR, (dvoid *) 0, (ub2 *) 0,
                       (ub2 *) 0, (ub4) 0, (ub4 *) 0, OCI_DEFAULT);
    (void) OCIDefineByPos(p_sql, &p_dfn, p_err, 1,
                          (dvoid *) & p_blob, (sb4) - 1,
                          (ub2) SQLT_BLOB, 0, 0, 0, OCI_DEFAULT);
    rc = OCIStmtExecute(p_svc, p_sql, p_err, (ub4) 1, (ub4) 0,
                        (CONST OCISnapshot *) 0,
                        (OCISnapshot *) 0, (ub4) OCI_DEFAULT);
                                                
    if (rc != 0) {
    	/* if record do not exist, insert it ! */
    	if( rc == OCI_NO_DATA ) {
    		isExist = 0;
    		goto insert;
    	}
    	
        OCIErrorGet((dvoid *) p_err, (ub4) 1, (text *) NULL,
                    &errcode, errbuf, (ub4) sizeof(errbuf),
                    OCI_HTYPE_ERROR);
        fprintf(stderr,
                "Error in mdxStore (select for update) - %.*s\n",
                512, errbuf);
        OCIDescriptorFree ( p_blob,   OCI_DTYPE_LOB );                
        free(pnfsid);
        goto bad;
    }

	amnt = val.dsize;
    rc = OCILobWrite(p_svc, p_err, p_blob, &amnt, (ub4) 1,
                     val.dptr, val.dsize, OCI_ONE_PIECE,
                     (dvoid *) 0, NULL, (ub2) 0, (ub1) 0);
    if (rc != 0) {
        OCIErrorGet((dvoid *) p_err, (ub4) 1, (text *) NULL,
                    &errcode, errbuf, (ub4) sizeof(errbuf),
                    OCI_HTYPE_ERROR);
        fprintf(stderr,
                "Error in mdxStore (write blob) - %.*s\n", 512, errbuf);
        OCIDescriptorFree ( p_blob,   OCI_DTYPE_LOB );                
        free(pnfsid);                
        goto bad;
    }


    vh = &hash[(h->lkey1 >> 3) & HASHAND];
    h->vh = vh;                 /* mark as active */
    ADDRING(vh->head, vh->tail, h);
    ADDRING(ccHead, ccTail, c);
#ifdef STATS
    stats[val.dsize]++;
    statsCounter++;
    if ((statsCounter % 5000) == 0)
        printStats();
#endif
    /* end the transaction */
    OCIDescriptorFree ( p_blob,   OCI_DTYPE_LOB );	
    free(pnfsid);
    return (0);
  bad:
    ADDERING(ccHead, ccTail, c);        /* put zombie back in list */
    ADDRING(emptyHead, emptyTail, h);
    /* cancel the transaction */
    return -1;
}

int mdxIoctl(MDX_FILE mdxf, int argc, char *argv[],
             int *replyLen, char *reply)
{
    return -1;
}

int mdxDelete(MDX_FILE conn, mdlDatum key)
{

    int rc;
    char errbuf[100];
    int errcode;
    char query[265];
    char *pnfsid;


	sprintf(query, "DELETE FROM t_%s WHERE pnfsid=:x", tbName );

    pnfsid = id2string(key);
    rc = OCIStmtPrepare(p_sql, p_err, query,
                        (ub4) strlen((char *) query),
                        (ub4) OCI_NTV_SYNTAX, (ub4) OCI_DEFAULT);
    rc = OCIBindByName(p_sql, &p_bnd, p_err, (text *) ":x", -1,
                       (dvoid *) pnfsid, strlen(pnfsid) + 1,
                       SQLT_STR, (dvoid *) 0, (ub2 *) 0,
                       (ub2 *) 0, (ub4) 0, (ub4 *) 0, OCI_DEFAULT);
    rc = OCIStmtExecute(p_svc, p_sql, p_err, (ub4) 1, (ub4) 0,
                        (CONST OCISnapshot *) 0,
                        (OCISnapshot *) 0, (ub4) OCI_DEFAULT);
    if (rc != 0) {
        OCIErrorGet((dvoid *) p_err, (ub4) 1, (text *) NULL,
                    &errcode, errbuf, (ub4) sizeof(errbuf),
                    OCI_HTYPE_ERROR);
        fprintf(stderr, "Error in mdxDelete - %.*s\n", 512, errbuf);
    }

    free(pnfsid);
    return rc;
}

int mdxClose(MDX_FILE mdx)
{
    int rc;
    
    if (mdx == NULL) {
        return 0;
    }
    
    removeCache();
    if (p_svc != NULL) {
        rc = OCILogoff(p_svc, p_err);   /* Disconnect */
        rc = OCIHandleFree((dvoid *) p_sql, OCI_HTYPE_STMT);    /* Free handles */
        rc = OCIHandleFree((dvoid *) p_svc, OCI_HTYPE_SVCCTX);
        rc = OCIHandleFree((dvoid *) p_err, OCI_HTYPE_ERROR);
        
        p_svc = NULL;
        p_err = NULL;
        p_sql = NULL;
    }
	
	if( tbName != NULL ) {
		free(tbName);
	}
	
    return rc;
}

int mdxFlush(MDX_FILE mdx, int force)
{
    return 0;
}

int mdxReadLock(MDX_FILE mdx)
{
    return 0;
}

int mdxWriteLock(MDX_FILE mdx)
{
	++isWriteLock;
    (void) OCITransStart(p_svc, p_err, 60, OCI_TRANS_NEW);
    return 0;
}

int mdxCommitLock(MDX_FILE mdx)
{
	int rc = 0;
    char errbuf[100];
    int errcode;
	
	if( isWriteLock ) {	
		isWriteLock = 0;
	    rc = OCITransCommit(p_svc, p_err, (ub4) 0);
    	if (rc != 0) {
        	OCIErrorGet((dvoid *) p_err, (ub4) 1, (text *) NULL,
                    &errcode, errbuf, (ub4) sizeof(errbuf),
                    OCI_HTYPE_ERROR);
	        fprintf(stderr,  "Error in mdxCommit - %.*s\n", 512, errbuf);
	    }			    
	}
    return rc;
}

int mdxAbortLock(MDX_FILE mdx)
{
	(void) OCITransRollback(p_svc, p_err, (ub4) 0);
    return 0;
}

static char *id2string(mdlDatum key)
{

	char *str = NULL;
	int i;
	
	str = malloc( key.dsize *2 + 1 );
	
	memset(str, 0, key.dsize *2 + 1 );
	
	for( i = 0; i < key.dsize ; i++ ) {		
		sprintf( str + 2*i , "%.2X", (unsigned char)key.dptr[i] );		
	}

	str[i*2] = '\0';
	
    return str;
}


void string2id(const char *str, char *ptr)
{

    int i, j;
    static char c2[3];
    c2[2] = '\0';
    
    for (i = 0, j = 0; i < 24; i += 2, j++) {

        c2[0] = str[i];
        c2[1] = str[i + 1];
        ptr[j] = strtol(c2, (char **) NULL, 16);
    }    
}
