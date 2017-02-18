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
#include <libpq-fe.h>


/* keep Fermi happy */
static const char defaultConnectString[] = "user=postgres";

#define TEXT   0
#define BINARY 1
/*#define BYTEAOID 17     This should be taken from postgres include file */

#define CACHE 1         /* define this to enable cache code */
#define CACHESIZE 400   /* number of records to cache */
#define RECSIZE 1012    /* max size of DB record */
#define HASHVSIZE 256   /* vertical size of HASH */
#define HASHAND 0xff    /* to generate index from 0 to 255 */


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

/* for debug output */
/* #define CDEB 1 */

/* for cache hit statistics */
/* #define STATS 1 */
/* PSCI (Pnfs Simple Cache Implementation !!!!)  */

/* structure used in Hash ring buffers */
typedef struct HCache {
    /* the key we are looking for */
    unsigned long lkey;
    unsigned long lkey0;
    unsigned long lkey1;
    /* int pageIndex; */
    char *dataPtr;  /* ptr to gdbm's record */
    int  dataSize;
    struct CCache *cc;   /* link back into usage queue */
    struct Hash *vh;     /* link back into Hash vertical table */
    struct HCache *prev;
    struct HCache *next;
} HCache;

/* vertical structure of Hash */
typedef struct Hash {
    struct HCache *head;
    struct HCache *tail;
} Hash;

typedef struct CCache {  /* sort pages according to last use */
    struct HCache *h;   /* link to HCache linked list */
    struct CCache *next;
    struct CCache *prev;
} CCache;

typedef struct cKey {
    unsigned long  l;
    unsigned long  l0;
    unsigned long  l1;
} cKey;

#if 0
typedef struct CPage {
    double alignementDummy;
    unsigned char value[RECSIZE];
} CPage;
#endif

static CCache *ccHead = (CCache *) 0;
static CCache *ccTail = (CCache *) 0;
/* static CPage cPages[CACHESIZE];        that's the big chunk */

static Hash hash[HASHVSIZE];  /* vertical hash list */
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
    unsigned long  l1;
    unsigned long  l2;
} bla;

/*
typedef struct {
     char *dptr;
     int dsize;
     int flags ;
} mdlDatum;

 */
#ifdef CDEB
static char *printKey(bla *b)
{
    static char s[200];
    (void) sprintf(s, "%04x%04x%08lx%08lx", b->s1, b->s2, b->l1, b->l2);
    return(s);
}
#endif

int  mdxInit( char * (*getenv)( const char * name ) , 
              int (*prt)(int , char *,...)){
    return 0 ;
}

static void removeCache()
{
    int i;
    CCache *co, *c = ccHead;

    while(c) {
        co = c;
        c = c->next;
        if (co->h->dataSize)
            (void) free(co->h->dataPtr);
        (void) free(co->h);
        (void) free(co);
    }
    ccHead = ccTail = (CCache *) 0;
    for (i=0; i<HASHVSIZE; i++)
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
    for (i=0; i<=RECSIZE; i++) {
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
    if (ccHead)  /* already done */
        return(0);
    for (i=0; i<CACHESIZE; i++) {
        if ((c = (CCache *) malloc(sizeof(CCache))) == NULL) {
            fprintf(stderr, "initCache: out of memory\n");
            removeCache();
            return(-1);
        }
        if ((h = (HCache *) malloc(sizeof(HCache))) == NULL) {
            fprintf(stderr, "initCache: out of memory\n");
            removeCache();
            return(-1);
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
    for (i=0; i<HASHVSIZE; i++) {
        hash[i].head = hash[i].tail = (HCache *) 0;
    }
    return(0);
}

static HCache *locateCache(cKey *k)
{
    Hash *hv = &hash[(k->l1 >> 3) & HASHAND];
    HCache *hc = hv->head;

    while(hc) {
        if (hc->lkey1 == k->l1 &&
            hc->lkey0 == k->l0 &&
            hc->lkey == k->l)
            return(hc);
        hc = hc->next;
    }
    return((HCache *) 0);
}

static int fetchCache(mdlDatum *k, mdlDatum *d)
{
    HCache *h;
  
    if ((h = locateCache((cKey *) k->dptr))) {
#if 0
        d->dsize = RECSIZE;
        d->dptr = cPages[h->pageIndex].value;
#endif
        d->dptr = h->dataPtr;
        d->dsize = h->dataSize;

        DELRING(ccHead, ccTail, h->cc);    /* move to newest */
        ADDRING(ccHead, ccTail, h->cc);
        return(0);
    }
    return(-1);
}

static void deleteCache(mdlDatum *k)
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
        DELRING(ccHead, ccTail, h->cc);  /* move to oldest */
        ADDERING(ccHead, ccTail, h->cc);
    }
}

mdlDatum mdxFirst( MDX_FILE conn )
{
    mdlDatum val ;
    PGresult *res;
    int len;
    char *value;
   
    val.dptr = NULL; val.dsize = 0; val.flags = 0;
    /* Start a transaction block */
    res = PQexec((PGconn *)conn, "BEGIN");
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "BEGIN command failed: %s\n", PQerrorMessage((PGconn *)conn));
        PQclear(res);
        return val ;
    }
    /* Should PQclear PGresult whenever it is no longer needed to avoid memory leaks */
    PQclear(res);
    
    /* Fetch row from pnfs */
    res = PQexec((PGconn *)conn, "DECLARE cr BINARY CURSOR FOR select pnfsid from pnfs");
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "DECLARE CURSOR failed: %s\n", PQerrorMessage((PGconn *)conn));
        PQclear(res);
        return val ;
    }
    PQclear(res);
    
    res = PQexec((PGconn *)conn, "FETCH FIRST in cr");
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "FETCH FIRST failed: %s\n", PQerrorMessage((PGconn *)conn));
        PQclear(res);
        return val ;
    }
    
    value = PQgetvalue(res, 0, 0);    /* Get pointer to the result  */
    len =  PQgetlength(res, 0, 0);    /* Get result length          */
    val.dptr  = (char *)malloc(len);  /* Allocate memory for result */
    memcpy(val.dptr, value, len);     /* Copy the result in place   */
    val.dsize = len;                  /* Stopy the result length    */
    if ( value ) val.flags = MDX_FREE ; /* ??? */
    PQclear(res);
    return val ;
}

mdlDatum mdxNext( MDX_FILE conn, mdlDatum key )
{
    mdlDatum val ;

    PGresult *res;
    int len;
    char *value;
   
    val.dptr = NULL; val.dsize = 0; val.flags = 0;

    res = PQexec((PGconn *)conn, "FETCH NEXT in cr");
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "FETCH NEXT failed: %s\n", PQerrorMessage((PGconn *)conn));
        PQclear(res);
        return val ;
    }
    if (PQntuples(res) > 0) {
        value = PQgetvalue(res, 0, 0);    /* Get pointer to the result  */
        len =  PQgetlength(res, 0, 0);    /* Get result length          */
        val.dptr  = (char *)malloc(len);  /* Allocate memory for result */
        memcpy(val.dptr, value, len);     /* Copy the result in place   */
        val.dsize = len;                  /* Stopy the result length    */
        if ( value ) val.flags = MDX_FREE ; /* ??? */
    } else {             /* If fetch returns no tuples we have reached the end of result set. Close the transaction. */
	PQclear(res);

	/* close the cursor ... we don't bother to check for errors ... */
	res = PQexec((PGconn *)conn, "CLOSE cr");
	PQclear(res);

	/* end the transaction */
	res = PQexec((PGconn *)conn, "END");
    }
    PQclear(res);
    return val ;
}

void mdxBreak( MDX_FILE conn )
{
    PGresult *res;

    /* close the cursor ... we don't bother to check for errors ... */
    res = PQexec((PGconn *)conn, "CLOSE cr");
    PQclear(res);

    /* end the transaction */
    res = PQexec((PGconn *)conn, "END");
    PQclear(res);
}

int mdxScan( MDX_FILE mdx )
{
    int i;
    mdlDatum key;

    key = mdxFirst ( mdx );
    while ( key.dptr ) {
        for (i = 0; i < 8; i++)
            fprintf(stderr,"%2.2x",key.dptr[i]);
        fprintf(stderr,"\n");
        if (key.dptr) free(key.dptr);
        key = mdxNext( mdx, key );
    };
  
    return 0 ;
}

MDX_FILE mdxOpen( char *name, int flags, int mode )
{
    PGconn     *conn;   /* Connection to the database */
    char query[128];
    char *dbName;
    const char *connectString;

    /* Make a connection to the database */
    dbName = rindex(name, '/');
    if( dbName == NULL ) {
	dbName = name;
    }else{
	++dbName;
    }
    
    sprintf(query, "%sConnect", dbName);    /* VP */
    connectString = getenv(query);          /* VP */
    if( connectString == NULL ) {
        connectString = getenv("dbConnectString");
    }      
    if( connectString == NULL ) {
        connectString = defaultConnectString;
    }
    sprintf(query, "dbname=%s %s", dbName, connectString);
    conn = PQconnectdb(query);

    /* Check to see that the backend connection was successfully made */
    if (PQstatus(conn) != CONNECTION_BAD) {
        if (initCache() == 0) {
            return((MDX_FILE) conn);
        } 
    } 
    fprintf(stderr, "Connection to database '%s' failed.\n", name);
    fprintf(stderr, "%s", PQerrorMessage(conn));
    PQfinish(conn); /* exit(1); Exit nicely */
    return((MDX_FILE) 0);
}


MDX_FILE mdxCreate( char *name, int flags, int mode )
{
    PGconn     *conn;   /* Connection to the database */
    PGresult   *res;    /* Result Set                 */
    char query[128];
    char *dbName;
    const char *connectString;

    /* Make a connection to the database */
    dbName = rindex(name, '/');
    if( dbName == NULL ) {
	dbName = name;
    }else{
	++dbName;
    }
    
    sprintf(query, "%sConnect", dbName);    /* VP */
    connectString = getenv(query);          /* VP */
    if( connectString == NULL ) {
        connectString = getenv("dbConnectString");
    }      
    if( connectString == NULL ) {
        connectString = defaultConnectString;
    }
    sprintf(query, "dbname=template1 %s", connectString);

    conn = PQconnectdb(query);

    /* Check to see that the backend connection was successfully made */
    if (PQstatus(conn) != CONNECTION_BAD) {
        dbName = rindex(name, '/')+1;
/*        sprintf(query, "CREATE DATABASE \"%s\" WITH LOCATION 'PGDATA2'", dbName); */
        sprintf(query, "CREATE DATABASE \"%s\"", dbName);
        res = PQexec(conn, query);
        if (PQresultStatus(res) != PGRES_COMMAND_OK) {
            fprintf(stderr, "CREATE DATABASE command failed: %s\n", PQerrorMessage(conn));
            PQclear(res);
            PQfinish(conn);
            return((MDX_FILE) 0);
        }
        PQclear(res);
        PQfinish(conn);        /* We don't need the connection to 'template1' anymore */

        if ((conn = mdxOpen(name, flags, mode)) != NULL) {
            res = PQexec(conn, "CREATE TABLE pnfs (pnfsid bytea PRIMARY KEY, data bytea, date timestamp)");
            if (PQresultStatus(res) != PGRES_COMMAND_OK) {
                fprintf(stderr, "CREATE TABLE command failed: %s\n", PQerrorMessage(conn));
                PQclear(res);
                PQfinish(conn);
                return((MDX_FILE) 0);
            }
	    PQclear(res);
# if 0            
	    res = PQexec(conn, "CREATE TRIGGER pnfs_trg BEFORE INSERT ON pnfs FOR EACH ROW EXECUTE PROCEDURE pnfs_trig()");
            if (PQresultStatus(res) != PGRES_COMMAND_OK) {
                fprintf(stderr, "CREATE TRIGGER command failed: %s\n", PQerrorMessage(conn));
                PQclear(res);
                PQfinish(conn);
                return((MDX_FILE) 0);
            }
	    PQclear(res);
# endif
            if (initCache() == 0) {
                return((MDX_FILE) conn);
            } 
        }
    } 
    fprintf(stderr, "Connection to database '%s' failed.\n", name);
    fprintf(stderr, "%s", PQerrorMessage(conn));
    PQfinish(conn); /* exit(1);  Exit nicely */
    return((MDX_FILE) 0);
}

int mdxCheck( char *name )
{
    return 0;
}

int mdxFetch( MDX_FILE conn, mdlDatum key, mdlDatum *val )
{
    PGresult   *res;
    char *value;
    int len;
    const char *paramValues[1] =  {key.dptr};
/*     Oid paramTypes[1] = {BYTEAOID};  */
    int paramLengths[1] = {key.dsize};
    int paramFormats[1] = {BINARY};
    val->dptr = NULL; val->dsize = 0; val->flags = 0;


    if (fetchCache(&key, val) == 0) {
#ifdef CDEB
        fprintf(stderr,"FETCH '%s' IN CACHE\n", printKey((bla *) key.dptr));
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

        /* Fetch row from pnfs */
        res = PQexecParams((PGconn *)conn,
                           "SELECT data FROM pnfs WHERE pnfsid = $1",
                           1,               /* one param                           */
                           NULL,            /* let the backend deduce param type   */
                           paramValues, paramLengths, paramFormats,
                           1);              /* ask for binary results              */
        if (PQresultStatus(res) != PGRES_TUPLES_OK)  {
#ifdef CDEB
            fprintf(stderr, "FETCH failed: %s\n", PQerrorMessage((PGconn *)conn));
#endif
            goto bad;
        }
        if (PQntuples(res) > 0) {
            value = PQgetvalue(res, 0, 0);    /* Get pointer to the result  */
            len =  PQgetlength(res, 0, 0);    /* Get result length          */
            val->dptr  = (char *)malloc(len);  /* Allocate memory for result */
            memcpy(val->dptr, value, len);     /* Copy the result in place   */
            val->dsize = len;                  /* Stopy the result length    */
#ifdef CDEB
            fprintf(stderr,"FETCH '%s' (%d) NOT IN CACHE\n",
                    printKey((bla *) key.dptr), vIndex);
#endif
            h->dataPtr = val->dptr;
            h->dataSize = val->dsize;
            val->flags = 0;

            h->lkey = p[0]; h->lkey0 = p[1]; h->lkey1 = p[2];
            /* include HCache into vertical Hash list */
            vh = &hash[(h->lkey1 >> 3) & HASHAND];
            h->vh = vh;
            ADDRING(vh->head, vh->tail, h);
            ADDRING(ccHead, ccTail, c);  /* move to newest */
            PQclear(res);
            return 0;
        }
        PQclear(res);
        /* put zombies back into list */
        ADDERING(ccHead, ccTail, c);
        ADDRING(emptyHead, emptyTail, h);
        val->dptr = NULL; val->dsize = 0; val->flags = 0;
        return 0;

    bad:
        PQclear(res);
        /* put zombies back into list */
        ADDERING(ccHead, ccTail, c);
        ADDRING(emptyHead, emptyTail, h);
#ifdef CDEB
        fprintf(stderr, "FETCH failed: %s\n", PQerrorMessage((PGconn *)conn));
#endif
        val->dptr = NULL; val->dsize = 0; val->flags = 0;
        return -1;
    }
}

int mdxStore( MDX_FILE conn, mdlDatum key , mdlDatum val ) 
{
    HCache *h;
    CCache *c;
    Hash *vh;
    unsigned long *p = (unsigned long *) key.dptr;

    PGresult   *res;
    const char *paramValues[2] = { key.dptr, val.dptr };
/*     Oid paramTypes[2] = {BYTEAOID, BYTEAOID};  */
    int paramLengths[2] = { key.dsize, val.dsize };
    int paramFormats[2] = {BINARY, BINARY};
    int isUpdated = 0;

#ifdef CDEB
    fprintf(stderr,"STORE '%s' '%x' (%d)\n", printKey((bla *) key.dptr), p[2], val.dsize);
#endif
    if ((h = locateCache((cKey *) key.dptr)) == (HCache *) 0) {
#ifdef CDEB
        fprintf(stderr,"NOT IN CACHE ");
#endif
        /* grab new Hash entry here */
        c = ccHead;   /* grab oldest */
        h = c->h;
        /* override with new key data */
        h->lkey =  p[0];
        h->lkey0 = p[1];
        h->lkey1 = p[2];
    } else {   /* found in active Cache list */
        c = h->cc;
#ifdef CDEB
        fprintf(stderr,"IN CACHE ");
#endif
    }

    /* check if entry was inuse before */
    if (h->vh) {
        DELRING(h->vh->head, h->vh->tail, h);
    } else {
        DELRING(emptyHead, emptyTail, h);
    }
    h->vh = (Hash *) 0;  /* mark as NOT IN USE */

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

    /* Start a transaction block */
    res = PQexec(conn, "BEGIN");
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "BEGIN command failed: %s\n", PQerrorMessage(conn));
	PQclear(res);
        goto bad;
    }
    PQclear(res);  /* Should PQclear PGresult whenever it is no longer needed to avoid memory leaks */

    res = PQexecParams((PGconn *)conn,
                       "UPDATE pnfs SET data=$2 WHERE pnfsid=$1",
                       2,               /* one param                           */
                       NULL,            /* let the backend deduce param type   */
                       paramValues, paramLengths, paramFormats,
                       1);              /* ask for binary results              */

    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "UPDATE pnfs TABLE failed: %s\n", PQerrorMessage((PGconn *)conn));
	PQclear(res);
        goto bad;
    }

	/* get number of updated rows */    
	isUpdated = atoi(PQcmdTuples(res)); 
    PQclear(res);


	/* if there is updated rows, then insert one */
	if( ! isUpdated ) {
	    res = PQexecParams((PGconn *)conn,
	                       "INSERT INTO pnfs VALUES ($1, $2, now())",
	                       2,               /* two params                          */
	                       NULL,            /* let the backend deduce param type   */
	                       paramValues, paramLengths, paramFormats,
	                       1);              /* ask for binary results              */
	
	    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
	        fprintf(stderr, "INSERT INTO pnfs TABLE failed: %s\n", PQerrorMessage((PGconn *)conn));
		PQclear(res);
	        goto bad;
	    }
	    PQclear(res);
	}
	
    vh = &hash[(h->lkey1 >> 3) & HASHAND];
    h->vh = vh;  /* mark as active */
    ADDRING(vh->head, vh->tail, h);
    ADDRING(ccHead, ccTail, c);
#ifdef STATS
    stats[val.dsize]++;
    statsCounter++;
    if ((statsCounter % 5000) == 0)
        printStats();
#endif
# if 1
    /* end the transaction */
    res = PQexec(conn, "COMMIT");
    PQclear(res);
# endif
    return(0);

    bad:  
    ADDERING(ccHead, ccTail, c);  /* put zombie back in list */
    ADDRING(emptyHead, emptyTail, h);
# if 1
    /* cancel the transaction */
    res = PQexec(conn, "ROLLBACK");
    PQclear(res);
# endif
    return -1;
}

int mdxRegisterDeletetion(MDX_FILE conn, mdlDatum key, mdlDatum val) 
{
	PGresult   *res;
    const char *paramValues[2] = { key.dptr, val.dptr };
/*     Oid paramTypes[2] = {BYTEAOID, BYTEAOID};  */
    int paramLengths[2] = { key.dsize, val.dsize };
    int paramFormats[2] = {BINARY, BINARY};

	res = PQexecParams((PGconn *)conn,
			"INSERT INTO deleted VALUES ($1, $2, now())",
			2,               /* two params                          */
			NULL,            /* let the backend deduce param type   */
			paramValues, paramLengths, paramFormats,
			1);              /* ask for binary results              */

	if (PQresultStatus(res) != PGRES_COMMAND_OK) {
		fprintf(stderr, "INSERT INTO 'deleted' TABLE failed: %s\n", PQerrorMessage((PGconn *)conn));
		PQclear(res);
	    return -1;
	}
	PQclear(res);
    return 0;
}

int mdxIoctl( MDX_FILE mdxf , int argc , char * argv [] , int * replyLen , char *reply ){
    return -1 ;
}

int mdxDelete( MDX_FILE conn, mdlDatum key )
{
    PGresult   *res;
    const char *paramValues[1] = {key.dptr};
/*     Oid paramTypes[1] = {BYTEAOID};  */
    int paramLengths[1] =  {key.dsize};
    int paramFormats[1] = {1};

    deleteCache(&key);

    res = PQexecParams((PGconn *)conn,
                       "DELETE FROM pnfs WHERE pnfsid=$1",
                       1,               /* one param                           */
                       NULL,            /* let the backend deduce param type   */
                       paramValues, paramLengths, paramFormats,
                       1);              /* ask for binary results              */

    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "DELETE FROM pnfs TABLE failed: %s\n", PQerrorMessage((PGconn *)conn));
        PQclear(res);
        return -1;
    }
    PQclear(res);
    return 0;
}

int mdxClose( MDX_FILE mdx )
{
    removeCache();
    PQfinish((PGconn *)mdx);
    return 0;
}

int mdxFlush( MDX_FILE mdx , int force ) 
{
    return 0 ;
}

int mdxReadLock( MDX_FILE mdx )
{
    return 0 ;
} 

int mdxWriteLock( MDX_FILE mdx )
{
    return 0 ;
} 

int mdxCommitLock( MDX_FILE mdx )
{
    return 0 ;
} 

int mdxAbortLock( MDX_FILE mdx )
{
    return 0 ;
} 
