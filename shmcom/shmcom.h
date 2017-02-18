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
#ifndef SHMCOM_H__
#define SHMCOM_H__

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>

#if defined(solaris) && ! defined(darwin)
   union semun {
               int val;
               struct semid_ds *buf;
               ushort *array;
   } ;
#endif
#ifdef linux 
       #if defined(__GNU_LIBRARY__) && !defined(_SEM_SEMUN_UNDEFINED)
       /* union semun is defined by including <sys/sem.h> */
       #else
       /* according to X/OPEN we have to define it ourselves */
       union semun {
               int val;                    /* value for SETVAL */
               struct semid_ds *buf;       /* buffer for IPC_STAT, IPC_SET */
               unsigned short int *array;  /* array for GETALL, SETALL */
               struct seminfo *__buf;      /* buffer for IPC_INFO */
       };
       #endif
#endif


typedef struct genShmHead_ {

  long size ;
  long type ;
  int  shmID ;
  
} genShmHead ;

typedef struct distShmHead_ {

  genShmHead gen ;
  int  semID ;
  int  clients , servers ;


} distShmHead ;

typedef struct scl_client_slot_ {

   pid_t pid ;
   int   shmid ;
   int   semid ;
   long  flags ;
   
}scl_client_slot ;


typedef struct scl_server_slot_ {

   pid_t pid ;
   int   semid ;
   int   requestSlot ;
   long  flags ;
   
}scl_server_slot ;

typedef struct scl_local_client_map_ {
     genShmHead *head ;
     int shmid ;
     int semid ;
} scl_local_client_map ;

typedef struct scl_io_desc {
   int requestSlot ;
   char *buffer ;
   int  size ;
   
} SCLIO ;

typedef struct scl_local_struc {

   int type , semid , id , server_semid , client_semid , client_shmid;
   char *base ;
   distShmHead *head ;
   scl_local_client_map *clientMap  ;
   int clientMaps ;
   scl_server_slot *server ;
   scl_client_slot *client ;
   int clients , servers ;
   
   /*
    * as long as we will only handle one io block we may put it here.
    */
   SCLIO  sclio ;
} SCL ;


#define SCL_DIST_TYPE     (0x14151617)
#define SCL_IO_TYPE       (0x27364554)


#define SCL_CLIENT        (0)
#define SCL_SERVER        (1)

#define SCL_FLG_INUSE      (1)

#define scl_total_size(c,s)  ((c)*sizeof(scl_client_slot)+\
                              (s)*sizeof(scl_server_slot)+sizeof(distShmHead))
 
#define scl_detach(x)    shmdt((char*)(x))
                             
#define SCL_MAX_CLIENTS   (64)
#define SCL_MAX_SERVERS   (64)


#define SCL_ERRORBASE   (-330)
#define SCL_BUSERROR    (-333)
#define SCL_NOT_PROT    (-334)
#define SCL_TIMEOUT     (-335)
#define SCL_NOTFOUND    (-336)
#define SCL_SHMMALLOC   (-337)
#define SCL_EXISTS      (-338)
#define SCL_MALLOC      (-339)
#define SCL_NOSERVER    (-340)
#define SCL_LAST        (-340)

#ifndef SCL_CRITICAL_TIMEOUT
#define SCL_CRITICAL_TIMEOUT (60)
#endif
/*
#define dont_use_signals
*/
#ifdef dont_use_signals
#define   sclStartShmAccess() 
#define    sclEndShmAccess(buserr) buserr=0
#define   sclInitShmAccess()  
#else
#define   sclStartShmAccess() \
   sigaction(SIGSEGV,&scl_jump_stuff.saNew,&scl_jump_stuff.saOld);\
   sigaction(SIGBUS,&scl_jump_stuff.saNew,&scl_jump_stuff.saOld);\
   if(!setjmp(scl_jump_stuff.env)){scl_jump_stuff.jump_set=1;
   
#define    sclEndShmAccess(buserr) \
   buserr=0;}else{buserr=1;scl_jump_stuff.jump_set=0;}\
   sigaction(SIGSEGV,&scl_jump_stuff.saOld,NULL);\
   sigaction(SIGBUS,&scl_jump_stuff.saOld,NULL);

#define   sclInitShmAccess()  if(scl_jump_stuff_init==0){\
   scl_jump_stuff.saNew.sa_handler=scl_intr_handler;\
   sigemptyset(&scl_jump_stuff.saNew.sa_mask);\
   sigaddset(&scl_jump_stuff.saNew.sa_mask,SIGBUS);\
   sigaddset(&scl_jump_stuff.saNew.sa_mask,SIGSEGV);\
   scl_jump_stuff.saNew.sa_flags=0;scl_jump_stuff_init=1;}
#endif

#define scl_get_gen_head(p)     ((genShmHead*)(p))
#define scl_get_gen_data(p)     (((char*)(p))+sizeof(genShmHead))
#define scl_get_head(p)     ((distShmHead*)(p))
#define scl_get_client(p)   ((scl_client_slot*)(((char*)(p))+sizeof(distShmHead)))
#define scl_get_server(p)   ((scl_server_slot*)(((char*)(p))+sizeof(distShmHead)+\
                               (scl_get_head(p)->clients)*\
                               sizeof(scl_client_slot)))

#define sclioData(s)     ((s)->buffer+sizeof(genShmHead))
#define sclioBuffer(s)   ((s)->buffer+sizeof(genShmHead))
#define sclioSize(s)     (((genShmHead*)((s)->buffer))->size)

int scl_create( key_t key , long size ) ;
int scl_check( key_t key ) ;
int scl_discard( key_t key ) ;
int scl_get_critical( char *ptr , int wt );
int scl_rel_critical( char *ptr  );
int scl_get_critical_low( int semid , int wt );
int scl_rel_critical_low( int semid   );
char * scl_attach( key_t key ) ;
char * scl_sattach( int shmid ) ;
void scl_dump_memory( FILE *out , unsigned char *p , long address , long rest );
int scl_print(  key_t key ) ;
int scl_sprint(  int shmid ) ;
int scl_rprint(  char *base ) ;
int scl_xprint( key_t key );
int scl_rxprint(  char *base );
int scl_add_client( char *base , int size ) ;
int scl_rm_client( char *base , int slot );
int scl_add_server( char *base , int slot );
int scl_rm_server( char *base , int slot );
int scl_wait_for( int semid , int semnum , int wt );
int scl_release_from( int semid , int semnum  );
int scl_client_get( char *base , int slot , int clientSlot , int wt );
int scl_server_get( char *base , int slot , int wt );
int scl_done( char *base , int slot , int type );
#define  scl_server_done(b,s)     scl_done(b,s,SCL_SERVER)
#define  scl_client_done(b,s)     scl_done(b,s,SCL_CLIENT)
int scl_answer_ready( char *base , int slot );
int scl_answer_wait( char *base , int slot , int wt );
SCLIO *sclServerWait( SCL *scl , int wt , int *rc );
int sclServerReady( SCL *scl , SCLIO *sclio );
int sclClose( SCL *scl );
SCL *sclClientOpen( key_t key , int size , int *rc );
SCLIO *sclClientGetBuffer( SCL *scl , int *rc  );
SCLIO *sclClientPostAndWait( SCL *scl , SCLIO *sclio , int slot , int wt ,int *rc ) ;
char * sclError( char *txt , int rc );


SCL *sclServerOpen( key_t key , int serverid , int *rc );
int sclServerClose( SCL *scl );

#define IO_SEM_FLAG (0) 

#endif

