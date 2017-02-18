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
 * of the programs is strictly prohibited unless otherwiLse provided
 * in the license agreement.
 */
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/times.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>
#include "shmcom.h"
#include "md2types.h"
#include "sdef.h"
#include "md2log.h"
#include "md2scan.h"
#include "allowed.h"

#define remove_oldies

#ifndef SCL_TIMEOUT   
#define SCL_TIMEOUT  (-335)   
#endif
void dbs_get_record( MDL *mdl , reqBuffer *rb , long size  );
void dbs_lookup( MDL *mdl , reqBuffer *rb, long size  );
void dbs_lookupOnly( MDL *mdl , reqBuffer *rb , long size );
void dbs_mkdir( MDL *mdl , reqBuffer *rb , long size );
void dbs_rmdir( MDL *mdl , reqBuffer *rb , long size );
void dbs_modflags( MDL *mdl , reqBuffer *rb , long size );
void dbs_setattr( MDL *mdl , reqBuffer *rb , long size );
void dbs_setattrs( MDL *mdl , reqBuffer *rb , long size );
void dbs_getattr( MDL *mdl , reqBuffer *rb , long size );
void dbs_readdir( MDL *mdl , reqBuffer *rb , long size );
void dbs_mkfile( MDL *mdl , reqBuffer *rb , long size );
void dbs_rmfile( MDL *mdl , reqBuffer *rb , long size );
void dbs_rename( MDL *mdl , reqBuffer *rb , long size );
void dbs_makelink( MDL *mdl , reqBuffer *rb , long size );
void dbs_readlink( MDL *mdl , reqBuffer *rb , long size );
void dbs_readdata( MDL *mdl , reqBuffer *rb , long size );
void dbs_writedata( MDL *mdl , reqBuffer *rb , long size );
void dbs_setsize( MDL *mdl , reqBuffer *rb , long size );
void dbs_setperm( MDL *mdl , reqBuffer *rb , long size );
void dbs_getroot( MDL *mdl , reqBuffer *rb , long size );
void dbs_truncate( MDL *mdl , reqBuffer *rb , long size );
void dbs_addtodir( MDL *mdl , reqBuffer *rb , long size );
void dbs_rmfromdir( MDL *mdl , reqBuffer *rb , long size );
void dbs_rmfromdirpos( MDL *mdl , reqBuffer *rb , long size );
void dbs_chparent( MDL *mdl , reqBuffer *rb , long size );
void dbs_delobject( MDL *mdl , reqBuffer *rb , long size );
void dbs_forcesize( MDL *mdl , reqBuffer *rb , long size );
void dbs_i_command( MDL *mdl , reqBuffer *rb , long size );
void dbs_get_chain( MDL *mdl , reqBuffer *rb , long size );
void dbs_find_id( MDL *mdl , reqBuffer *rb , long size );
void dbs_mod_link( MDL *mdl , reqBuffer *rb , long size );

int dbs_execute_command( int argc , char * argv[] , 
                         int * replyLen , char *answer , MDL *mdl ) ;
int dbs_list_access_bin( char * answer , int * replyLen ) ;
int dbs_list_access( char * answer , int * replyLen ) ;
int dbs_backup( MDL *mdl , reqBuffer *rb ,  char *dbPath , int modified );
int dbs_addToEnv( char *name );
char *  db_stringHeader(  md_auth *a );
int dbs_mergePermissions( md_permission *res , md_permission in , md_auth *auth );

void  installLevelMask(  MDL *mdl , char * levelMask );

typedef void (*dbs_func)(MDL *mdl , reqBuffer *rb , long size ) ;

#define  SWF_WRITE   (1<<2)

#define SWF_SHUTDOWN   (-18818)

typedef  struct {
     int       funcNumber ;
     dbs_func  jump ;
   long long   okCounter ;
     int       flags ;
}  dbs_switch ;

typedef struct dbs_statistics_ {

   unsigned long   writes ;
   long     files , directories;

}dbs_statistics ;

static char * dbs_switch_names[] = {
   "getroot"  , "get_record" , "getattr"   , "lookup" ,
   "mkdir"    , "setattr"    , "rmdir"     , "readdir",
   "mkfile"   , "rmfile"     , "rename"    , "mklink" ,
   "readlink" , "readdata"   , "writedata" , "setsize" ,
   "setperm"  , "truncate"   , "rmfromdir" , "addtodir" ,
   "chparent" , "delobject"  , "forcesize" , "NULL" ,
   "looponly" , "command"    , "get_chain" , "find_id" ,
   "mod_link" , "setattrs"  , "rmfromdirpos" , "mod_flags"

} ;
static dbs_switch  dbSwitchboard[] = {

{ RBR_GETROOT      , dbs_getroot     ,0, 0 } ,
{ RBR_GET_RECORD   , dbs_get_record  ,0, 0 } ,
{ RBR_GET_ATTR     , dbs_getattr     ,0, 0 } ,
{ RBR_LOOKUP       , dbs_lookup      ,0, 0 } ,
{ RBR_MKDIR        , dbs_mkdir       ,0, SWF_WRITE } ,
{ RBR_SET_ATTR     , dbs_setattr     ,0, SWF_WRITE } ,
{ RBR_RMDIR        , dbs_rmdir       ,0, SWF_WRITE } ,
{ RBR_READDIR      , dbs_readdir     ,0, 0 } ,
{ RBR_MKFILE       , dbs_mkfile      ,0, SWF_WRITE } ,
{ RBR_RMFILE       , dbs_rmfile      ,0, SWF_WRITE } ,
{ RBR_RENAME       , dbs_rename      ,0, SWF_WRITE } ,
{ RBR_MAKELINK     , dbs_makelink    ,0, SWF_WRITE } ,
{ RBR_READLINK     , dbs_readlink    ,0, 0 } ,
{ RBR_READDATA     , dbs_readdata    ,0, 0 } ,
{ RBR_WRITEDATA    , dbs_writedata   ,0, SWF_WRITE } ,
{ RBR_SETSIZE      , dbs_setsize     ,0, SWF_WRITE } ,
{ RBR_SETPERM      , dbs_setperm     ,0, SWF_WRITE } ,
{ RBR_TRUNCATE     , dbs_truncate    ,0, SWF_WRITE } ,
{ RBR_RM_FROM_DIR  , dbs_rmfromdir   ,0, SWF_WRITE } ,
{ RBR_ADD_TO_DIR   , dbs_addtodir    ,0, SWF_WRITE } ,
{ RBR_CH_PARENT    , dbs_chparent    ,0, SWF_WRITE } ,
{ RBR_DEL_OBJECT   , dbs_delobject   ,0, SWF_WRITE } ,
{ RBR_FORCE_SIZE   , dbs_forcesize   ,0, SWF_WRITE } ,
{ RBR_BACKUP       , NULL            ,0, SWF_WRITE } ,
{ RBR_LOOKUP_ONLY  , dbs_lookupOnly  ,0, 0 } ,
{ RBR_I_COMMAND    , dbs_i_command   ,0, 0 } ,
{ RBR_GET_CHAIN    , dbs_get_chain   ,0, 0 } ,
{ RBR_FIND_ID      , dbs_find_id     ,0, 0 } ,
{ RBR_MOD_LINK     , dbs_mod_link    ,0, SWF_WRITE } ,
{ RBR_SET_ATTRS    , dbs_setattrs    ,0, SWF_WRITE } ,
{ RBR_RM_POSITION  , dbs_rmfromdirpos    ,0, SWF_WRITE } ,
{ RBR_MOD_FLAGS    , dbs_modflags    ,0, SWF_WRITE } ,
{ 0                , NULL  , 0, 0       } 


} ;


struct {
  int backup ;
  int signals ;
  int nowrite ;
} status = { 0 , 0 , 0  } ;

static char * dbName ;
static long   dbSlot ;
static char * dataBase ;
static int    externalDebugLevel = 0 ;

static char * getNameByRequestId( int id ){
   int switchNames = sizeof( dbs_switch_names ) / sizeof( dbs_switch_names[0] ) ;
   id = id - RBR_FIRST ;
   return id < 0 ? "Unknown" : id >= switchNames ? "Unknown" : dbs_switch_names[id] ;
}
void db_intr_handler( int sig )
{
  /*fprintf(stderr," Process %d got signal %d\n",getpid(),sig);*/
  switch( sig ){
    case SIGTERM :
    case SIGINT  :
       status.signals = sig ;
    break ;
    
    case SIGCHLD :
       status.backup  = 0 ;
    break ; 
  }
  return ; 
}

main( int argc , char *argv[] )
{
  key_t      key ;
  int        rc , backup , w  , fd ;
  long       rbSize ;
  SCL       *scl ;
  SCLIO     *sclio ;
  reqBuffer *rb ;
  char      *error ;
  MDL       *mdl ;
  mdRecord  *mdr ;
  char      *shmKey , *dbPath , *pnfsSetup , *logFile , *levelMask ;
  char		*regDeletion;
  char       dataBaseTmp[1024] ;
  time_t          now , lastStatTime , lastFlushTime ;
  dbs_switch     *sw ;
  dbs_statistics  dbStats ;
  md_statistics   stats ;
  long long       lastRequest , requestTime ;
  reqBackup      *md ;
  clock_t        started ;
  struct  tms    dummy_arg ;
  reqGetAttr * helper ;   
  int		rd;
  
  if( argc < 2 )goto usage ;

   if( argc == 2  ){
      pnfsSetup = "/usr/etc/pnfsSetup" ;
   }else{
      pnfsSetup = argv[2] ;
   }
  dbName = argv[1] ;
  
  if( dbs_addToEnv( pnfsSetup ) ){
     fprintf(stderr," Sorry, couldn't add %s to environment\n",pnfsSetup);
     exit(2);
  }

  if( ( logFile = getenv( "dbserverLog" ) ) == NULL ){
     fprintf(stderr," Sorry, couldn't find dbserverLog in %s\n",pnfsSetup);
     md2pOpen( "/dev/null" , 0 ) ;
     md2pNewLevel( 0 ) ; 
  }else{
     md2pOpen( logFile , 0 ) ;
     md2pNewLevel( md2pMODINFO ) ; /* only interested in modifications */
     md2pNewLevel( md2pMOREINFO ) ; 
  }
  md2pPrintf(md2pMaxLevel,"%s - start\n", db_stringHeader(NULL));
  if( ( dataBase = getenv( "database" ) ) == NULL ){
     md2pPrintf(md2pMaxLevel,"%s - Sorry, couldn't find database in %s\n",
                db_stringHeader(NULL),pnfsSetup);
     exit(1);
  }
  sprintf(dataBaseTmp , "%s/databases" , dataBase ) ;
  if( dbs_addToEnv( dataBaseTmp ) ){
     md2pPrintf(md2pMaxLevel,"%s - Sorry, couldn't add %s to environment\n",
                db_stringHeader(NULL),dataBaseTmp);
     exit(1);
  }
  if( ( dbPath = getenv(  dbName  ) ) == NULL ){
     md2pPrintf(md2pMaxLevel,"%s - Sorry, couldn't find %s in %s\n",
                db_stringHeader(NULL),dbName,dataBaseTmp);
     exit(1);
  }
  
  if( ( shmKey = getenv( "shmkey" ) ) == NULL )shmKey="1122" ;
  /*
   * initiate the database md2X routines. for the dynamic part we
   * need an initialization mainly to find the modules.conf file.
   */
  if( md2Init( getenv ) ){
     md2pPrintf(md2pMaxLevel,"%s - Can't init Database %s(%s)\n",
        db_stringHeader(NULL),dbName,dbPath) ;
     exit(2);
  } 
  /* 
   * try to open the database
   */
  if ( (regDeletion = getenv("registerDeletion") ) == NULL)
		rd = 0;
  else if (strcmp(regDeletion, "yes")==0 || strcmp(regDeletion, "on")==0)
	  rd = 1;
  else 
	  rd = 0;
  if( ! ( mdl = md2OpenReadWrite( dbPath , &rc, rd ) ) ){
     md2pPrintf(md2pMaxLevel,"%s - Can't open Database (read/write)%s(%s) : %d(%d)\n",
                db_stringHeader(NULL),dbName , dbPath , rc ,errno);
     if( ! ( mdl = md2OpenReadOnly( dbPath , &rc ) ) ){
        md2pPrintf(md2pMaxLevel,"%s - Can't open Database (read/only)%s(%s) : %d(%d)\n",
                   db_stringHeader(NULL),dbName , dbPath , rc ,errno);
        exit(1);
     }
  }
  /*
   * determine the database id
   */
  if( rc = md2GetDbId( mdl , &dbSlot ) ){
       md2pPrintf(md2pMaxLevel,"%s - Can't determine DB ID: %d(%d)\n",
                db_stringHeader(NULL),rc ,errno);
       md2Close( mdl ) ;
       exit(1);
  }
  sscanf( shmKey , "%x" , &key ) ; 
  /*
   * try to get the levelmask ( no problem if not )
   */
  if( ( levelMask = getenv( "levelmask" ) ) != NULL ){
      installLevelMask( mdl , levelMask ) ;
  }

                
/*
 *   dive away
#define DEBUG
 */
#ifndef DEBUG
	if (fork()) exit(0);
/*
	close(0);
	close(1);
	close(2); */
        freopen("/dev/null", "r", stdin);
        freopen("/dev/null", "a", stdout);
        freopen("/dev/null", "a", stderr);


#ifdef solaris
        setpgrp() ;
#else
	if ((fd = open("/dev/tty", 2)) >= 0) {
		ioctl(fd, TIOCNOTTY, (char *)0);
		(void) close(fd);
	}
#endif

#endif 
/* endif of DEBUG */


  /* ------------------------------------------------------------ */
  {
     /* handle the signals */
     struct sigaction newSigAction ;
     
     newSigAction.sa_handler = db_intr_handler ;
     newSigAction.sa_flags   = 0 ;
     sigemptyset(&newSigAction.sa_mask);
     /*sigaddset(&newSigAction.sa_mask,SIGBUS); */
     sigaction( SIGTERM , &newSigAction , NULL ) ;
     sigaction( SIGINT  , &newSigAction , NULL ) ;
     sigaction( SIGHUP  , &newSigAction , NULL ) ;
     sigaction( SIGCHLD , &newSigAction , NULL ) ;

  }
  /* ------------------------------------------------------------ */
  /* 
   *  open the connection to the shm pinboard
   */
  md2pPrintf(md2pMaxLevel|md2pW,"%s - Trying to connect to Key %x slot %d\n",
                db_stringHeader(NULL),key , dbSlot);
  if( ! ( scl = sclServerOpen(  key , dbSlot , &rc ) ) ){
       error = sclError( "sclServerOpen" , rc ) ;
       md2pPrintf(md2pMaxLevel,"%s - Problem : %s(%d)\n",
                db_stringHeader(NULL),error , errno );
       md2Close( mdl ) ;
       exit(1);
  }
  /* ------------------------------------------------------------ */
  {
#ifdef use_debug_printout
     char filename[1024] ;
     sprintf( filename , "/tmp/%s-%d.log" , dbName , getpid() ) ;
     dbgprintOpen( filename ) ;
#endif    
  } 
  
  /* ------------------------------------------------------------ */
  backup = 0 ;
  w      = 0 ;
  time( &now );
  lastStatTime = lastFlushTime = now ;
  lastRequest  = 0 ;
  memset( (char*) &dbStats , 0 , sizeof( dbStats ) ) ;
  while(1){
        /*
         * and this is the incredible server loop
         */
        dbgprintRewind() ;
        sclio = sclServerWait( scl , 10 , &rc ) ;
        if( ! sclio ){
            if( rc == SCL_TIMEOUT ){
               
               if( status.signals ){
                 md2pPrintf(md2pMaxLevel,"%s - sclServerWait : Signal %d\n",
                    db_stringHeader(NULL),status.signals);
                 break ;
               }
               md2Flush( mdl , 0 ) ;
               continue ;
            }else{
               md2pPrintf(md2pMaxLevel,"%s - sclServerWait : Problem %d(%d)\n",
                db_stringHeader(NULL),rc,errno);
               break ;
            }
        }
        rb     = (reqBuffer*) sclioBuffer(sclio);
        rbSize = sclioSize(sclio);
        md     = ( reqBackup *)&(rb->data[0]) ;
#ifdef neverDefined
        showReqBufferHead(rb);
        { int i ;
            md2pPrintf(md2pMaxLevel," Version : %d\n",(rb)->version);
            md2pPrintf(md2pMaxLevel," Request : 0x%x\n",(rb)->request);
            md2pPrintf(md2pMaxLevel," Status  : 0x%x\n",(rb)->status);
            md2pPrintf(md2pMaxLevel," Answer  : 0x%x(%d)\n",(rb)->answer,(rb)->answer);
            md2pPrintf(md2pMaxLevel," Size    : %d\n",(rb)->size);
            md2pPrintf(md2pMaxLevel," Data    : ");
            for(i=0;i<16;i++)printf("%2.2X ",(rb)->data[i]);printf("\n");
        }
#endif
        /*
         * get time of arrival
         */
        requestTime =  ( ((long long)(md->auth.timestamp.tv_sec)) << 32 ) |
                       ( md->auth.timestamp.tv_usec      )   ;
        
        if( status.backup ) {
            rb -> status = SRB_ERROR ;
            rb -> answer = 444  ;
        }else{ 

          if( ( rb -> request < RBR_FIRST ) || ( rb -> request > RBR_LAST ) ){
              rb -> status = SRB_ERROR ;
              rb -> answer = 12345  ;  
          }else{
              sw = &dbSwitchboard[rb->request-RBR_FIRST] ;      
              if( sw -> jump != NULL ){
                if( ( sw -> flags & SWF_WRITE ) && status.nowrite ){
                   rb -> status = SRB_ERROR ;
                   rb -> answer = MDEnotAllowed  ;  
#ifdef remove_oldies
                }else if( ( sw -> flags & SWF_WRITE ) &&
                          ( requestTime < lastRequest )   ){
                   rb -> status = SRB_ERROR ;
                   rb -> answer = SCL_TIMEOUT  ;  
                   md2pPrintf(md2pMaxLevel,
                             "%s - Request %d removed caused by late arrival \n",
                             db_stringHeader(&(md->auth)),rb->request);
#endif
                }else{
		  started = times( &dummy_arg ) ; 
                  (*sw->jump)( mdl , rb , rbSize ) ; 
                  if( ( ( sw -> flags & SWF_WRITE ) && ( externalDebugLevel > 0 ) ) ||
		      ( externalDebugLevel > 1 ) ){
	      
		        md2pPrintf(md2pMaxLevel,"%s - %s(%d) ",
			           db_stringHeader(&(md->auth)),
				   getNameByRequestId(rb->request),
				   rb->request);
                        helper = (reqGetAttr*)md ;   
                        md2pPrintf(md2pMaxLevel,"%s ticks = %d\n",mdStringID(helper->id),(int)(times(&dummy_arg)-started));
		  }	
		  
                  sw -> okCounter ++ ;
                  if( rb -> status == SRB_OK ){
                   switch( rb -> request ){
                     case RBR_MKDIR  : 
                         dbStats.directories ++  ; 
                         break ;
                     case RBR_RMDIR  : 
                         dbStats.directories --  ; 
                         break ;
                     case RBR_MKFILE : 
                     case RBR_MAKELINK : 
                         dbStats.files ++ ; 
                         break ;
                     case RBR_RMFILE :                      
                         dbStats.files -- ; 
                         break ;
                   }
                  }else{
                     if( rb -> answer == SWF_SHUTDOWN ){
                       status.signals = - SWF_SHUTDOWN ;
                     }
                  }
                  if( sw -> flags & SWF_WRITE ){
                     dbStats.writes ++ ;
                     lastRequest = requestTime ;
                  }
                }
              }
          }
        }
        
        rc = sclServerReady( scl , sclio );
        if(rc){
           md2pPrintf(md2pMaxLevel,"%s - sclServerReady : Problem %d(%d)\n",
                db_stringHeader(NULL),rc,errno);
           break ;
        }
        if( status.signals ){
           md2pPrintf(md2pMaxLevel,"%s - (PID %d) Got Signal %d\n",
                db_stringHeader(NULL),getpid(),status.signals);
           break;
        }
        
        /* needful things            */
        time( &now ) ;
        if( ( now - lastFlushTime ) > 30 ){
           /* the database flush                     */
           md2Flush( mdl , 0 ) ;
           lastFlushTime = now ;
        }else if( ( now - lastStatTime ) > 5*60 ){
          
            if( md2GetRootStatistics( mdl , &stats ) )continue ;
               stats.dirObjects  +=  dbStats.directories ;
               stats.fileObjects +=  dbStats.files ;
            if( md2SetRootStatistics( mdl , &stats ) )continue ;
            memset( (char*) &dbStats , 0 , sizeof( dbStats ) ) ;

           lastStatTime = now ;
        }
  }  
  (void)md2GetRootStatistics( mdl , &stats ) ;
  stats.dirObjects  +=  dbStats.directories ;
  stats.fileObjects +=  dbStats.files ;
  (void)md2SetRootStatistics( mdl , &stats ) ;
  md2pPrintf(md2pMaxLevel,"%s - stop\n",db_stringHeader(NULL));
  sclServerClose( scl );
  md2Close( mdl ) ;
  dbgprintClose();
  exit(0) ;
   
 usage : 
   printf( " USAGE : %s <dbName>  [<pnfsSetup>]\n" , argv[0] ) ;
   
   exit(1);
}
#define iProblem(code,msg) { rc_code = (code) ; rc_msg = (msg) ; goto BAD ; }
int dbs_execute_command( int argc , char * argv[] , 
                         int * replyLen , char *answer , MDL *mdl ) 
{
   int rc_code = 0 ;
   char * rc_msg ;
   *replyLen = 0 ;
   if( argc < 1 )iProblem(-24,"Not enough arguments") ;
   
   if( ! strcmp( argv[0] , "test" ) ){
     int rc = 0 ;
     if( argc < 3 )iProblem( -1 , "Usage : test <text> <code>" ) ;
     strcpy( answer , argv[1] ) ;
     *replyLen = strlen( answer ) ;
     sscanf( argv[2] , "%d" , &rc ) ;
     return rc ;
   }else if( ! strcmp( argv[0] , "fs" ) ){
      if( argc < 2 )iProblem(-24,"Not enough arguments for 'fs'") ; ;
      return md2Ioctl( mdl , "io" , argc-1 , argv+1 , replyLen , answer ) ;
   }else if( ! strcmp( argv[0] , "disablewrite" ) ){
      if( status.nowrite )iProblem(-105,"Already disabled") ;
      status.nowrite = 1 ;
      md2Flush( mdl , 1 ) ;
      return 0 ;
   }else if( ! strcmp( argv[0] , "enablewrite" ) ){
      if( ! status.nowrite )iProblem(-106,"Already disabled") ;
      status.nowrite = 0 ;
      return 0 ;
   }else if( ! strcmp( argv[0] , "getpid" ) ){
      sprintf(answer,"%d",getpid());
      *replyLen = strlen( answer )+1 ;
      return 0 ;
   }else if( ! strcmp( argv[0] , "shutdown" ) ){
      sprintf(answer,"%d",getpid());
      *replyLen = strlen( answer ) ;
      return SWF_SHUTDOWN ;
   }else if( ! strcmp( argv[0] , "listaccess" ) ){
      
      return dbs_list_access( answer , replyLen ) ;
      
   }else if( ! strcmp( argv[0] , "listaccessbin" ) ){
      
      return dbs_list_access_bin( answer , replyLen ) ;
      
   }else if( ! strcmp( argv[0] , "debuglevel" ) ){
      int waitTime ;
      if( argc != 2 ){
         sprintf(answer,"failed argc=%d",argc);
         *replyLen = strlen( answer ) ;
         return -1 ;
      }
      
      sscanf( argv[1] , "%d" , &externalDebugLevel ) ;
      return 0 ;
   }else if( ! strcmp( argv[0] , "wait" ) ){
      int waitTime ;
      if( argc != 2 ){
         sprintf(answer,"failed argc=%d",argc);
         *replyLen = strlen( answer ) ;
         return -1 ;
      }
      
      sscanf( argv[1] , "%d" , &waitTime ) ;
      sleep( waitTime ) ;
      return 0 ;
   }else{
      iProblem( -24 ,"Command not found") ;
   }
   return 0 ;
   
   BAD :
      strcpy( answer , rc_msg ) ;
      *replyLen = strlen(answer) ;
      return rc_code ;
}
int dbs_list_access( char * answer , int * replyLen ) {
   int rc = 0 ;
   int switchSize  = sizeof( dbSwitchboard ) / sizeof( dbSwitchboard[0] ) ;
   int switchNames = sizeof( dbs_switch_names ) / sizeof( dbs_switch_names[0] ) ;
   int len , i ;
   char  * cursor ;
   time_t now ;
   
   len    = switchSize < switchNames ? switchSize : switchNames ;   
   cursor = answer ;
   time( &now ) ;
   
   sprintf( cursor , "%s=%ld\n" , dbName , dbSlot) ;
   cursor += strlen( cursor ) ; 
   sprintf( cursor , "time=%ld\n" , now ) ;
   cursor += strlen( cursor ) ; 
   for( i = 0 ; i < len ; i ++ ){
   
      sprintf( cursor , "%s=%lld\n" ,
               dbs_switch_names[i] ,
               dbSwitchboard[i].okCounter ) ;
      cursor += strlen( cursor ) ;
   }
   *replyLen = strlen( answer ) ;
   return 0 ;
}
int dbs_list_access_bin( char * answer , int * replyLen ) {
   int rc = 0 ;
   int switchSize  = sizeof( dbSwitchboard ) / sizeof( dbSwitchboard[0] ) ;
   long long * longAnswer = ( long long *) answer ;
   int  i ;
   char  * cursor ;
   time_t now ;
      
   for( i = 0 ; i < switchSize ; i ++ ){   
      longAnswer[i+1]  = dbSwitchboard[i].okCounter  ;
   }
   longAnswer[0] = (long long)0 ;
   time( (time_t*)longAnswer ) ;

   *replyLen = (switchSize+1)*sizeof( long long )  ;
   return 0  ;
}
int dbs_addToEnv( char *name )
{
  char string[1024] , *env ;
  FILE *f ;
  int len ;
  
   if( (  ( f = fopen( name , "r" ) ) == NULL ) ){
     fprintf(stderr," Sorry, %s not found : %d\n" , name , errno ) ;
     return -1 ;
   }else{
      while( fgets( string , 1024 , f ) != NULL ){
         if( ( *string == '#' ) ||
             ( ( len = strlen( string ) ) < 2 ) )continue ;
         string[len-1] = '\0' ;
         if( ( env = malloc( len + 16 ) ) == NULL ){
            fprintf(stderr," Malloc failed(exit)\n") ;
            exit(1) ;
         }
         strcpy( env , string ) ;
         md2pPrintf(md2pMaxLevel,"%s - %s\n",db_stringHeader(NULL),env);
         putenv( env ) ;
      }
   }
   fclose(f) ;
   return 0;
}
#define MaxCopySize  (64*1024)

int dbs_backup( MDL *mdl , reqBuffer *rb , char * dbPath , int modified )
{
  int rc  ;
  static char save[1024] , db[1024] ;
  static int fdOut , fdIn  ;
  
  if( dbPath ){ /* backup init */
  
     reqBackup *md = ( reqBackup *)&(rb->data[0]) ;

     printf( " Starting Backup from %s to %s\n" , dbPath , md -> name ) ;
     
     if( ( ! modified ) && ( ! md -> force ) ){ rc = 555 ; goto theEnd ;};
     
     strcpy( save , md -> name ) ;
     strcpy( db , dbPath ) ;
     
     rc = 0 ;
     if( ( fdIn = open( db , O_RDONLY  ) ) < 0 ){
        fprintf(stderr," Sorry, can't open %s (%d)\n",db, errno );
        rc = errno ;
        goto theEnd ;
     }
     if( ( fdOut = open( save , O_CREAT | O_EXCL , 0600 ) ) < 0 ){
        fprintf(stderr," Sorry, can't open %s (%d)\n",save, errno );
        close( fdIn ) ;
        rc = errno ;
        goto theEnd ; 
     }


    theEnd :
     if( rc  ){             
        rb -> status = SRB_ERROR ;
        rb -> answer = rc ;
        return 0 ;
     }else{
        rb -> status = SRB_OK ;
        rb -> size   = sizeof( reqBackup ) ;
        return 1 ;
     }
     
  }else{ /* the real backup */
    char *buf ; long c , w ;
    
      if( ( fdIn < 0 ) || ( fdOut < 0 ) )return -1 ;
      md2Flush( mdl , 0 ) ;
      if( ( buf = malloc( MaxCopySize ) ) == NULL ){
         while( 
                ( ( c = read( fdIn , buf , MaxCopySize ) ) > 0 ) &&
                ( ( w = write( fdOut , buf , c ) ) == c ) 
               ) ;
      }
      if( c < 0 ){
        fprintf( stderr, " Read of %s failed with %d\n",db,errno);
      }else if( w != c ){
        fprintf(stderr," Write of %s failed with %d\n",save,errno);
      }
      close( fdIn );
      close( fdOut ) ;
      free( buf );
      if( ( c < 0 ) || ( w != c ) )unlink( save ) ;
      
      return 0 ;
  }
}
#define MX_ARGV   (32)
void dbs_i_command( MDL *mdl , reqBuffer *rb , long size )
{
  int rc , argc , i , l ;
  char *argv[MX_ARGV] , * cur ;
  long long tmp[1024] ; /* needs to be 8 aligned */
  int  replyLen = sizeof( tmp ) ;
  reqICommand *md = ( reqICommand *)&(rb->data[0]) ;

  md2pPrintf(md2pMODINFO,"%s - i_command ", db_stringHeader(NULL) );
  md2pPrintf(md2pMODINFO,"size=%d;", md->size );
  md2pPrintf(md2pMODINFO,"command=%-20s\n", md->command );

  for( i = 0 , cur = md -> command ; i < (MX_ARGV-1) ; i++ ){
  
      argv[i] = cur ;
      l = strlen( cur ) ;
      if( l <= 0 )break ;
      cur += ( l + 1 ) ;
  }
  argv[i] = NULL ;
  argc  = i ;
  
  rc = dbs_execute_command( argc , argv , 
                            &replyLen , (char *)tmp , mdl ) ;
  
  /*
   * rc == 0 : command is stdout
   * rc != 0 : command is stderr
   */
  memcpy( md -> command , tmp , replyLen ) ;
  md -> size   = replyLen ;
  rb -> size   = sizeof( reqICommand ) ;
  rb -> answer = rc ;
  if( rc  != 0 ){             
    rb -> status = SRB_ERROR ;
  }else{
    rb -> status = SRB_OK ;
  }
  return ;
}
void dbs_get_record( MDL *mdl , reqBuffer *rb , long size )
{
  int rc ;
  mdRecord *mdr = (mdRecord *)&(rb -> data[0]);
  
               mdr = (mdRecord *)&(rb -> data[0]);
               if( rc = md2GetRecord( mdl , mdr , 0 ) ){
                  rb -> status = SRB_ERROR ;
                  rb -> answer = rc ;
               }else{
                  rb -> status = SRB_OK ;
                  rb -> size   = sizeof( *mdr ) ;
               }
   

}
void dbs_setattr( MDL *mdl , reqBuffer *rb , long size )
{
  int rc ;
  reqSetAttr *md = ( reqSetAttr *)&(rb->data[0]) ;
  md_attr attr ;
  
  rc = md2GetAttribute( mdl , md->id ,mdGetLevel(md -> perm),  &attr ) ;
  if(rc)goto theEnd ;
  
  if( md->attr.mst_size != md_no_size ){
     if( ! mdWriteAllowed( &attr , &md->auth ) ){ rc = MDEnotAllowed; goto theEnd ;}
  }else{  
     if( ! mdSetAttrAllowed( &attr , &md->auth ) ){ rc = MDEnotAllowed; goto theEnd ;}
  }
  rc = md2ModifyUnixAttr( mdl ,md->id , mdGetLevel(md -> perm), &(md->attr) );

  if( rc )goto theEnd ;
  rc = md2GetUnixAttr( mdl , md->id , mdGetLevel(md -> perm), &( md -> attr) ) ;

theEnd:

  if( rc  ){             
    rb -> status = SRB_ERROR ;
    rb -> answer = rc ;
  }else{
     rb -> status = SRB_OK ;
     rb -> size   = sizeof( reqSetAttr ) ;
  }
}
void dbs_modflags( MDL *mdl , reqBuffer *rb , long size )
{
  int rc ;
  reqModFlags *md = ( reqModFlags *)&(rb->data[0]) ;
  
  rc = md2ModFlags( mdl , md->id , &(md->flags) , md->mask  ) ;

theEnd:

  if( rc  ){             
    rb -> status = SRB_ERROR ;
    rb -> answer = rc ;
  }else{
     rb -> status = SRB_OK ;
     rb -> size   = sizeof( reqModFlags ) ;
  }
}
void dbs_setattrs( MDL *mdl , reqBuffer *rb , long size )
{
  int rc ;
  reqSetAttr *md = ( reqSetAttr *)&(rb->data[0]) ;
  md_attr attr ;
  
  rc = md2GetAttribute( mdl , md->id ,mdGetLevel(md -> perm),  &attr ) ;
  if(rc)goto theEnd ;
  
  if( mdIsSpecialID(md->id) && ( mdpGetSpecial(md->perm) == MDO_REQ_IGNORE ) ){
     memcpy( &(md->attr) , &attr , sizeof(attr) ) ;
     mdClearSpecialID(md->id) ;
     goto theEnd ;
  }
  if( md->attr.mst_size != md_no_size ){
     if( ! mdWriteAllowed( &attr , &md->auth ) ){ rc = MDEnotAllowed; goto theEnd ;}
  }else{  
     if( ! mdSetAttrAllowed( &attr , &md->auth ) ){ rc = MDEnotAllowed; goto theEnd ;}
  }
  rc = md2ModifyUnixAttrs( mdl ,md->id , mdGetLevel(md -> perm), &(md->attr) );

  if( rc )goto theEnd ;
  rc = md2GetUnixAttr( mdl , md->id , mdGetLevel(md -> perm), &( md -> attr) ) ;

theEnd:

  if( rc  ){             
    rb -> status = SRB_ERROR ;
    rb -> answer = rc ;
  }else{
     rb -> status = SRB_OK ;
     rb -> size   = sizeof( reqSetAttr ) ;
  }
}
void dbs_mod_link( MDL *mdl , reqBuffer *rb , long size )
{
  int rc , nlink ;
  reqSetAttr *md = ( reqSetAttr *)&(rb->data[0]) ;
  md_attr attr ;
  
  rc = md2GetAttribute( mdl , md->id ,mdGetLevel(md -> perm),  &attr ) ;
  if(rc)goto theEnd ;
  
  if( ! mdSetAttrAllowed( &attr , &md->auth ) ){ 
     rc = MDEnotAllowed; 
     goto theEnd ;
  }
  nlink = md->attr.mst_nlink ;
  rc = md2ModifyLinkCount( mdl ,md->id , &nlink );
  md->attr.mst_nlink = nlink ;

theEnd:

  if( rc  ){             
    rb -> status = SRB_ERROR ;
    rb -> answer = rc ;
  }else{
     rb -> status = SRB_OK ;
     rb -> size   = sizeof( reqSetAttr ) ;
  }
}
void dbs_forcesize( MDL *mdl , reqBuffer *rb , long size )
{
  int rc ;
  reqForceSize *md = ( reqForceSize *)&(rb->data[0]) ;
  md_unix attr ;
  
  rc = md2ForceSize( mdl , md->id ,mdGetLevel(md -> perm),  md -> size ) ;

theEnd:

  if( rc  ){             
    rb -> status = SRB_ERROR ;
    rb -> answer = rc ;
  }else{
     rb -> status = SRB_OK ;
     rb -> size   = sizeof( reqForceSize ) ;
  }
}
void dbs_delobject( MDL *mdl , reqBuffer *rb , long size )
{
  int rc ;
  reqDelObject *md = ( reqDelObject *)&(rb->data[0]) ;

   /*mdpSetEntryOnly( md -> item.perm ) ;*/
  if( md -> type == RBR_OBJ_FILE ){
    rc = md2DeleteFile( mdl , md -> id ) ;
  }else if( md -> type == RBR_OBJ_DIR ){
    rc = md2DeleteDirectory( mdl , md -> id ) ;  
  }else{
    rc = 2233 ;
  }

  if( rc  ){             
    rb -> status = SRB_ERROR ;
    rb -> answer = rc ;
  }else{
    rb -> status = SRB_OK ;
    rb -> size   = sizeof( reqDelObject ) ;
  }

}
void dbs_get_chain( MDL *mdl , reqBuffer *rb , long size )
{
  int rc ;
  mdRecord dir ;
  
  reqGetChain *md = ( reqGetChain *)&(rb->data[0]) ;

  dir.head.ID = md -> dir ;
  if( rc = md2GetRecord( mdl , &dir , 0 ) )goto theEnd ;
  md -> parent = dir.head.parentID ;
  if( ! mdIsNullID( md -> child ) ){  
      rc = md2FindEntryByID( mdl , md -> dir , md -> child , &md->item );
      if(rc)goto theEnd ;  
  }
   
theEnd :

  if( rc  ){             
    rb -> status = SRB_ERROR ;
    rb -> answer = rc ;
  }else{
    rb -> status = SRB_OK ;
    rb -> size   = sizeof( reqGetChain ) ;
  }

}
void dbs_addtodir( MDL *mdl , reqBuffer *rb , long size )
{
  int rc ;
  reqAddToDir *md = ( reqAddToDir *)&(rb->data[0]) ;

   /*mdpSetEntryOnly( md -> item.perm ) ;*/
   strcpy( md -> item.name , md -> name ) ;

   rc = md2ExtAddToDirectory( mdl , md->id , &md -> item );
  if( rc  ){             
    rb -> status = SRB_ERROR ;
    rb -> answer = rc ;
  }else{
    rb -> status = SRB_OK ;
    rb -> size   = sizeof( reqAddToDir ) ;
  }

}
void dbs_rmfromdir( MDL *mdl , reqBuffer *rb , long size )
{
  int rc ;
  reqRmFromDir *md = ( reqRmFromDir *)&(rb->data[0]) ;
 
   rc = md2ExtRemoveFromDirectory( mdl , md -> id , md -> name ,
                                   NULL , &md->item );
 
  if( rc  ){             
    rb -> status = SRB_ERROR ;
    rb -> answer = rc ;
  }else{
    rb -> status = SRB_OK ;
    rb -> size   = sizeof( reqRmFromDir ) ;
  }
}
void dbs_rmfromdirpos( MDL *mdl , reqBuffer *rb , long size )
{
  int rc ;
  reqRemovePosition *md = ( reqRemovePosition *)&(rb->data[0]) ;
 
   rc = md2ExtRemoveFromDirectoryPosition( mdl , md -> id , md -> rmId , md->position ) ;
 
  if( rc  ){             
    rb -> status = SRB_ERROR ;
    rb -> answer = rc ;
  }else{
    rb -> status = SRB_OK ;
    rb -> size   = sizeof( reqRemovePosition ) ;
  }
}

void dbs_truncate( MDL *mdl , reqBuffer *rb , long size )
{
  int rc ;
  reqTruncate *md = ( reqTruncate *)&(rb->data[0]) ;
  md_attr attr ;
  
  rc = md2GetAttribute( mdl , md->id ,mdGetLevel(md -> perm),  &attr ) ;
  if(rc)goto theEnd ;

  if( ! mdWriteAllowed( &attr , &md->auth ) ){ rc = MDEnotAllowed; goto theEnd ;}
       
  rc = md2TruncateZero( mdl , md->id , mdGetLevel(md -> perm) ) ;

theEnd:

  if( rc  ){             
    rb -> status = SRB_ERROR ;
    rb -> answer = rc ;
  }else{
     rb -> status = SRB_OK ;
     rb -> size   = sizeof( reqTruncate ) ;
  }
}
void dbs_mkdir( MDL *mdl , reqBuffer *rb , long size )
{
  int rc ;
  reqMkdir *md = ( reqMkdir *)&(rb->data[0]) ;
  md_attr attr ;
  
  rc = md2Lookup( mdl , md->id , md->name , &(md->resId) ) ;
  if( ! rc ){ rc = MDEexists ;  goto theEnd ; }
  rc = md2GetAttribute( mdl , md->id ,mdpGetLevel( md->perm),  &attr ) ;
  if(rc)goto theEnd ;

  if( ! mdWriteAllowed( &attr , &md->auth ) ){ rc = MDEnotAllowed; goto theEnd ;}
       
  rc = md2MakeDirectory( mdl , md->id , md->name , &(md->resId) );
  if( rc )goto theEnd ;

  if( md->attr.mst_gid > 0xffff )md->attr.mst_gid = md->auth.gid ;
     
  rc = md2ModifyUnixAttr( mdl ,md->resId , mdpGetLevel( md->perm) | mdpAllLevels , &(md->attr) );
  if( rc )goto theEnd ;
  rc = md2GetUnixAttr( mdl , md->resId , mdpGetLevel( md->perm) , &( md -> attr) ) ;

theEnd:

  if( rc  ){             
    rb -> status = SRB_ERROR ;
    rb -> answer = rc ;
  }else{
     rb -> status = SRB_OK ;
     rb -> size   = sizeof( reqMkdir ) ;
  }
}
void dbs_rmdir( MDL *mdl , reqBuffer *rb , long size )
{
  int rc ;
  reqRmdir *md = ( reqRmdir *)&(rb->data[0]) ;
  md_attr attr ;
  
  rc = md2GetAttribute( mdl , md->id ,mdpGetLevel(md->perm),  &attr ) ;
  if(rc)goto theEnd ;
  
  if( ! mdWriteAllowed( &attr , &md->auth ) ){ rc = MDEnotAllowed; goto theEnd ;}
     
  rc = md2RemoveDirectory( mdl , md->id , md->name  );

theEnd:

  if( rc  ){             
    rb -> status = SRB_ERROR ;
    rb -> answer = rc ;
  }else{
     rb -> status = SRB_OK ;
     rb -> size   = sizeof( reqRmdir ) ;
  }
}
void dbs_chparent( MDL *mdl , reqBuffer *rb , long size )
{
  int rc ;
  reqChParent *md = ( reqChParent *)&(rb->data[0]) ;
  md_unix attr ;
  
  rc = md2ChangeParent( mdl , md->id , md -> parent ) ;

theEnd:

  if( rc  ){             
    rb -> status = SRB_ERROR ;
    rb -> answer = rc ;
  }else{
     rb -> status = SRB_OK ;
     rb -> size   = sizeof( reqChParent ) ;
  }
}
/*
 * dbs_rmfile
 *  -  checks rwx right of the parent directory
 *  -  looks up the name inside of the directory
 *  -  if the nlink count is larger then 1 we return mismatch
 *  -  otherwise the inform the external remove handler
 *  -  and we delete the fileentry and the file inode
*/
void dbs_rmfile( MDL *mdl , reqBuffer *rb , long size )
{
  int rc ;
  reqRmfile *md = ( reqRmdir *)&(rb->data[0]) ;
  md_attr attr ;
  md_e_dir_entry  ent ;
  md_dir_item     item ;
  md_id_t         resID ;
  md_unix unixAttributes ;
      
  md2pPrintf(md2pMODINFO,"%s - remove ", db_stringHeader(NULL) );
  
  rc = md2GetAttribute( mdl , md->id , 0 ,  &attr ) ;
  if(rc)goto theEnd ;

  
  if( ! mdWriteAllowed( &attr , &md->auth ) ){ 
       rc = MDEnotAllowed; goto theEnd ;
  }
  md2pPrintf(md2pMODINFO,"%s %s ",mdStringID(md->id),md->name) ;             
  rc = md2ExtLookupPerm( mdl , 
                        md->id ,&(md->perm) ,md->name,
                        &resID, &item  ) ;
  if(rc)goto theEnd ;
  
  if( md->id.db != resID.db ){
       rc = MDEdbMissmatch; goto theEnd ;
  }
  md2pPrintf(md2pMODINFO,"%s ",mdStringID(resID) ) ;             
  
  rc = md2GetUnixAttr( mdl , resID , 0 ,  &unixAttributes ) ;
  if(rc)goto theEnd ;

  if( ( unixAttributes.mst_nlink > 1 )  && ( unixAttributes.mst_nlink < 20000 ) ){
     rc = MDEdbMissmatch ;
     goto theEnd ;
  }
  

  memset( (char **)&ent , 0 , sizeof(ent) ) ;
  ent.dirId         = md->id ;
  ent.dirPermission = md->perm ;
  strcpy( ent.objEntry.name , md->name ) ;
  if( rc = md2ExtHandlerREMOVE( mdl , &ent ) ){
     rc = MDEnotAllowed ;
     goto theEnd ; 
  }
     
  rc = md2RemoveFile( mdl , md->id , md -> perm , md->name  );

theEnd:

  if( rc  ){             
    rb -> status = SRB_ERROR ;
    rb -> answer = rc ;
  }else{
     rb -> status = SRB_OK ;
     rb -> size   = sizeof( reqRmfile ) ;
  }
  md2pPrintf(md2pMODINFO," -> %d(%d)\n",rb->status,rb->answer) ;
}
void dbs_lookup( MDL *mdl , reqBuffer *rb , long size )
{
  int rc ;
  reqLookup *lu = ( reqLookup *)&(rb->data[0]) ;
  md_dir_item item ;
  md_attr attr ;

  if( ( ! mdpIsPrivileged( lu -> perm ) ) &&
        mdpIsLupInvisible( lu -> perm , mdpGetLevel( lu -> perm ) ) ){
        rc = MDEnotFound ;
        goto theEnd ;
  }
        
  rc = md2ExtLookupPerm( mdl ,lu->id ,&(lu->perm) ,lu->name, NULL, &(lu->item ) ) ;
  if(rc)goto theEnd ;

  dbs_mergePermissions( &(lu->perm) , lu->item.perm ,&(lu -> auth ) ) ;
  lu->item.perm = lu->perm ;
  
  if( lu -> item.ID.db != lu->id.db ){ rc = MDEdbMissmatch ; goto theEnd ; }
  
  rc = md2GetExtAttributePerm( mdl , lu -> item.ID , lu -> item.perm , &attr , NULL ) ;
  if(rc)goto theEnd ;
  memcpy((char *)&( lu -> attr),(char *)&attr.unixAttr,sizeof(md_unix)) ;
    
theEnd:

  if( rc  ){             
    rb -> status = SRB_ERROR ;
    rb -> answer = rc ;
  }else{
     rb -> status = SRB_OK ;
     rb -> size   = sizeof( reqLookup ) ;
  }
}
void dbs_lookupOnly( MDL *mdl , reqBuffer *rb , long size )
{
  int rc ;
  reqLookup *lu = ( reqLookup *)&(rb->data[0]) ;
  md_dir_item item ;
  md_attr attr ;

  if( ( ! mdpIsPrivileged( lu -> perm ) ) &&
        mdpIsLupInvisible( lu -> perm , mdpGetLevel( lu -> perm ) ) ){
        rc = MDEnotFound ;
        goto theEnd ;
  }
        
  rc = md2ExtLookupPerm( mdl ,lu->id ,&(lu->perm) ,lu->name, NULL, &(lu->item ) ) ;
  if(rc)goto theEnd ;

  dbs_mergePermissions( &(lu->perm) , lu->item.perm ,&(lu -> auth ) ) ;
  lu->item.perm = lu->perm ;
  
    
theEnd:

  if( rc  ){             
    rb -> status = SRB_ERROR ;
    rb -> answer = rc ;
  }else{
     rb -> status = SRB_OK ;
     rb -> size   = sizeof( reqLookup ) ;
  }
}
int dbs_mergePermissions( md_permission *res , md_permission in , md_auth *auth )
{
  mdpDoModification( *res , in , mdpLevelBits    , mdpModifyLevelBit ) ;
  mdpDoModification( *res , in , mdpNoIOBit      , mdpModifyNoIOBit ) ;
  mdpDoModification( *res , in , mdpNoWayBackBit , mdpModifyNoWayBackBit ) ;

  res -> high = in.high ;

}
void dbs_readdir( MDL *mdl , reqBuffer *rb , long rbSize )
{
  int rc , i , mcount ;
  reqReadDir *rd = ( reqReadDir *)&(rb->data[0]) ;
  md_dir_item item ;

  rc = md2DirOpen( mdl, rd -> id , &(rd -> cookie) ) ; 
  if( rc ) goto theEnd ;
  
  mcount = rbSize / sizeof( md_dir_item) - 1 ;
  rd -> count  = mcount < rd -> count ? mcount : rd -> count ;

  for( i = 0 ; i < rd -> count ;i++ ){
     
       if( md2DirNext(mdl,&(rd->cookie),&(rd->e[i].item)) )break ;
       rd->e[i].cookie = rd -> cookie;
  }    
  
  
  rd -> count = i ;
  md2DirClose( mdl ) ;
theEnd:

  if( rc  ){             
    rb -> status = SRB_ERROR ;
    rb -> answer = rc ;
  }else{
     rb -> status = SRB_OK ;
     rb -> size   = sizeof( reqReadDir ) ;
  }
}
void dbs_mkfile( MDL *mdl , reqBuffer *rb , long size )
{
  int rc ;
  reqMkfile *md = ( reqMkfile *)&(rb->data[0]) ;
  md_attr attr ;
  md_dir_item item ;
  
  rc = md2GetAttribute( mdl , md->id  , mdGetLevel( md -> perm ),  &attr ) ;
  if(rc)goto theEnd ;
  
  if( ! mdWriteAllowed( &attr , &md->auth ) ){ rc = MDEnotAllowed; goto theEnd ;}
    
  md -> resPerm = md -> perm ;
   
  rc = md2ExtLookupPerm( mdl , md->id , &(md -> perm) , md->name , &(md->resId)  , &item);
  if( rc == 0 ){ 
      /* object already exist  */
      /* merge the found and the incoming permissions  */
     dbs_mergePermissions( &(md -> perm) , item.perm , &(md->auth) ) ;
     item.perm = md -> perm ;
     md -> resPerm = item.perm ;
     if( 1 /* ! mdpGetSpecial(item.perm) */ ){
     
        if( md->attr.mst_size == 0 ){
           rc = md2TruncateZero( mdl , md->resId , mdGetLevel(md -> resPerm) ) ;
           if(rc)goto theEnd ;
        }
        md->attr.mst_mode = md_no_mode ; /* just to be compatible */
        rc = md2ModifyUnixAttr( mdl ,md->resId ,
                                mdGetLevel(md->resPerm) | mdpAllLevels ,
                                &(md->attr) );
        if(rc)goto theEnd ;
        
     }
                               
                               
                               
  }else if( rc ==  MDEnotFound ){
     rc = md2MakeFilePerm( mdl , md->id ,
                                 md->perm,
                                 md->name ,
                                 &(md -> resId),
                                 &(md -> resPerm) ) ;
     if(rc)goto theEnd ;
     
     if( md->attr.mst_gid > 0xffff )md->attr.mst_gid = md->auth.gid ;
     /*
     md2pPrintf(md2pMaxLevel,"%s - md->attr.mst_gid %d\n",
                db_stringHeader(NULL),md->attr.mst_gid);
     */
     /*
     
     rc = md2ModifyUnixAttr( mdl ,md->resId ,
                             mdGetLevel(md -> perm) | mdpAllLevels ,
                             &(md->attr) );
     */
     rc = md2ModifyUnixAttr( mdl ,md->resId ,
                             mdGetLevel(md -> perm)  ,
                             &(md->attr) );

     if(rc)goto theEnd ;
  }else 
    goto theEnd ;

       

  rc = md2GetUnixAttr( mdl , md->resId ,
                       mdGetLevel(md -> resPerm) , &( md -> attr) ) ;

theEnd:

  if( rc  ){             
    rb -> status = SRB_ERROR ;
    rb -> answer = rc ;
  }else{
     rb -> status = SRB_OK ;
     rb -> size   = sizeof( reqMkfile ) ;
  }
}
void dbs_rename( MDL *mdl , reqBuffer *rb , long size )
{
  int rc ;
  reqRename *rn = ( reqRename *)&(rb->data[0]) ;
  md_attr attr ;
  md_e_dir_entry  fromItem , toItem ;
  md_id_t     toID , fromID ;
  mdRecord    fromDir ;
  
  if( rn->from.id.db != rn->to.id.db ){rc= MDEnotAllowed ; goto theEnd ;}
  
  if( !( md2IsTagName( rn -> to.name ) && 
         md2IsTagName( rn -> from.name    ) ) )
      {rc= MDEnotAllowed ; goto theEnd ;}
      
  fromDir.head.ID = rn -> from.id ;
  rc = md2GetRecord( mdl , &fromDir , 0 ) ;
  if( rc ){ rc = MDEnotFound ; goto theEnd ; }
  if( fromDir.body.dirInode.attr.flags & MD_INODE_FLAG_NOMOVE ){
     rc = MDEnotAllowed ;
     goto theEnd ;
  }
  rc = md2GetAttribute( mdl , rn -> from.id ,
                       mdpGetLevel( rn -> from.perm) ,  &attr   ) ;
  if(rc){rc= MDEnotFound ; goto theEnd ;}
  
  
   
  if( ! mdWriteAllowed( &attr , &rn->auth ) ){ rc = MDEnotAllowed; goto theEnd ;}
     
  rc = md2GetAttribute( mdl , rn -> to.id ,
                       mdpGetLevel( rn -> to.perm) ,  &attr   ) ;
  if(rc){rc= MDEnotFound ; goto theEnd ;}
  
    
  if( ! mdWriteAllowed( &attr , &rn->auth ) ){ rc = MDEnotAllowed; goto theEnd ;}
     
  toItem.dirId         = rn -> to.id ; 
  toItem.dirPermission = rn -> to.perm ; 
  if( ! md2ExtLookup( mdl , rn -> to.id , rn -> to.name ,
                      &toID , &toItem.objEntry ) )
     {rc= MDEexists ; goto theEnd ;}

  fromItem.dirId         = rn -> from.id ; 
  fromItem.dirPermission = rn -> from.perm ; 
  if(  md2ExtLookup( mdl , rn -> from.id , rn -> from.name ,
                     &fromID , &fromItem.objEntry ) )
     {rc= MDEnotFound ; goto theEnd ;}

  if( mdIsEqualID( fromID , rn -> to.id ) ){
     rc = MDEnotAllowed ;
     goto theEnd ;
  }
  md2pPrintf(md2pMaxLevel,"%s - DEBUG : rename\n",db_stringHeader(NULL));
  if( ( rc = md2ExtHandlerRENAME( mdl , &fromItem , &toItem ) ) != 0 ){
     md2pPrintf(md2pMaxLevel,"%s - DEBUG : md2ExtHandlerRENAME %d\n",db_stringHeader(NULL),rc);
     rc = MDEnotAllowed ;
     goto theEnd ;
  } 
  md2pPrintf(md2pMaxLevel,"%s - DEBUG : md2ExtHandlerRENAME(2) %d\n",db_stringHeader(NULL),rc);
  strcpy( fromItem.objEntry.name , rn -> to.name ) ;
  rc = md2ExtAddToDirectory( mdl ,rn ->to.id , &fromItem.objEntry ) ;
  if( rc )goto theEnd ;
  
  rc = md2RemoveFromDirectory( mdl , rn -> from.id , rn -> from.name , &fromID ) ;
  if( rc ){rc= MDEpanic1 ; goto theEnd ;}

  rc = md2ChangeParent( mdl , fromID , rn ->to.id ) ;
  if(rc){rc= MDEpanic1 ; goto theEnd ;}

theEnd:

  md2pPrintf(md2pMaxLevel,"%s - DEBUG : md2ExtHandlerRENAME(3) %d\n",db_stringHeader(NULL),rc);
  if( rc  ){             
    rb -> status = SRB_ERROR ;
    rb -> answer = rc ;
  }else{
     rb -> status = SRB_OK ;
     rb -> size   = sizeof( reqRename ) ;
  }
}
void dbs_makelink( MDL *mdl , reqBuffer *rb , long size )
{
  int rc ;
  md_dir_item  item ;
  md_attr attr ;
  
  reqMakeLink *ml = ( reqMakeLink *)&(rb->data[0]) ;

  md2pPrintf(md2pMaxLevel,"%s - makelink %s %s;",
                db_stringHeader(NULL),mdStringID( ml -> id ) , ml -> name );
  rc = mdFindDirectoryEntry( mdl , ml->id , ml->name , &item );
  md2pPrintf(md2pMaxLevel,"find=%d;",rc);
  if( ! rc ){ rc = MDEexists ; goto theEnd ; }
  
  rc = md2GetAttribute( mdl , ml->id  , mdGetLevel( ml -> perm ),  &attr ) ;
  md2pPrintf(md2pMaxLevel,"attr=%d;",rc);
  if(rc)goto theEnd ;
  
  if( ! mdWriteAllowed( &attr , &ml->auth ) ){ rc = MDEnotAllowed; goto theEnd ;}
    
  rc = md2MakeLink( mdl , ml->id , ml->name ,
                    mdpGetLevel( ml->perm), 
                    ml->path ,  &(ml->resID)  ) ;
  md2pPrintf(md2pMaxLevel,"symlink=%d (uid=%d,gid=%d);",rc,ml->attr.mst_uid,ml->attr.mst_gid);
  if(rc)goto theEnd ;
  md2pPrintf(md2pMaxLevel,"id=%s;",mdStringID(ml->resID));
  
  rc = md2ModifyUnixAttr( mdl ,ml->resID ,
                          mdGetLevel(ml -> perm) | mdpAllLevels ,
                          &(ml->attr) );
  md2pPrintf(md2pMaxLevel,"attrRc=%d;",rc);
     
theEnd:

  if( rc  ){             
    rb -> status = SRB_ERROR ;
    rb -> answer = rc ;
  }else{
     rb -> status = SRB_OK ;
     rb -> size   = sizeof( reqMakeLink ) ;
  }
}
void dbs_readlink( MDL *mdl , reqBuffer *rb , long size )
{
  int rc ;
  reqReadLink *rl = ( reqReadLink *)&(rb->data[0]) ;

  rc = md2ReadLink( mdl , rl->id ,
                    mdpGetLevel( rl->perm) ,
                    rl -> path    ) ;
    
theEnd:

  if( rc  ){             
    rb -> status = SRB_ERROR ;
    rb -> answer = rc ;
  }else{
     rb -> status = SRB_OK ;
     rb -> size   = sizeof( reqReadLink ) ;
  }
}
void dbs_find_id( MDL *mdl , reqBuffer *rb , long size )
{
  int rc ;
  reqFindId *rd  ;
  md_attr  attr , realAttr ;
  md_dir_item item ;
  md_id_t id ;
  
  rd = ( reqFindId *)&(rb->data[0]) ;
  /*
   * check if parent exists and if we allowed to read.
   */
  rc = md2GetExtAttributePerm( mdl , rd->parentId  ,  rd -> perm ,
                               &attr ,NULL ) ;
  if(rc)goto theEnd ;

  if( ! mdReadAllowed( &attr , &rd->auth ) ){ rc = MDEnotAllowed; goto theEnd ;}
 
  id = rd -> id ; 
  mdClearSpecialID(id);

  rc = md2FindEntryByID( mdl , rd->parentId , id , &item ) ;
  if(rc)goto theEnd ;

  /*
   * this doesn't work on db jumps
   * In that case we take the attributes of the parent
   */
  rc = md2GetExtAttributePerm( mdl , rd->id  ,  rd -> perm , &realAttr ,NULL ) ;
  if(rc){  
      rc = 0 ;  
      memcpy( (char *)&(rd->attr) ,
              (char *)&attr.unixAttr ,
              sizeof( md_unix ) ) ;
  }else{
       memcpy( (char *)&(rd->attr) ,
              (char *)&realAttr.unixAttr ,
              sizeof( md_unix ) ) ;
 }
  rd->attr.mst_mtime = time(NULL);
  rd->attr.mst_mode  = 0100444 ;

  mdCopyItem( rd->data , item.name ) ;
  rd->size              = strlen( item.name )  ;
  rd->data[rd->size++]  = '\n' ;
  rd->attr.mst_size     = rd->size ;
  rd->attr.mst_sizeHigh = 0 ;
   
theEnd:

  if( rc < 0  ){             
    rb -> status = SRB_ERROR ;
    rb -> answer = rc ;
  }else{
     rb -> status = SRB_OK ;
     rb -> size   = sizeof( reqFindId ) ;
  }
}
void dbs_readdata( MDL *mdl , reqBuffer *rb , long size )
{
  int          rc , special  ;
  reqReadData *rd = ( reqReadData *)&(rb->data[0]) ;
  md_attr      attr ;

  rc = md2GetExtAttributePerm( mdl , rd->id  ,  rd -> perm ,
                               &attr ,NULL ) ;
  if(rc)goto theEnd ;
  
    memcpy( (char *)&(rd->attr) ,
            (char *)&attr.unixAttr ,
            sizeof( md_unix ) ) ;


  if( ! mdReadAllowed( &attr , &rd->auth ) ){ rc = MDEnotAllowed; goto theEnd ;}

  if( ( special = mdpGetSpecial( rd->perm ) ) )mdpUnsetNoIO( rd->perm ) ;


  if( mdIsSpecialID( rd->id ) ){
    if( special == MDO_REQ_GET_COUNTERS ){
      char answer[4*1024] ;
      int replyLen = sizeof( answer ) ;
      rc = dbs_list_access( answer , &replyLen ) ;
      if( ( rd -> offset != 0 ) || ( rd -> size < rc ) ){ 
         rc = MDEnotFound ; 
         goto theEnd ;
      }
      memcpy( rd -> data , answer , rc = replyLen ) ;
    }else if( special == MDO_REQ_GET_DATABASE ){
      char   dbFile[1024] , line[256] ;
      FILE * stream ;
      
      sprintf( dbFile , "%s/D-%04d" , dataBase , rd->id.low >> 3 ) ;
      md2pPrintf(md2pMaxLevel,"%s - trying to open (%s)\n",
                   db_stringHeader(&rd->auth),dbFile );
      
      if( ( stream = fopen( dbFile , "r" ) ) == NULL ){
         md2pPrintf(md2pMaxLevel,"%s - opening (%s) failed %d\n",
                    db_stringHeader(&rd->auth),dbFile ,errno);
         rc = MDEnotFound ; 
         goto theEnd ;
      }
      fgets( line , sizeof(line)-1  , stream ) ;
      fclose( stream ) ;
      rc = strlen( line ) ;
      if( ( rd -> offset != 0 ) || ( rd -> size < rc ) ){ 
         rc = MDEnotFound ; 
         goto theEnd ;
      }
      memcpy( rd -> data , line , rc ) ;
    }else{
      rc =  md2ReadDataPerm( mdl , rd->id , rd->perm , rd->auth.mountID ,
                             rd->data , (off_t)(rd->offset) , rd->size );     
    }
  }else{
  
     rc =  md2ReadDataPerm( mdl , rd->id , rd->perm , rd->auth.mountID ,
                            rd->data , (off_t)(rd->offset) , rd->size );
  }
  rd->size = rc ;
   
theEnd:


  if( rc < 0  ){             
    rb -> status = SRB_ERROR ;
    rb -> answer = rc ;
  }else{
     rb -> status = SRB_OK ;
     rb -> size   = sizeof( reqReadData ) ;
  }
}
void dbs_writedata( MDL *mdl , reqBuffer *rb , long size )
{
  int rc ;
  reqWriteData *rl = ( reqWriteData *)&(rb->data[0]) ;
  md_attr attr ;
  
  rc = md2GetAttribute( mdl , rl->id  ,
                       mdGetLevel( rl -> perm ),  &attr ) ;
  if(rc)goto theEnd ;
  /*
  md2pPrintf(md2pMaxLevel," check attr : uid=%d,gid=%d,mode=%d\n",
                            attr.mst_uid,attr.mst_gid,attr.mst_mode);
  md2pPrintf(md2pMaxLevel," check auth : uid=%d,gid=%d,priv=%d\n",
                            rl->auth.uid,rl->auth.gid,rl->auth.priv);
  */
  if( ! mdWriteAllowed( &attr , &rl->auth ) ){ rc = MDEnotAllowed; goto theEnd ;}
  /*
  md2pPrintf(md2pMaxLevel," check ok\n" ) ;
  
  fprintf(stderr," dbs writedata : id %s ",mdStringID( rl -> id ) );
  fprintf(stderr," perm : %s offset : %d size : %d\n",mdStringPermission(rl->perm),rl->offset,rl->size);
  if( mdpGetSpecial( rl->perm ) )mdpUnsetNoIO( rl->perm ) ;
   */
  rc =  md2WriteDataPerm( mdl , rl->id , rl->perm,
                      rl -> data , (long)(rl->offset) ,
                      rl -> size );

  if(rc<0)goto theEnd ;
  rl -> size = rc ;
  rc = md2GetUnixAttr( mdl , rl->id  ,
                       mdGetLevel( rl -> perm ),  &(rl->attr) ) ;
  if(rc)goto theEnd ;
   
theEnd:

  if( rc < 0  ){             
    rb -> status = SRB_ERROR ;
    rb -> answer = rc ;
  }else{
     rb -> status = SRB_OK ;
     rb -> size   = sizeof( reqWriteData ) ;
  }
}
#ifdef old_setsize
void dbs_setsize( MDL *mdl , reqBuffer *rb )
{
  int rc ;
  reqSetSize *rl = ( reqSetSize *)&(rb->data[0]) ;
       
  if( ( ! mdpIsNoIO( rl->perm ) ) ||  
      ( rc = md2ForceSize( mdl , rl->id ,
                           mdpGetLevel(rl-> perm ) , rl->size )  ) )
        { rc = MDEnotAllowed ; goto  theEnd ; }   
   
theEnd:

  if( rc < 0  ){             
    rb -> status = SRB_ERROR ;
    rb -> answer = rc ;
  }else{
     rb -> status = SRB_OK ;
     rb -> size   = sizeof( reqSetSize ) ;
  }
}
#else
void dbs_setsize( MDL *mdl , reqBuffer *rb , long size )
{
  int rc ;
  reqSetSize *rl = ( reqSetSize *)&(rb->data[0]) ;
       
  if( ( ! mdpIsNoIO( rl->perm ) ) ||  
      ( rc = md2ForceSizePerm( mdl , rl->id , rl-> perm  , rl->size )  ) )
        { rc = MDEnotAllowed ; goto  theEnd ; }   
   
theEnd:

  if( rc < 0  ){             
    rb -> status = SRB_ERROR ;
    rb -> answer = rc ;
  }else{
     rb -> status = SRB_OK ;
     rb -> size   = sizeof( reqSetSize ) ;
  }
}
#endif
void dbs_setperm( MDL *mdl , reqBuffer *rb , long size )
{
  int rc ;
  reqSetPerm *rl = ( reqSetPerm *)&(rb->data[0]) ;
       
  rc = md2ModPermission( mdl , rl->id , rl->name , rl->newPerm) ;
   
theEnd:

  if( rc < 0  ){             
    rb -> status = SRB_ERROR ;
    rb -> answer = rc ;
  }else{
     rb -> status = SRB_OK ;
     rb -> size   = sizeof( reqSetPerm ) ;
  }
}
void dbs_getattr( MDL *mdl , reqBuffer *rb , long size )
{
  int rc ;
  reqGetAttr *ga = ( reqGetAttr *)&(rb->data[0]) ;

  if( rc = md2GetExtAttributePerm(mdl,ga->id,ga->perm,&(ga->attr),&(ga->rec))){             
        rb -> status = SRB_ERROR ;
        rb -> answer = rc ;
  }else{
        rb -> status = SRB_OK ;
        rb -> size   = sizeof( reqGetAttr ) ;
  }
  return ;
}
void dbs_getroot( MDL *mdl , reqBuffer *rb , long size )
{
  int rc ;
  reqGetRoot *ga = ( reqGetRoot *)&(rb->data[0]) ;

  if(  (  ( ! mdIsNullID( ga ->  config )  ) && 
          ( rc = md2SetRootConfig( mdl , &(ga ->  config ) )  ) ) ||
       ( rc = md2GetRootDir( mdl , &(ga ->  id) )         ) ||
       ( rc = md2GetRootConfig( mdl , &(ga ->  config ) ) )     ){             
        rb -> status = SRB_ERROR ;
        rb -> answer = rc ;
  }else{
        rb -> status = SRB_OK ;
        rb -> size   = sizeof( reqGetRoot ) ;
  }
  return ;
}
/* ====================================================================== */
   void  installLevelMask(  MDL *mdl , char * levelMask )
/* ====================================================================== */
{
   int state = 0 ;
   int i ;
   char c ;
   char outArray[32] ;
   int levelCounter = 0 ;
   int outPosition  = 0 ;
   
   for( i = 0 ;  ; i++ ){
      c = levelMask[i] ;
      switch( state ){
        case 0 :
           if( ( c == '-' ) || ( ( c >= '0' ) && ( c <= '9' ) ) ){
              if( outPosition < ( sizeof(outArray) -2 ) )
                  outArray[outPosition++] = c ;
              state = 1 ;
           }else if( ( c == ':' ) || ( c == '\0' ) ) {
              outPosition = 0 ;
              (void)md2SetLevelMask( levelCounter , -1 ) ;
              levelCounter ++ ;
           }
        break ;
        case 1 :
           if( ( c >= '0' ) && ( c <= '9' )  ){
              if( outPosition < ( sizeof(outArray) -2 ) )
                  outArray[outPosition++] = c ;
           }else if( ( c == ':' ) || ( c == '\0' ) ){
              int levelValue ;
              outArray[outPosition] = '\0' ;
              sscanf( outArray , "%d" , &levelValue ) ;
              outPosition = 0 ;
              (void)md2SetLevelMask( levelCounter , levelValue ) ;
              levelCounter ++ ;
              state = 0 ;
           }
        break ;
      
      
      } 
      if( c == '\0' )break ;  
   }
}
/* ====================================================================== */
   char *  db_stringHeader(  md_auth *a )
/* ====================================================================== */
{
 static char string[1024] ;
 struct tm *tms ;
 int rc ;
 time_t     clock ;
 
 time(&clock) ;
 tms = localtime( &clock ) ;
 rc  = strftime( string , 128 , " %D %T " , tms ) ;
 if( rc <= 0 ){ string[0] = '\0' ; return string ; }
 
 if( a ){
    sprintf( string + rc , "%d.%d.%d.%d-%d-%d %8s" , 
              a->host >> 24 , (a->host>>16) & 0xFF ,
              (a->host>>8) & 0xFF ,a->host & 0xFF ,
              a->uid,a->gid , dbName            ) ;
 }else{
    sprintf( string + rc , "0.0.0.0-0-0  %8s" , dbName );
 }
           
 return string ;  
}

