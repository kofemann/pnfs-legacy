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
 #include <stdlib.h>
#include <string.h>
#include <gdbm.h>
#include "dbglue.h"

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
#define ADDERING(head, tail, obj) \
{ \
  if ((head) == NULL) { \
    (head) = (tail) = (obj); \
    (obj)->prev = (obj)->next = NULL; \
  } else { \
    (head)->prev = (obj); \
    (obj)->next = (head); \
    (obj)->prev = NULL; \
    (head) = (obj); \
  } \
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

/* #define CDEB 1   /* for debug output */
/* #define STATS 1   /* for cache hit statistics */
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
/* static CPage cPages[CACHESIZE];        /* that's the big chunk */

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
static char *printKey(bla *b)
{
  static char s[200];
  (void) sprintf(s, "%04x%04x%08x%08x", b->s1, b->s2, b->l1, b->l2);
  return(s);
}
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
  for(i=0; i<HASHVSIZE; i++)
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
  for(i=0; i<=RECSIZE; i++) {
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
  for(i=0; i<HASHVSIZE; i++) {
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
mdlDatum mdxFirst( MDX_FILE mdx )
{
   datum v ;
   mdlDatum val ;
   
   v = gdbm_firstkey( (GDBM_FILE) mdx ) ;
   
   val.dsize = v.dsize ;
   val.dptr  = v.dptr ;
   if( v.dptr )val.flags = MDX_FREE ;
   
   return val ;
}
mdlDatum mdxNext( MDX_FILE mdx , mdlDatum key )
{
   datum k , v ;
   mdlDatum val ;
   
   k.dptr  = key.dptr ;
   k.dsize = key.dsize ;
   
   v = gdbm_nextkey( (GDBM_FILE) mdx , k ) ;
   
   val.dsize = v.dsize ;
   val.dptr  = v.dptr ;
   if( v.dptr )val.flags = MDX_FREE ;
   
   return val ;
}

int mdxScan( MDX_FILE mdx )
{
int rc , i;
unsigned char tmp[8] ;
datum key , nextkey ;

        key = gdbm_firstkey ( mdx );
        while ( key.dptr ) {
           nextkey = gdbm_nextkey ( mdx, key );
           memcpy( tmp , key.dptr , 8 ) ;
           for(i=0;i<8;i++)fprintf(stderr,"%2.2X",tmp[i]);
           fprintf(stderr,"\n");
           key = nextkey;
        };
  
  return 0 ;
}

MDX_FILE mdxOpen( char *name , int flags ,int mode )
{

 int flag ;
 GDBM_FILE x ;
 char tname[1024];
 
 flag = 0 ;
 if( flags & MDX_RDONLY )flag |= GDBM_READER ;
 if( flags & MDX_RDWR )flag |= GDBM_WRITER ;
 flag |= GDBM_FAST ;
 
 if ((x = gdbm_open( name , 8*1024 , flag , mode , NULL )) != NULL) {
   if (initCache() == 0)
     return((MDX_FILE) x);
   gdbm_close(x);
   return((MDX_FILE) 0);
 }
 return((MDX_FILE) x);
}


MDX_FILE mdxCreate( char *name , int flags ,int mode )
{
 char tname[1024];
 int flag ;
 GDBM_FILE x ;
 
 flag = 0 ;
 flag |= GDBM_WRCREAT ;

 if ((x = gdbm_open( name , 8*1024 , flag , mode , NULL )) != NULL) {
   if (initCache() == 0)
     return((MDX_FILE) x);
   gdbm_close(x);
   return((MDX_FILE) 0);
 }
 return((MDX_FILE) x);
}

int mdxCheck( char *name )
{
 GDBM_FILE x ;
 
 
 x = gdbm_open( name , 8*1024 , GDBM_READER , 0 , NULL ) ;
 if( x )gdbm_close( x ) ;
 return x ? 0 : -1 ;
}

int mdxFetch( MDX_FILE mdx , mdlDatum key, mdlDatum *val )
{
  datum k , v ;

  if (fetchCache(&key, val) == 0) {
#ifdef CDEB
    printf("FETCH '%s' IN CACHE\n", printKey((bla *) key.dptr));
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

#if 0
    cp = &cPages[h->pageIndex];
    /* clear data area */
    memset((char *) cp, 0, sizeof(CPage));
#endif

    k.dptr  = key.dptr ;
    k.dsize = key.dsize ;
   
    v = gdbm_fetch( (GDBM_FILE) mdx , k ) ;

    if (v.dptr) {
#if 0
      memcpy(cp->value, v.dptr, v.dsize);
      (void) free(v.dptr);
#endif
#ifdef CDEB
      printf("FETCH '%s' (%d) NOT IN CACHE\n",
	     printKey((bla *) key.dptr), vIndex);
#endif
#if 0    
      val->dptr = cp->value;
#endif
      val->dptr = h->dataPtr = v.dptr;
      val->dsize = h->dataSize = v.dsize;
      val->flags = 0;

      h->lkey = p[0]; h->lkey0 = p[1]; h->lkey1 = p[2];
      /* include HCache into vertical Hash list */
      vh = &hash[(h->lkey1 >> 3) & HASHAND];
      h->vh = vh;
      ADDRING(vh->head, vh->tail, h)
      ADDRING(ccHead, ccTail, c);  /* move to newest */
      return 0;
    }
    /* put zombies back into list */
    ADDERING(ccHead, ccTail, c);
    ADDRING(emptyHead, emptyTail, h);
  }
#ifdef CDEB
  printf("FETCH ERROR %d\n", gdbm_errno);
#endif
  val->dptr = NULL; val->dsize = 0; val->flags = 0;
  return 0;
}

int mdxStore( MDX_FILE mdx , mdlDatum key , mdlDatum val ) 
{
  datum k, v;
  /*  CPage *cp; */
  HCache *h;
  CCache *c;
  Hash *vh;
  unsigned long *p = (unsigned long *) key.dptr;

#ifdef CDEB
  printf("STORE '%s' '%x' (%d)", printKey((bla *) key.dptr), p[2], val.dsize);
#endif
  if ((h = locateCache((cKey *) key.dptr)) == (HCache *) 0) {
#ifdef CDEB
    printf("NOT IN CACHE ");
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
    printf("IN CACHE ");
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

  v.dptr = val.dptr; v.dsize = val.dsize;
  k.dptr = key.dptr; k.dsize = key.dsize;

  if (gdbm_store( (GDBM_FILE) mdx , k , v , GDBM_REPLACE ) == 0) {
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
    return(0);
  }

bad:  
#ifdef CDEB
  printf("ERROR %d\n", gdbm_errno);
#endif
  ADDERING(ccHead, ccTail, c);  /* put zombie back in list */
  ADDRING(emptyHead, emptyTail, h);
  return(-1);
}
int mdxIoctl( MDX_FILE mdxf , int argc , char * argv [] , int * replyLen , char *reply ){
   return -1 ;
}

int mdxDelete( MDX_FILE mdx , mdlDatum key  )
{
  datum k;

  k.dsize = key.dsize ;
  k.dptr  = key.dptr ;
  deleteCache(&key);
  return(gdbm_delete( (GDBM_FILE) mdx , k ));
}

int mdxClose( MDX_FILE mdx )
{
  removeCache();
  gdbm_close( (GDBM_FILE) mdx ) ;
}
int mdxFlush( MDX_FILE mdx , int force ) 
{
   gdbm_sync( (GDBM_FILE) mdx );
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
