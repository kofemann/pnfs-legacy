/**
 *  A fairly simple wrapper functions for shm API.  These mainly
 *  correct for the non-interruptable nature of the shm driver and to
 *  implement retries should PNFS return SCL_TIMEOUT.
 *
 *  The default policy is to retry indefinitely until rc !=
 *  SCL_TIMEOUT.  The end-user may override this by calling
 *  myGet_register_max_retries() with +ve numbers.  This is achieved
 *  using the -m command-line option.
 *
 *  Initially, retries are attempt as fast as possible.  After
 *  TRY_EMIT_WARNING attempts a warning message is emitted and the
 *  retries are attempted after a sleep().  The initial sleep duration
 *  is a second.  This increases linearly after each SCL_TIMEOUT until
 *  it reaches MAX_SLEEP_TIME seconds.
 */
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

#include <string.h> // NEEDED FOR DEBUG CODE

#include "dump_wrap.h"

/* How many db operations before introducing the delay */
#define BE_NICE_COUNT 100

/* A constant */
#define USECS_IN_MILLISECOND 1000
#define USECS_IN_SECOND      1000000

/* The number of retries before warning end-user */
#define TRY_EMIT_WARNING 3

/* How often to emit a "still trying" message */
#define TRY_EMIT_STILL_TRYING 10

/* Maximum time sleep()ing before a db operation when retrying */
#define MAX_SLEEP_TIME   5

static void shmclient_call_recover();
static void shmclient_call_prepare();
static void maybe_sleep();


/**
 * Build a retry-loop using a for().
 *
 * NB this is border-line macro abuse.  The implementation requires
 * that the loop contents does not do escape from the loop: there must
 * be no break, goto or return statements inside the loop.
 */
#define RETRY_DB_OPERATION(NAME,TRY,RC,ID)  for(			     \
						_retry_db_operation_init( &TRY, &RC); \
					  _retry_db_operation_test( NAME, TRY, RC, ID); \
					  _retry_db_operation_incr( &TRY))

static void _retry_db_operation_init( int *try_p, int *rc_p);
static int _retry_db_operation_test( const char *name, int try, int rc, md_id_t id);
static void _retry_db_operation_incr( int *try_p);

/* Used to store the old sigmask state whilst undertaking a shm operation */
static sigset_t old_set;

static struct timeval shm_db_total_elapsed, shm_overhead_total_elapsed;
static struct timeval shm_db_this_call_start, shm_overhead_this_call_start;

static unsigned int be_nice_delay = MYGET_DEFAULT_DELAY * USECS_IN_MILLISECOND;
static long be_nice_counter;

static int max_retries;


#ifndef timeradd
#warning Using custom timeradd
#define timeradd(a,b,r) do{                        \
       (r)->tv_usec = (a)->tv_usec + (b)->tv_usec; \
       (r)->tv_sec  = (a)->tv_sec  + (b)->tv_sec;  \
                                                   \
       while( (r)->tv_usec > USECS_IN_SECOND) {    \
        (r)->tv_usec -= USECS_IN_SECOND;           \
        (r)->tv_sec++;                             \
       }                                           \
  } while(0)
#endif

#ifndef timersub
#warning Using custom timersub
#define timersub(a,b,r) do{                \
       (r)->tv_usec = (a)->tv_usec;        \
       (r)->tv_sec  = (a)->tv_sec;         \
                                           \
       if( (r)->tv_usec < (b)->tv_usec) {  \
         (r)->tv_sec--;                    \
         (r)->tv_usec += USECS_IN_SECOND;  \
       }                                   \
                                           \
       (r)->tv_sec -= (b)->tv_sec;         \
       (r)->tv_usec -= (b)->tv_usec;       \
                                           \
       while( (r)->tv_usec > USECS_IN_SECOND) {\
         (r)->tv_usec -= USECS_IN_SECOND;  \
         (r)->tv_sec++;                    \
       }                                   \
    } while(0)
#endif


/**
 *  Register a delay: the delay is expressed in milliseconds.
 */
void myGet_register_delay( long delay)
{
  if( delay < 0)
    return;

  be_nice_delay = delay * USECS_IN_MILLISECOND;
}

/**
 *  Allow end-user to register a max number of tries before giving up.
 */
void myGet_register_max_retries( int new_max_retries)
{
  if( new_max_retries >= 0)
    max_retries = new_max_retries;
}


int myGetRootId( SCL *scl , int db , md_id_t *id )
{
  int rc, try;
  md_id_t dummy_id;

  mdSetNullID( dummy_id);

  if( !scl)
    return -1; // FAILED.

  RETRY_DB_OPERATION( "mdmGetRootId", try, rc, dummy_id)
    rc = mdmGetRootId( scl , db, id);

  return rc;
}


int myGetRecord( SCL *scl , md_id_t id ,  mdRecord *rec )
{
  int rc, try;

  if( !scl)
    return -1; // FAILED.

  RETRY_DB_OPERATION( "mdmGetRecord", try, rc, id)
    rc = mdmGetRecord( scl, id, rec);

  return rc;
}



int myReadDirAuth( SCL *scl, md_auth *auth, md_id_t id, mdPermission perm,
		   long cookie, int count, reqExtItem *extItem)
{
  int rc, try;

  if( !scl)
    return -1; // FAILED.

  RETRY_DB_OPERATION( "mdmReadDirAuth", try, rc, id)
    rc = mdmReadDirAuth( scl, auth, id, perm, cookie, count, extItem);

  return rc;
}


int myReadData( SCL *scl, md_id_t id, mdPermission perm, long offset,
		long size, char *data)
{
  int rc, try;

  if( !scl)
    return -1; // FAILED.

  RETRY_DB_OPERATION( "mdmReadData", try, rc, id)
    rc = mdmReadData( scl, id, perm, offset, size, data);

  return rc;
}

void myTimeElapsed( struct timeval *overhead_elapsed_p,
		    struct timeval *db_elapsed_p)
{
  memcpy( db_elapsed_p, &shm_db_total_elapsed,
	  sizeof( struct timeval));
  memcpy( overhead_elapsed_p, &shm_overhead_total_elapsed,
	  sizeof( struct timeval));
}


/**
 *  Private utility fns.
 */


/**
 *  All steps needed before calling one of the shmclient API.
 */
void shmclient_call_prepare()
{
  sigset_t all_sigs_set;

  gettimeofday( &shm_overhead_this_call_start, NULL);

  maybe_sleep();

  /**
   *  Set sigmask to block all signal handling whilst shmclient API is
   *  in effect: the shmclient code has a bug where it doesn't handle
   *  interrupts well.
   */
  sigfillset( &all_sigs_set);

  if( sigprocmask( SIG_SETMASK, &all_sigs_set, &old_set) == -1)
    perror( "sigprocmask");

  gettimeofday( &shm_db_this_call_start, NULL);
}


/**
 *  All steps needed after calling one of the shmclient API.
 */
void shmclient_call_recover()
{
  struct timeval db_this_call_end, overhead_this_call_end;
  struct timeval db_this_call_elapsed_time, overhead_this_call_elapsed_time;

  gettimeofday( &db_this_call_end, NULL);

  /**
   *  Restore signal mask to previous settings.
   */
  if( sigprocmask( SIG_SETMASK, &old_set, NULL) == -1)
    perror( "sigprocmask");

  gettimeofday( &overhead_this_call_end, NULL);

  timersub( &db_this_call_end, &shm_db_this_call_start,
	    &db_this_call_elapsed_time);
  timeradd( &shm_db_total_elapsed, &db_this_call_elapsed_time,
	    &shm_db_total_elapsed);

  timersub( &overhead_this_call_end, &shm_overhead_this_call_start,
	    &overhead_this_call_elapsed_time);
  timeradd( &shm_overhead_total_elapsed, &overhead_this_call_elapsed_time,
	    &shm_overhead_total_elapsed);
}



/**
 *  We try to give up some time to allow other shm clients a bite of
 *  the pie.
 */
void maybe_sleep()
{
  if( be_nice_delay == 0)
    return;

  if( ++be_nice_counter % BE_NICE_COUNT == 0) {
    be_nice_counter = 0;
    usleep( be_nice_delay);
  }
}



/**
 *  Initialise for the retry loop
 */
void _retry_db_operation_init( int *try_p, int *rc_p)
{
  *try_p = 0;

  // This is to silence a compiler warning
  *rc_p = 0;
}


/**
 *  The test condition for the retry loop.  This also includes
 *  preparation for the actual DB call.
 */
int _retry_db_operation_test( const char *name, int try, int rc, md_id_t id)
{
  int sleep_time, retries_after_warning;

  if( (try != 0 && rc != SCL_TIMEOUT) ||
      (max_retries > 0 && try == max_retries)) {
    if( rc < 0)
      fprintf( stderr, "Failed %s (for %s) : %d\n", name, 
	       mdIsNullID( id) ? "root ID" : mdStringID( id),
	       rc);

    else
      if( try > TRY_EMIT_WARNING)
	fprintf( stderr, "Succeeded %s (for %s) after %u attempts.\n",
		 name, mdIsNullID( id) ? "root ID" : mdStringID( id), try);

    return 0;  /* exit loop. */
  }

  if( try == TRY_EMIT_WARNING)
    fprintf( stderr, "WARNING: have attempted %s (for %s) %u times, still trying...\n",
	     name, mdIsNullID( id) ? "root ID" : mdStringID( id), try);
  else if( try > TRY_EMIT_WARNING) {
    retries_after_warning = try - TRY_EMIT_WARNING;

    if( retries_after_warning % TRY_EMIT_STILL_TRYING == 0)
      fprintf( stderr, "After %u tries, continuing to attempt %s (for %s)...\n",
	       try, name, mdIsNullID( id) ? "root ID" : mdStringID( id));
  } 

  /* If dbservers are having problems we should try to be kind */
  if( try >= TRY_EMIT_WARNING) {
    sleep_time = 1 + try - TRY_EMIT_WARNING;

    sleep( sleep_time < MAX_SLEEP_TIME ? sleep_time : MAX_SLEEP_TIME);
  }

  /* Prepare for calling database */
  shmclient_call_prepare();

  return 1; /* should loop */
}


/**
 *  The incremental operator for the retry loop.  This also includes
 *  the recovery from the DB call.
 */
void _retry_db_operation_incr( int *try_p)
{
  shmclient_call_recover();

  ++*try_p;
}



