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
/*
 *     semctl has semarg as last argument and not NULL
 */
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <setjmp.h>
#include "md2log.h"
#include "shmcom.h"
#include "dbgprint.h"

void scl_intr_handler( int sig );

struct {
   int jump_set ;
   jmp_buf  env ;
   struct sigaction saNew , saOld ;

} scl_jump_stuff ;
int scl_jump_stuff_init = 0 ;

char * scl_attach( key_t key )
{
   int shmid  ;
   distShmHead *head ;

   if( (int)key == 0 )return NULL ;
   shmid = shmget( key , 0 , 0  ) ;
   if( shmid < 0 )return NULL ;

   head = (distShmHead *) shmat( shmid , NULL , 0 ) ;
   if( (int)head == -1 )return NULL ;

   return (char *)head ;

}
char * scl_sattach( int shmid )
{
   distShmHead *head ;

   head = (distShmHead *) shmat( shmid , NULL , 0 ) ;
   if( (int)head == -1 )return NULL ;

   return (char *)head ;

}
int scl_get_critical( char *ptr , int wt )
{
   int shmid , rc , buserr , semid , size , rerrno ;
   distShmHead *head ;
   struct sigaction saNew , saOld ;
   struct sembuf sops[4];

   head = (distShmHead *)ptr ;
   sclInitShmAccess() ;

   sclStartShmAccess() ;

      shmid  = head -> gen.shmID ;
      semid  = head -> semID ;
      size   = head -> gen.size ;

   sclEndShmAccess(buserr) ;
   if(buserr)return SCL_BUSERROR ;

   return scl_get_critical_low( semid , wt ) ;
}
int scl_get_critical_low( int semid , int wt )
{
   int rc , rerrno ;
   struct sigaction saNew , saOld ;
   struct sembuf sops[4];

   if( wt > 0 ){
      saNew.sa_handler=scl_intr_handler;
      sigemptyset(&saNew.sa_mask);
      saNew.sa_flags=0;
      sigaction(SIGALRM,&saNew,&saOld);


      /*
       * allow SIGALRM to hadle timeouts
       */

      sigset_t allowSet ;

      sigemptyset( &allowSet ) ;
      sigaddset( &allowSet , SIGALRM ) ;

      sigprocmask( SIG_UNBLOCK , &allowSet , NULL ) ;

      alarm( wt ) ;

      sops[0].sem_num  = 0 ;
      sops[0].sem_op   = -1 ;
      sops[0].sem_flg  = SEM_UNDO ;

      rc = semop(semid, sops, 1 ) ;
      rerrno = errno ;
      alarm( 0 ) ;

      /*
       * block SIGALRM
       */
      sigprocmask( SIG_BLOCK , &allowSet , NULL ) ;
      sigaction(SIGALRM,&saOld,NULL);
      if( rc < 0 ){
         if( rerrno == EINTR )return SCL_TIMEOUT ;
         else return -1 ;
      }

   }else{

      sops[0].sem_num  = 0 ;
      sops[0].sem_op   = -1 ;
      sops[0].sem_flg  = IPC_NOWAIT | SEM_UNDO ;

      if( semop(semid, sops, 1 ) < 0 ){
         if( errno == EAGAIN )return SCL_TIMEOUT ;
         else return -1 ;
      }

   }
   return 0 ;
}
int scl_rel_critical( char *ptr  )
{
   int shmid , rc , buserr , semid , size ;
   distShmHead *head ;
   struct sigaction saNew , saOld ;

   head = (distShmHead *)ptr ;
   sclInitShmAccess() ;

   sclStartShmAccess() ;

      shmid  = head -> gen.shmID ;
      semid  = head -> semID ;
      size   = head -> gen.size ;

   sclEndShmAccess(buserr) ;

   if(buserr)return SCL_BUSERROR ;

   return scl_rel_critical_low( semid ) ;
}
int scl_rel_critical_low( int semid  )
{
   struct sembuf sops[4];
   int rc ;


      sops[0].sem_num  = 0 ;
      sops[0].sem_op   = 1 ;
      sops[0].sem_flg  = SEM_UNDO ;

      rc = semop(semid, sops, 1 ) ;
      if( rc < 0 )return -1 ;

   return 0 ;
}
int scl_discard( key_t key )
{
   int shmid , rc , buserr , semid , shmid2 , size ;
   distShmHead *head ;
   union semun  semarg ;

   if( (int)key == 0 )return -1 ;
   shmid = shmget( key , 0 , 0  ) ;
   if( shmid < 0 )return -errno ;

   head = (distShmHead *) shmat( shmid , NULL , 0 ) ;
   if( (int)head == -1 )return -errno ;

   sclInitShmAccess() ;

   sclStartShmAccess() ;

      shmid2 = head -> gen.shmID ;
      semid  = head -> semID ;
      size   = head -> gen.size ;

   sclEndShmAccess(buserr) ;

   if(buserr){ rc = SCL_BUSERROR ; goto problem ; } ;

   if( shmid2 != shmid ){ rc = SCL_NOT_PROT ; goto problem ; } ;

   if( semid >= 0 )(void)semctl( semid , 0 , IPC_RMID , semarg ) ;
   (void)shmctl( shmid , IPC_RMID , NULL ) ;
   (void)shmdt( (char *)head ) ;
   return 0 ;

problem :

   if( head )(void)shmdt( (char *)head ) ;

   return rc ;
}
int scl_release_from( int semid , int semnum  )
{
   struct sembuf sops[4];
   int rc ;

      sops[0].sem_num  = semnum ;
      sops[0].sem_op   = 1 ;
      sops[0].sem_flg  = IO_SEM_FLAG ;

      dbgprintln( " scl_release_from : semop( %d , (%d,%d,%d) , 1 )",
                  semid,sops[0].sem_num,sops[0].sem_op,sops[0].sem_flg ) ;

      rc =  semop(semid, sops, 1 ) ;
      dbgprintln( " scl_release_from : semop -> %d (errno=%d)" , rc , errno ) ;
      if( rc < 0 ){
         md2pPrintf(md2pMaxLevel,
                    "scl_release_from : semnum %d ; semid %d ;\n",
                    semnum , semid  ) ;
         md2pPrintf(md2pMaxLevel,"scl_release_from : trying again \n" ) ;
      }
      return rc ;

}
int scl_wait_for( int semid , int semnum , int wt )
{
   struct sigaction saNew , saOld ;
   struct sembuf sops[4];
   int rc , rerrno ;

   dbgprintln( " scl_wait_for( %d , %d , %d ) " , semid , semnum , wt ) ;

   if( wt > 0 ){
      saNew.sa_handler=scl_intr_handler;
      sigemptyset(&saNew.sa_mask);
      saNew.sa_flags=0;
      sigaction(SIGALRM,&saNew,&saOld);

      /*
       * allow SIGALRM to hadle timeouts
       */

      sigset_t allowSet ;

      sigemptyset( &allowSet ) ;
      sigaddset( &allowSet , SIGALRM ) ;

      sigprocmask( SIG_UNBLOCK , &allowSet , NULL ) ;
      alarm( wt ) ;

      sops[0].sem_num  = semnum ;
      sops[0].sem_op   = -1 ;
      sops[0].sem_flg  = IO_SEM_FLAG ;

      dbgprintln( " scl_wait_for : semop( %d , (%d,%d,%d) , 1 )",
                  semid,sops[0].sem_num,sops[0].sem_op,sops[0].sem_flg ) ;
      rc = semop(semid, sops, 1 ) ;
      rerrno = errno ;
      dbgprintln( " scl_wait_for : semop -> %d (errno=%d)" , rc , rerrno ) ;
      alarm( 0 ) ;

      /*
       * block SIGALRM
       */
      sigprocmask( SIG_BLOCK , &allowSet , NULL ) ;
      sigaction(SIGALRM,&saOld,NULL);
      if( rc < 0 ){
         if( rerrno == EINTR )return SCL_TIMEOUT ;
         else return -1 ;
      }

   }else{

      sops[0].sem_num  = semnum ;
      sops[0].sem_op   = -1 ;
      sops[0].sem_flg  = IPC_NOWAIT | IO_SEM_FLAG ;

      dbgprintln( " scl_wait_for : semop( %d , (%d,%d,%d) , 1 )",
                  semid,sops[0].sem_num,sops[0].sem_op,sops[0].sem_flg ) ;
      rc = semop(semid, sops, 1 )  ;
      dbgprintln( " scl_wait_for : semop -> %d (errno=%d)" , rc , errno ) ;
      if( rc < 0 ){
         if( errno == EAGAIN )return SCL_TIMEOUT ;
         else return -1 ;
      }

   }
   return 0 ;
}
int scl_client_get( char *base , int slot , int clSlot , int wt )
{
  int grc , buserr , rc , critical , semid ;
  distShmHead *head ;
  scl_client_slot *client ;
  scl_server_slot *server ;
   union semun  semarg ;

   sclInitShmAccess() ;
   grc = 0 ;
   sclStartShmAccess() ;
     head   = scl_get_head( base ) ;
     server = scl_get_server( base ) ;
     semid  = server[slot].semid ;
     if( ( slot >= head->servers ) ||
         ( ! server[slot].flags ) )grc = SCL_NOTFOUND ;
     if( ( clSlot >= head->clients ) ||
         ( ! client[clSlot].flags ) )grc = SCL_NOTFOUND ;
   sclEndShmAccess(buserr) ;
   if(buserr)return SCL_BUSERROR ;

   if( grc )return grc ;

   rc = scl_wait_for( semid , SCL_CLIENT , wt ) ;
   if( rc == 0 ){

      sclStartShmAccess() ;
         server[slot].requestSlot  = clSlot ;
      sclEndShmAccess(buserr) ;
      if(buserr)return SCL_BUSERROR ;
      return 0 ;
   }else if( rc != SCL_TIMEOUT ){
      return rc ;
   }else{
     rc = kill( server[slot].pid , 0 ) ;
     if( rc && ( errno == ESRCH ) ){
       /*
        * this is an attempt to clean situation if one of the
          servers appears dead to us.
        */
       sclStartShmAccess() ;
          if( ! ( critical = scl_get_critical( base , SCL_CRITICAL_TIMEOUT ) ) ){
            server[slot].flags = 0 ;
            (void)semctl( server[slot].semid , 0 , IPC_RMID , semarg ) ;
          }
          scl_rel_critical( base  )  ;
       sclEndShmAccess(buserr) ;
       if(buserr)return SCL_BUSERROR ;
       if(critical)return SCL_TIMEOUT ;
     }else if( rc ){
        return rc ;
     }

     return SCL_TIMEOUT ;
  }
}
int scl_server_get( char *base , int slot , int wt )
{
  int grc , buserr , rc , critical , semid ;
  distShmHead *head ;
  scl_client_slot *client ;
  scl_server_slot *server ;

   sclInitShmAccess() ;
   grc = 0 ;
   sclStartShmAccess() ;
     head   = scl_get_head( base ) ;
     server = scl_get_server( base ) ;
     semid  = server[slot].semid ;
     if( ( slot >= head->servers ) ||
         ( ! server[slot].flags ) )grc = SCL_NOTFOUND ;
   sclEndShmAccess(buserr) ;
   if(buserr)return SCL_BUSERROR ;

   if( grc )return grc ;

   return scl_wait_for( semid , SCL_SERVER , wt ) ;

}
int scl_done( char *base , int slot , int type )
{
  int grc , buserr , rc , critical , semid , cSlot ;
  distShmHead *head ;
  scl_client_slot *client ;
  scl_server_slot *server ;

   sclInitShmAccess() ;
   grc = 0 ;
   sclStartShmAccess() ;
     head   = scl_get_head( base ) ;
     server = scl_get_server( base ) ;
     semid  = server[slot].semid ;
     cSlot  = server[slot].requestSlot ;
     if( ( slot >= head->servers ) ||
         ( ! server[slot].flags ) )grc = SCL_NOTFOUND ;
   sclEndShmAccess(buserr) ;
   if(buserr)return SCL_BUSERROR ;

   if( grc )return grc ;

   rc =  scl_release_from( semid , type ) ;
   if(rc)return rc ;
   return cSlot < 0 ? SCL_NOTFOUND : cSlot ;
}
int scl_answer_ready( char *base , int slot )
{
  int grc , buserr , rc , critical , semid , rslot ;
  distShmHead *head ;
  scl_client_slot *client ;
  scl_server_slot *server ;

   sclInitShmAccess() ;
   grc = 0 ;
   sclStartShmAccess() ;
     head   = scl_get_head( base ) ;
     server = scl_get_server( base ) ;
     client = scl_get_client( base ) ;
     if( ( slot >= head->servers ) ||
         ( ! server[slot].flags ) ){
          grc = SCL_NOTFOUND ;
     }else if( ( rslot  = server[slot].requestSlot ) >= head->clients ){
          grc = SCL_NOTFOUND ;
     }else{
        semid = client[rslot].semid ;
     }
   sclEndShmAccess(buserr) ;
   if(buserr)return SCL_BUSERROR ;
   if( grc )return grc ;

   return scl_release_from( semid , 0 ) ;

}
int scl_answer_wait( char *base , int slot , int wt )
{
  int grc , buserr  ,semid  ;
  distShmHead *head ;
  scl_client_slot *client ;
  scl_server_slot *server ;

   sclInitShmAccess() ;
   grc = 0 ;
   sclStartShmAccess() ;
     head   = scl_get_head( base ) ;
     server = scl_get_server( base ) ;
     client = scl_get_client( base ) ;
     if( ( slot >= head->servers ) ||
         ( ! client[slot].flags ) ){
          grc = SCL_NOTFOUND ;
     }else{
        semid = client[slot].semid ;
     }
   sclEndShmAccess(buserr) ;
   if(buserr)return SCL_BUSERROR ;
   if( grc )return grc ;

   return scl_wait_for( semid , 0 , wt ) ;

}
int scl_add_server( char *base , int slot )
{
  int grc , buserr , s , critical ;
  distShmHead *head ;
  scl_server_slot *server ;
  union semun  semarg ;

   sclInitShmAccess() ;
   grc = 0 ;
   sclStartShmAccess() ;
     head   = scl_get_head( base ) ;
     server = scl_get_server( base ) ;

     s = head -> servers ;
   sclEndShmAccess(buserr) ;
   if(buserr)return SCL_BUSERROR ;

   if( slot >= s )return SCL_NOTFOUND ;

   sclStartShmAccess() ;
     critical = scl_get_critical( base , SCL_CRITICAL_TIMEOUT /* seconds */ ) ;
     if( ! critical ){
        if( server[slot].flags  && ! kill( server[slot].pid , 0 ) ){

            grc = SCL_EXISTS ;

        }else{
           if( server[slot].flags && ( server[slot].semid > -1 ) ){
           }else{
              server[slot].semid = semget( IPC_PRIVATE , 2 , IPC_CREAT | 0700 ) ;
           }
           if( server[slot].semid >= 0 ){
              server[slot].flags = SCL_FLG_INUSE ;
              semarg.val = 1 ;
              semctl( server[slot].semid , SCL_CLIENT , SETVAL , semarg ) ;
              semarg.val = 0 ;
              semctl( server[slot].semid , SCL_SERVER , SETVAL , semarg ) ;
           }else{
              grc = SCL_SHMMALLOC ;
           }
           server[slot].pid = getpid() ;
        }
        scl_rel_critical( base );
     }
   sclEndShmAccess(buserr) ;
   if(buserr)return SCL_BUSERROR ;
   if(critical)return critical ;

   return grc ;

}
int scl_rm_server( char *base , int slot )
{
  int grc , buserr , s , critical ;
  distShmHead *head ;
  scl_server_slot *server ;
   union semun  semarg ;

   sclInitShmAccess() ;
   grc = 0 ;
   sclStartShmAccess() ;
     head   = scl_get_head( base ) ;
     server = scl_get_server( base ) ;

     s = head -> servers ;
   sclEndShmAccess(buserr) ;
   if(buserr)return SCL_BUSERROR ;

   if( slot >= s )return SCL_NOTFOUND ;

   sclStartShmAccess() ;
     critical = scl_get_critical( base , SCL_CRITICAL_TIMEOUT /* seconds */ ) ;
     if( ! critical ){
        if( server[slot].flags ){
           (void)semctl( server[slot].semid , 0 , IPC_RMID , semarg ) ;
           server[slot].flags = 0 ;
        }else{
           grc = SCL_NOTFOUND ;
        }
        scl_rel_critical( base );
     }
   sclEndShmAccess(buserr) ;
   if(buserr)return SCL_BUSERROR ;
   if(critical)return critical ;

   return grc ;

}
int scl_rm_client( char *base , int slot )
{
  int grc , buserr , c , critical ;
  distShmHead *head ;
  scl_client_slot *client ;
  scl_server_slot *server ;
   union semun  semarg ;

   sclInitShmAccess() ;
   grc = 0 ;
   sclStartShmAccess() ;
     head   = scl_get_head( base ) ;
     client = scl_get_client( base ) ;
     server = scl_get_server( base ) ;

     c = head -> clients ;
   sclEndShmAccess(buserr) ;
   if(buserr)return SCL_BUSERROR ;
   if( slot >= c )return SCL_NOTFOUND ;

   sclStartShmAccess() ;
     critical = scl_get_critical( base , SCL_CRITICAL_TIMEOUT /* seconds */ ) ;
     if( ! critical ){
        if( client[slot].flags ){
           (void)shmctl( client[slot].shmid , IPC_RMID , NULL ) ;
           (void)semctl( client[slot].semid , 0 , IPC_RMID , semarg ) ;
           client[slot].flags = 0 ;
        }else{
           grc = SCL_NOTFOUND ;
        }
        scl_rel_critical( base );
     }
   sclEndShmAccess(buserr) ;
   if(buserr)return SCL_BUSERROR ;
   if(critical)return critical ;

   return grc ;

}
#ifdef auto_shm_recover
int scl_add_client( char *base , int size )
{
  int rc , buserr , slot , c , critical ;
  char *io ;
  distShmHead *head ;
  genShmHead *genHead ;
  scl_client_slot *client ;
  scl_server_slot *server ;

   sclInitShmAccess() ;

   sclStartShmAccess() ;
     head   = scl_get_head( base ) ;
     client = scl_get_client( base ) ;
     server = scl_get_server( base ) ;

     c = head -> clients ;

     critical = scl_get_critical( base , SCL_CRITICAL_TIMEOUT /* seconds */ ) ;
     if( ! critical ){
        /*
         * now try to find an empty slot
         */
        for( slot = 0 ; ( slot < c ) && client[slot].flags; slot ++ );
        if( slot < c ){
          /* found a slot */
          io = 0 ; /* triggers creation of new shm */
        }else{
          /* no slot available, ( really ? ) */
          for( slot = 0 ; slot < c ; slot ++ ){
              rc = kill( client[slot].pid , 0 ) ;
              if( ( rc < 0 ) && ( errno == ESRCH ) ){
                 /* this process is dead, let's reuse the shm */
                 io = (char*)shmat( client[slot].shmid , NULL , 0 ) ;
                 if( (int) io == -1 )io = NULL ;
                 if( io != NULL ){
                    /* check if it's valid */
                    genHead = (genShmHead*)io ;
                    if( ( genHead -> type  != SCL_IO_TYPE     ) ||
                        ( genHead -> shmID != client[slot].shmid ) ){
                       /*
                        * wrong header
                        */
                       shmdt( io ) ;
                       io = NULL ; /* seems to be something else */
                       client[slot].shmid = -1 ;
                    }else if( genHead -> size < size ){
                       /*
                        * wrong size
                        */
                       shmdt( io ) ;
                       io = NULL ;
                       (void)shmctl( client[slot].shmid , IPC_RMID , NULL ) ;
                       client[slot].shmid = -1 ;

                    }/* else{
                        }
                    */
                    client[slot].flags = 0 ;
                 }
                 break ;
              }
          } /* end of the client slot loop */
        }
        if( slot < c ){

            if( io == NULL ){
               rc = shmget( IPC_PRIVATE , size , 0700 ) ;
               if( rc >= 0 ){
                  client[slot].shmid = rc ;
                  io = (char *)shmat( client[slot].shmid , 0L , 0 ) ;
                  if( (int)io == -1 )io = NULL ;
               }
            }
            if( io != NULL ){
               genHead = (genShmHead*)io ;
               genHead -> type    = SCL_IO_TYPE ;
               genHead -> shmID   = client[slot].shmid ;
               genHead -> size    = size ;
               client[slot].pid   = getpid() ;
               client[slot].flags = SCL_FLG_INUSE ;
               shmdt( io )  ;
            }

        }
        scl_rel_critical( base );
      }
   sclEndShmAccess(buserr) ;
   if(buserr)return SCL_BUSERROR ;
   if( critical )return critical ;
   if( slot >= c )return SCL_NOTFOUND ;
   if( ! io )return SCL_SHMMALLOC ;
   return slot ;

}
#else
int scl_add_client( char *base , int size )
{
  int rc , buserr , slot , c , critical ;
  char *io ;
  distShmHead *head ;
  genShmHead *genHead ;
  scl_client_slot *client ;
  scl_server_slot *server ;

   sclInitShmAccess() ;

   sclStartShmAccess() ;
     head   = scl_get_head( base ) ;
     client = scl_get_client( base ) ;
     server = scl_get_server( base ) ;

     c  = head -> clients ;
     io = 0 ;
     critical = scl_get_critical( base , SCL_CRITICAL_TIMEOUT /* seconds */ ) ;
     if( ! critical ){
        /*
         * now try to find an empty slot
         */
        for( slot = 0 ; ( slot < c ) && client[slot].flags; slot ++ );
        if( slot < c ){

           rc = shmget( IPC_PRIVATE , size , 0700 ) ;
           if( rc >= 0 ){
              client[slot].shmid = rc ;
              io = (char *)shmat( client[slot].shmid , 0L , 0 ) ;
              if( (int) io == -1 )io = NULL ;
              if( io != NULL  ){
                 /*
                  * shm is ok. initialize it.
                  */
                 genHead = (genShmHead*)io ;
                 genHead -> type    = SCL_IO_TYPE ;
                 genHead -> shmID   = client[slot].shmid ;
                 genHead -> size    = size ;
                 /*
                  * and now the semaphore
                  */
                 rc = semget( IPC_PRIVATE , 1 , 0700 ) ;
                 if( rc >= 0 ){
                    client[slot].semid = rc ;
                    client[slot].pid   = getpid() ;
                    client[slot].flags = SCL_FLG_INUSE ;
                    shmdt( io )  ;
                 }else{ /* couldn't get semid */
                    shmdt( io )  ;
                   (void)shmctl( client[slot].shmid , IPC_RMID , NULL ) ;
                 }
              }else{ /* can't attach shm */
                (void)shmctl( client[slot].shmid , IPC_RMID , NULL ) ;
              }
           }else{ /* couldn't get shm */
              io = NULL ;
           }
        } /* else { no empty slot } */
        scl_rel_critical( base );
      }
   sclEndShmAccess(buserr) ;
   if(buserr)return SCL_BUSERROR ;
   if( critical )return critical ;
   if( slot >= c )return SCL_NOTFOUND ;
   if( ! io )return SCL_SHMMALLOC ;
   return slot ;

}
#endif
int scl_install( key_t key , int c , int s )
{
  int rc , buserr , i ;
  char * base ;
  distShmHead *head ;
  scl_client_slot *client ;
  scl_server_slot *server ;

  rc = scl_create( key , scl_total_size( c , s ) ) ;
  if( rc < 0 )return rc ;

  if( ( base = scl_attach( key ) ) == NULL ){
     scl_discard( key ) ;
     return -1 ;
  }

   sclInitShmAccess() ;

   sclStartShmAccess() ;

     head   = scl_get_head( base ) ;

     head -> clients = c ;
     head -> servers = s ;

     client = scl_get_client( base ) ;
     server = scl_get_server( base ) ;


     for( i = 0 ; i < head -> clients ; i ++ ){
        client[i].pid   = 0 ;
        client[i].shmid = -1 ;
        client[i].semid = -1 ;
        client[i].flags = 0 ;
     }
     for( i = 0 ; i < head -> servers ; i ++ ){
        server[i].pid   = 0 ;
        server[i].semid = -1 ;
        server[i].flags = 0 ;
        server[i].requestSlot = -1 ;
     }

   sclEndShmAccess(buserr) ;

   if(buserr){
       scl_discard( key ) ;
       return SCL_BUSERROR ;
   }
   scl_detach( base ) ;
   return 0 ;
}
int scl_print( key_t key )
{
  char * base ;

  if( ( base = scl_attach( key ) ) == NULL ){
     return -1 ;
  }
  return  scl_rprint( base ) ;
}
int scl_xprint( key_t key )
{
  char * base ;

  if( ( base = scl_attach( key ) ) == NULL ){
     return -1 ;
  }
  return  scl_rxprint( base ) ;
}
int scl_sprint( int shmid )
{
  char * base ;

  if( ( base = scl_sattach( shmid ) ) == NULL ){
     return -1 ;
  }
  return  scl_rprint( base ) ;
}
int scl_rprint(  char *base )
{
  int rc , buserr , i , s , c , ext , sr , cl ;
  long type ;
  distShmHead *head ;
  genShmHead *io ;
  unsigned long *data ;
  scl_client_slot *client ;
  scl_server_slot *server ;
  union semun  semarg ;

  ext = 1 ;

   head   = scl_get_head( base );
   rc = 0 ;
   sclInitShmAccess() ;

   sclStartShmAccess() ;


     printf( " -------------------------------\n" ) ;
     printf( " Shm Type   : %X " , type = head -> gen.type ) ;

     if( type == SCL_DIST_TYPE ){
        printf( " (Main Distributer)\n" ) ;
        printf( " Total Size : %d bytes\n" , head -> gen.size ) ;
        printf( " Shm ID     : %d\n" , head -> gen.shmID ) ;
        printf( " Sem ID     : %d\n" , head -> semID ) ;
        printf( " Clients    : %d\n" , head -> clients ) ;
        printf( " Servers    : %d\n" , head -> servers ) ;

        c = head -> clients ;
        s = head -> servers ;
     }else if( type == SCL_IO_TYPE ){
        printf( " (I/O Data Area)\n" ) ;
        printf( " Total Size : %d bytes\n" , head -> gen.size ) ;
        printf( " Shm ID     : %d\n" , head -> gen.shmID ) ;
     }else{
        printf( " (Absolutely unknown)\n" ) ;
     }
   sclEndShmAccess(buserr) ;
   if(buserr){
       fprintf(stderr," Bus Error reading header informations\n");
       rc = SCL_BUSERROR ;
       goto problem  ;
   }
   if( type != SCL_DIST_TYPE ){ rc = 0 ; goto problem ; }


   sclStartShmAccess() ;
     client = scl_get_client( base ) ;
     server = scl_get_server( base ) ;

     printf( " -- Clients --------------------------\n" );
     printf( " Slot   Pid      Shm      Sem     Flags" );
     if(ext)printf( ext?"   V     Shm Size   Start\n" :"\n");

     for( i = 0 ; i < head -> clients ; i ++ ){
        printf( " %2.2d   " , i ) ;
        printf( " %5.5d   " , client[i].pid  ) ;
        if( client[i].shmid >= 0 )printf( " %6.6d  " , client[i].shmid  ) ;
        else printf( " ......  "  ) ;
        if( client[i].semid >= 0 )printf( " %6.6d  " , client[i].semid  ) ;
        else printf( " ......  "  ) ;
        printf( " %5.5X  " , client[i].flags ) ;
        if( ext && client[i].flags ){
            cl = semctl( client[i].semid , 0 , GETVAL , semarg ) ;
            if( cl < 0 ){
               printf( " .   " ) ;
            }else{
               printf( " %d   " , cl ) ;
            }
            io = scl_get_gen_head( shmat( client[i].shmid , NULL , 0) ) ;
            if( (int)io == -1 )io = NULL ;
            if( ! io ){
                printf(" Not Accessable\n");
            }else{ int j ;
                data = (unsigned long*)scl_get_gen_data( io ) ;
                printf(" %6d      %8.8X %8.8X %8.8X",io->size,data[0],data[1],data[2]);
                /*for(j=0;j<10;j++)printf(" %8.8X" , data[j] ) ;*/
                printf("\n");
                shmdt( (char *)io ) ;
            }

        }else
            printf("\n");
     }
     printf( " -- Servers --------------------------\n" );
     printf( " Slot   Pid      Sem     Flags   rSlot" );
     if(ext)printf( ext?"    Client   Server\n" :"\n");
     for( i = 0 ; i < head -> servers ; i ++ ){
        printf( " %2.2d   " , i ) ;
        printf( " %5.5d   " , server[i].pid  ) ;
        if( server[i].semid >= 0 )printf( " %6.6d  " , server[i].semid  ) ;
        else printf( " ......  "  ) ;
        printf( " %5.5X  " , server[i].flags ) ;
        if( server[i].requestSlot >= 0 )printf( "   %2.2d    " , server[i].requestSlot  ) ;
        else printf( "   ..    "  ) ;
        if( ext && server[i].flags ){
           cl = semctl( server[i].semid , SCL_CLIENT , GETVAL , semarg ) ;
           sr = semctl( server[i].semid , SCL_SERVER , GETVAL , semarg ) ;
           printf( "    %d         %d\n" , cl , sr ) ;
        }else
            printf("\n");
     }

   sclEndShmAccess(buserr) ;

   if(buserr){
       fprintf(stderr," Bus Error reading Data\n");
       rc = SCL_BUSERROR ;
       goto problem  ;
   }

problem :

   scl_detach( base ) ;
   return rc ;
}
int scl_rxprint(  char *base )
{
  int rc , buserr , i , s , c , ext , sr , cl ;
  long type ;
  distShmHead *head ;
  genShmHead *io ;
  unsigned long *data ;
  scl_client_slot *client ;
  scl_server_slot *server ;
  union semun  semarg ;

  ext = 1 ;

   head   = scl_get_head( base );
   rc = 0 ;
   sclInitShmAccess() ;

   sclStartShmAccess() ;


     printf( "ShmType   %X " , type = head -> gen.type ) ;

     if( type == SCL_DIST_TYPE ){
        printf( " MainDistributer\n" ) ;
        printf( "TotalSize  %d bytes\n" , head -> gen.size ) ;
        printf( "ShmID      %d\n" , head -> gen.shmID ) ;
        printf( "SemID      %d\n" , head -> semID ) ;
        printf( "Clients    %d\n" , head -> clients ) ;
        printf( "Servers    %d\n" , head -> servers ) ;

        c = head -> clients ;
        s = head -> servers ;
     }else if( type == SCL_IO_TYPE ){
        printf( " (I/O Data Area)\n" ) ;
        printf( " TotalSize %d bytes\n" , head -> gen.size ) ;
        printf( " ShmID     %d\n" , head -> gen.shmID ) ;
     }else{
        printf( " (Absolutely unknown)\n" ) ;
     }
   sclEndShmAccess(buserr) ;
   if(buserr){
       fprintf(stderr," Bus Error reading header informations\n");
       rc = SCL_BUSERROR ;
       goto problem  ;
   }
   if( type != SCL_DIST_TYPE ){ rc = 0 ; goto problem ; }


   sclStartShmAccess() ;
     client = scl_get_client( base ) ;
     server = scl_get_server( base ) ;
     /*
     printf( " -- Clients --------------------------\n" );
     */
     printf( "# Slot   Pid      Shm      Sem     Flags" );
     if(ext)printf( ext?"   V     Shm Size   Start\n" :"\n");

     for( i = 0 ; i < head -> clients ; i ++ ){
        printf( "client %d   " , i ) ;
        printf( " %d   " , client[i].pid  ) ;
        if( client[i].shmid >= 0 )printf( " %d  " , client[i].shmid  ) ;
        else printf( " .  "  ) ;
        if( client[i].semid >= 0 )printf( " %d  " , client[i].semid  ) ;
        else printf( " .  "  ) ;
        printf( " %5.5X  " , client[i].flags ) ;
        if( ext && client[i].flags ){
            cl = semctl( client[i].semid , 0 , GETVAL , semarg ) ;
            if( cl < 0 ){
               printf( " .   " ) ;
            }else{
               printf( " %d   " , cl ) ;
            }
            io = scl_get_gen_head( shmat( client[i].shmid , NULL , 0) ) ;
            if( (int)io == -1 )io = NULL ;
            if( ! io ){
                printf(" NotAccessable\n");
            }else{ int j ;
                data = (unsigned long*)scl_get_gen_data( io ) ;
                printf(" %6d      %8.8X %8.8X %8.8X",io->size,data[0],data[1],data[2]);
                /*for(j=0;j<10;j++)printf(" %8.8X" , data[j] ) ;*/
                printf("\n");
                shmdt( (char *)io ) ;
            }

        }else
            printf("\n");
     }
     /*
     printf( " -- Servers --------------------------\n" );
     */
     printf( "# Slot   Pid      Sem     Flags   rSlot" );
     if(ext)printf( ext?"    Client   Server\n" :"\n");
     for( i = 0 ; i < head -> servers ; i ++ ){
        printf( "server %d   " , i ) ;
        printf( " %d   " , server[i].pid  ) ;
        if( server[i].semid >= 0 )printf( " %d  " , server[i].semid  ) ;
        else printf( " . "  ) ;
        printf( " %5.5X  " , server[i].flags ) ;
        if( server[i].requestSlot >= 0 )printf( "   %d    " , server[i].requestSlot  ) ;
        else printf( "   .    "  ) ;
        if( ext && server[i].flags ){
           cl = semctl( server[i].semid , SCL_CLIENT , GETVAL , semarg ) ;
           sr = semctl( server[i].semid , SCL_SERVER , GETVAL , semarg ) ;
           printf( "    %d         %d\n" , cl , sr ) ;
        }else
            printf("\n");
     }

   sclEndShmAccess(buserr) ;

   if(buserr){
       fprintf(stderr," Bus Error reading Data\n");
       rc = SCL_BUSERROR ;
       goto problem  ;
   }

problem :

   scl_detach( base ) ;
   return rc ;
}
int scl_create( key_t key , long size )
{
   int shmid , rc , semid ,mode ;
   distShmHead *head , x ;
   union semun  semarg ;

   shmid = semid = -1 ;
   head = 0 ;
   mode = 0600 ;

   if( (int)key == 0 )return -1000 ;
   if( size < sizeof( distShmHead ) )return -1002 ;
   shmid = shmget( key , size , IPC_CREAT | IPC_EXCL | mode ) ;
   if( shmid < 0 )return -errno ;
/*
 * create the semaphore and set it so one
 */
   semid = semget( IPC_PRIVATE , 1 , IPC_CREAT | mode ) ;
   if( semid < 0 ){ rc = errno ; goto problem ; }
   semarg.val = 1 ;
   rc = semctl( semid , 0 , SETVAL , semarg ) ;
   if( rc < 0 ){ rc = errno ; goto problem ; }

/*
 * attach the shm area and set the appropriate entries.
 */
   head = (distShmHead *) shmat( shmid , NULL , 0 ) ;
   if( (int)head == -1 ){ rc = errno ; goto problem ; }

   head -> gen.size  = size ;
   head -> gen.shmID = shmid ;
   head -> gen.type  = SCL_DIST_TYPE ;
   head -> semID     = semid ;

   (void)shmdt( (char *)head ) ;
   return shmid ;
 problem :
   if( semid >= 0 )(void)semctl( semid , 0 , IPC_RMID , semarg ) ;
   if( shmid >= 0 )(void)shmctl( shmid , IPC_RMID , NULL ) ;
   return -rc ;
}
void scl_intr_handler( int sig )
{

  if( scl_jump_stuff.jump_set &&
     ( ( sig == SIGSEGV ) || ( sig == SIGBUS ) ) ){

     longjmp( scl_jump_stuff.env , sig ) ;
  }else if( sig == SIGALRM ){
     /*
     fprintf( stderr," Got alarm signal\n" ) ;
     */
  }else{
     fprintf( stderr," Got interrupt %d without jump set\n" , sig ) ;
  }
  return ;
}
void scl_dump_memory( FILE *out , unsigned char *p , long address , long rest )
{
   unsigned char *m ;
   int i , r ;

   for(  ; rest > 0 ;  ){
      fprintf(out," %8.8X ",address) ;
      address += 16 ;
      m = p ;
      r = rest ;
      for( i = 0 ; ( r > 0 ) && ( i < 16 )  ; r-- , i++ , m++)
         fprintf(out," %2.2x", *m ) ;
      for( ; i < 16 ;  i++ )fprintf(out,"   " ) ;
      fprintf(out," *");
      for( i = 0 ; (rest > 0) && ( i < 16 ); rest-- , i++ , p++){
         if(isprint(*p))fprintf(out,"%c", *p ) ;
         else fprintf(out,".");
      }
      for( ; i < 16 ;  i++ )fprintf(out," " ) ;
      fprintf(out,"*\n" ) ;
   }
   return ;
}
SCL *sclServerOpen( key_t key , int serverid , int *rc )
{
  SCL *scl ;
  int buserr , i ,critical , grc ;
  union semun  semarg ;

  if( rc ) *rc = 0 ;
  grc = 0 ;
  if( ( scl = (SCL*)malloc(sizeof(SCL))  )== NULL )return NULL ;

  if( ( scl -> base = scl_attach( key ) ) == NULL ){
    free( ( char *) scl ) ;
    if( rc )*rc = SCL_SHMMALLOC ;
    return NULL ;
  }
   scl -> type = SCL_SERVER ;

   sclInitShmAccess() ;

   sclStartShmAccess() ;

     scl -> head   = scl_get_head( scl -> base ) ;
     scl -> client = scl_get_client( scl ->  base ) ;
     scl -> server = scl_get_server( scl -> base ) ;
     scl -> clients = scl -> head -> clients ;
     scl -> servers = scl -> head -> servers ;
     scl -> semid   = scl -> head -> semID ;
     scl -> id      = serverid ;

   sclEndShmAccess(buserr) ;
   if(buserr){
       if( rc )*rc = SCL_BUSERROR ;
       scl_detach( scl -> base ) ;
       free( (char *) scl ) ;
       return NULL ;
   }

   scl -> clientMaps = scl -> clients ;

   scl -> clientMap = (scl_local_client_map*)
                      malloc(sizeof(scl_local_client_map)*scl->clientMaps ) ;

   if( ! scl -> clientMap ){
       if( rc )*rc = SCL_MALLOC ;
       scl_detach( scl -> base ) ;
       free( (char *) scl ) ;
       return NULL ;
   }
   for( i = 0 ; i < scl -> clientMaps ; i++ ){
      scl -> clientMap[i].head  = NULL ;
      scl -> clientMap[i].shmid = -1 ;
      scl -> clientMap[i].semid = -1 ;
   }
   scl -> sclio.buffer = NULL ;
   scl -> sclio.size   = 0 ;

   if( serverid >= scl -> servers ){ grc = SCL_NOTFOUND ; goto problem ; }

   sclStartShmAccess() ;
     critical = scl_get_critical_low( scl->semid , SCL_CRITICAL_TIMEOUT /* seconds */ ) ;
     if( ! critical ){
        if( scl->server[scl->id].flags  && ! kill( scl->server[scl->id].pid , 0 ) ){

            grc = SCL_EXISTS ;

        }else{
           if( scl->server[scl->id].flags && ( scl->server[scl->id].semid > -1 ) ){
           }else{
              scl->server[scl->id].semid = semget( IPC_PRIVATE , 2 , IPC_CREAT | 0700 ) ;
           }
           if( scl->server[scl->id].semid >= 0 ){
              scl->server[scl->id].flags = SCL_FLG_INUSE ;
              semarg.val = 1 ;
              semctl( scl->server[scl->id].semid , SCL_CLIENT , SETVAL , semarg ) ;
              semarg.val = 0 ;
              semctl( scl->server[scl->id].semid , SCL_SERVER , SETVAL , semarg ) ;
           }else{
              grc = SCL_SHMMALLOC ;
           }
           scl->server[scl->id].pid = getpid() ;
        }
        scl_rel_critical_low( scl->semid );
        scl->server_semid = scl->server[scl->id].semid ;
     }
   sclEndShmAccess(buserr) ;
   if(buserr){ grc = SCL_BUSERROR ; goto problem ; }
   if(critical){ grc = critical ; goto problem ; }

   return scl ;

problem :
       if(rc)*rc = grc ;
       scl_detach( scl -> base ) ;
       free( (char *) ( scl  -> clientMap ) ) ;
       free( (char *) scl ) ;
       return NULL ;

}
SCL *sclClientOpen( key_t key , int size , int *rc )
{
  SCL *scl ;
  int buserr , i ,critical , grc , slot ;
  union semun  semarg ;
  char *io ;
  genShmHead *genHead ;

  if( rc ) *rc = 0 ;
  grc = 0 ;
  if( ( scl = (SCL*)malloc(sizeof(SCL))  )== NULL )return NULL ;

  if( ( scl -> base = scl_attach( key ) ) == NULL ){
    free( ( char *) scl ) ;
    if( rc )*rc = SCL_SHMMALLOC ;
    return NULL ;
  }
   scl -> type = SCL_CLIENT ;

   sclInitShmAccess() ;

   sclStartShmAccess() ;

     scl -> head   = scl_get_head( scl -> base ) ;
     scl -> client = scl_get_client( scl ->  base ) ;
     scl -> server = scl_get_server( scl -> base ) ;
     scl -> clients = scl -> head -> clients ;
     scl -> servers = scl -> head -> servers ;
     scl -> semid   = scl -> head -> semID ;

   sclEndShmAccess(buserr) ;
   if(buserr){
       if( rc )*rc = SCL_BUSERROR ;
       scl_detach( scl -> base ) ;
       free( (char *) scl ) ;
       return NULL ;
   }

   scl -> clientMaps = scl -> clients ;

   scl -> clientMap = (scl_local_client_map*)
                      malloc(sizeof(scl_local_client_map)*scl->clientMaps ) ;

   if( ! scl -> clientMap ){
       if( rc )*rc = SCL_MALLOC ;
       scl_detach( scl -> base ) ;
       free( (char *) scl ) ;
       return NULL ;
   }
   for( i = 0 ; i < scl -> clientMaps ; i++ )scl -> clientMap[i].head = NULL ;
   scl -> sclio.buffer = NULL ;
   scl -> sclio.size   = 0 ;

   sclStartShmAccess() ;
     critical = scl_get_critical_low( scl->semid , SCL_CRITICAL_TIMEOUT /* seconds */ ) ;
     if( ! critical ){
        /*
         * now try to find an empty slot
         */
        for( slot = 0 ; ( slot < scl -> clients) && scl->client[slot].flags; slot ++ );
        if( slot < scl -> clients ){

           grc = shmget( IPC_PRIVATE , size , 0700 ) ;
           if( grc >= 0 ){
              scl->client[slot].shmid = grc ;
              io = (char *)shmat( scl->client[slot].shmid , 0L , 0 ) ;
              if( (int) io == -1 )io = NULL ;
              if( io != NULL ){
                 /*
                  * shm is ok. initialize it.
                  */
                 genHead = (genShmHead*)io ;
                 genHead -> type    = SCL_IO_TYPE ;
                 genHead -> shmID   = scl->client[slot].shmid ;
                 genHead -> size    = size ;
                 /*
                  * and now the semaphore
                  */
                 grc = semget( IPC_PRIVATE , 1 , 0700 ) ;
                 if( grc >= 0 ){
                    scl->client[slot].semid = grc ;
                    scl->client[slot].pid   = getpid() ;
                    scl->client[slot].flags = SCL_FLG_INUSE ;
                    shmdt( io )  ;
                 }else{ /* couldn't get semid */
                    shmdt( io )  ;
                   (void)shmctl( scl->client[slot].shmid , IPC_RMID , NULL ) ;
                   io = NULL ;
                 }
              }else{ /* can't attach shm */
                (void)shmctl( scl->client[slot].shmid , IPC_RMID , NULL ) ;
              }
           }else{ /* couldn't get shm */
              io = NULL ;
           }
        } /* else { no empty slot } */
        scl->id      = slot ;
        scl->client_semid = scl->client[slot].semid ;
        scl->client_shmid = scl->client[slot].shmid ;

        scl_rel_critical_low( scl->semid );
      }
   sclEndShmAccess(buserr) ;
   if(buserr){ grc = SCL_BUSERROR ; goto problem ; }
   if(critical){ grc = critical ; goto problem ; }
   if(!io){ grc = SCL_SHMMALLOC ; goto problem ; }

   scl->sclio.buffer = (char *)shmat( scl->client_shmid  , 0L , 0 ) ;
   if( (int)scl->sclio.buffer == -1 ){
      scl->sclio.buffer = NULL ;
      goto problem ;
   }
   /*fprintf(stderr," shmid : %d scl->sclio.buffer : %x %d\n",
   scl->client_shmid  ,scl->sclio.buffer , errno );*/
   genHead = (genShmHead*)scl->sclio.buffer ;
   scl->sclio.size = genHead -> size ;
   return scl ;

problem :
       if(rc)*rc = grc ;
       scl_detach( scl -> base ) ;
       free( (char *) ( scl  -> clientMap ) ) ;
       free( (char *) scl ) ;
       return NULL ;

}
int sclClose( SCL *scl )
{
   if( ( ! scl ) ||
       ( scl -> type != SCL_CLIENT ) && ( scl -> type != SCL_SERVER ) )
         return SCL_NOT_PROT ;

   switch( scl -> type ){

      case SCL_CLIENT : return sclClientClose( scl ) ;
      case SCL_SERVER : return sclServerClose( scl ) ;

   }

}
int sclClientClose( SCL *scl )
{
   int critical , buserr , grc ;
   union semun  semarg ;

   sclStartShmAccess() ;
     critical = scl_get_critical_low( scl -> semid , SCL_CRITICAL_TIMEOUT /* seconds */ ) ;
     if( ! critical ){
        if( scl->client[scl->id].flags ){
           (void)shmctl( scl->client[scl->id].shmid , IPC_RMID , NULL ) ;
           (void)semctl( scl->client[scl->id].semid , 0 , IPC_RMID ,  semarg ) ;
           scl->client[scl->id].flags = 0 ;
        }else{
           grc = SCL_NOTFOUND ;
        }
        scl_rel_critical_low( scl -> semid );
     }
   sclEndShmAccess(buserr) ;
   if(buserr)return SCL_BUSERROR ;
   if(critical)return critical ;
       scl -> type = -1 ;
       scl_detach( scl -> base ) ;
       free( (char *) ( scl  -> clientMap ) ) ;
       free( (char *) scl ) ;

       return 0 ;
}
int sclServerClose( SCL *scl )
{
   int critical , buserr , grc ;
   union semun  semarg ;

   sclStartShmAccess() ;
     critical = scl_get_critical_low( scl -> semid , SCL_CRITICAL_TIMEOUT /* seconds */ ) ;
     if( ! critical ){
        if( scl->server[scl->id].flags ){
           (void)semctl( scl->server[scl->id].semid , 0 , IPC_RMID , semarg ) ;
           scl->server[scl->id].flags = 0 ;
        }else{
           grc = SCL_NOTFOUND ;
        }
        scl_rel_critical_low( scl -> semid );
     }
   sclEndShmAccess(buserr) ;
   if(buserr)return SCL_BUSERROR ;
   if(critical)return critical ;

       scl -> type = -1 ;
       scl_detach( scl -> base ) ;
       free( (char *) ( scl  -> clientMap ) ) ;
       free( (char *) scl ) ;

  return 0 ;
}
SCLIO *sclClientGetBuffer( SCL *scl , int *rc  )
{
  if(rc)*rc = 0 ;
  return &( scl -> sclio ) ;
}
SCLIO *sclClientPostAndWait( SCL *scl , SCLIO *sclio , int slot , int wt , int *grc )
{
  int server_semid , server_flags , buserr , critical , rc ;

/* added code to avoid SEGV dead if 'slot' is out of order !!!!  -mg */

   if (slot >= scl->servers) {
         md2pPrintf(md2pMaxLevel,
               "sclClientPostAndWait : slot(%d) >= scl->servers(%d) ",
               slot , scl->servers ) ;
     if (grc)*grc = SCL_NOTFOUND;
     return(NULL);

   }

   sclStartShmAccess() ;
      server_flags = scl->server[slot].flags ;
      server_semid = scl->server[slot].semid ;
   sclEndShmAccess(buserr) ;
   if(buserr){ if(grc)*grc=SCL_BUSERROR ; return NULL ; }
   if( ! server_flags ){ if(grc)*grc=SCL_NOSERVER ; return NULL ; }

   rc = scl_wait_for( server_semid , SCL_CLIENT , wt ) ;
   if( rc == 0 ){

      sclStartShmAccess() ;
         scl->server[slot].requestSlot  = scl->id ;
      sclEndShmAccess(buserr) ;
      if(buserr){  if(grc)*grc=SCL_BUSERROR ; return NULL ; }
      scl_release_from( server_semid , SCL_SERVER ) ;

   }else if( rc == SCL_TIMEOUT ){
     if(grc)*grc=SCL_TIMEOUT ;
     return NULL ;
   }else{
     if(grc)*grc=rc ;
     return NULL ;
   }
   if( rc = scl_wait_for( scl->client[scl->id].semid , 0 , 100000 ) ){
      if(grc)*grc=rc ;
      return NULL ;
   }
   return &( scl -> sclio ) ;
}
SCLIO *sclServerWait( SCL *scl , int wt , int *rc )
{

   int grc , buserr , slot , client_shmid ;
   scl_local_client_map  * client_map ;
   grc = 0 ;

   grc = scl_wait_for( scl -> server_semid , SCL_SERVER , wt ) ;
   if( grc < 0 ){
     if( rc )*rc = grc ;
     return NULL ;
   }
   scl -> sclio.requestSlot = -1 ;
   sclStartShmAccess() ;
      slot = scl -> server[scl->id].requestSlot ;
      if( ( slot < 0 ) ||
          ( slot >= scl -> head -> servers ) )grc = SCL_NOTFOUND ;

      if( ! scl -> client[slot].flags  ){
         /*
          * client died ( we simply correct our stuff )
          */
         (void)scl_release_from( scl -> semid , SCL_CLIENT );
         grc = /* SCL_AGAIN ; */ SCL_TIMEOUT ;
      }else{
         scl -> sclio.requestSlot = slot ;
         scl -> client_semid      = scl -> client[slot].semid ;
         client_shmid             = scl -> client[slot].shmid ;
      }
   sclEndShmAccess(buserr) ;
   if(buserr || grc ){
     if(rc)*rc = buserr ? SCL_BUSERROR : grc ;
     return NULL ;
   }
   /*
    * did we already attach the shm of the new client ?
    */
   client_map = &scl->clientMap[scl -> sclio.requestSlot] ;

   if( client_map -> shmid != client_shmid ){
     /*
      * may be the first time or client changed
      */
      if( client_map -> head  != NULL ){
        /*
         * client changed
         */
        shmdt( (char *)(client_map -> head) ) ;
      }
      client_map -> head = (genShmHead *)shmat(client_shmid,NULL,0) ;
      if( (int) client_map -> head == -1 )client_map -> head = NULL ;
      if( client_map -> head == NULL ){
         /*
          * if we are not allowed to attach more shm's let's detach some
          * old ones.
          *
          */
          if( errno == EMFILE ){
              int i ;
              for( i = 0 ; i < scl->clientMaps ; i++ ){
                 if( scl->clientMap[i].head != NULL ){
                     shmdt( (void*)scl->clientMap[i].head ) ;
                     scl->clientMap[i].head  = NULL ;
                     scl->clientMap[i].shmid = -1 ;
                 }
              }
              client_map -> head = (genShmHead *)shmat(client_shmid,NULL,0) ;
              if( (int) client_map -> head == -1 )client_map -> head = NULL ;

          }
      }
      if( client_map -> head == NULL ){
         /* if(rc)*rc = SCL_SHMMALLOC ; */
         if(rc)*rc = errno ;
         /*
          * we have to reset our own state and to release the
          * client if still possible
          */
         (void)scl_release_from( scl -> server_semid , SCL_CLIENT );
         /*
          * the next step will bring the client into problems
          * but he should recognize, that the request area didn't
          * change.
          */
         (void)scl_release_from( scl -> client_semid , 0 ) ;
         return NULL ;
      }else{
         client_map -> shmid = client_shmid ;
         client_map -> semid = scl -> client_semid ;
      }

   }else{
      /*
       * there is the believe, that that the buffer is
       * still valid.
       */
   }
   scl -> sclio.buffer = (char *)client_map -> head ;
   scl -> sclio.size   = client_map -> head -> size ;
   return &(scl -> sclio) ;
}
int sclServerReady( SCL *scl , SCLIO *sclio )
{
 int rc , buserr , client_semid ;

   if( ( ! sclio ) || ( sclio -> requestSlot < 0 ) )return SCL_NOTFOUND ;

   if( rc = scl_release_from( scl -> server_semid , SCL_CLIENT ) )return rc ;

   return scl_release_from( scl -> client_semid , 0 ) ;
}
char * sclError( char *txt , int rc )
{
static char error_text[128] ;
static char *sclErrorText[] = {
 " Unknown(0) " , " Unknown(1) " , " Unknown(2) " ,
 " Bus Error " , " Protocol Violation " , " Timeout " ,
 " Not Found " , " Shm Allocation Error " , " Object Already Exists " ,
 " Allocation Error " , " Service Not Available " , NULL

};
  txt = txt ? txt : "" ;

  if( ( rc > SCL_ERRORBASE ) || ( rc < SCL_LAST ) )rc = SCL_ERRORBASE ;
  sprintf( error_text,"%s %s(%d)" , txt , sclErrorText[-rc+SCL_ERRORBASE] , rc ) ;
  return error_text ;
}
