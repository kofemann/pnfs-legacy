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
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>


main( int argc , char * argv[] ){
  int semid , rc ;
  struct semid_ds big ;
  struct sem sems[16] ;
  
        union semun {
               int val;
               struct semid_ds *buf;
               ushort *array;
          } arg;

 if( argc < 2 ){
    fprintf(stderr," USAGE : %s <semid>\n",argv[0] ) ;
    exit(4);
 }
 sscanf( argv[1] , "%d" , &semid ) ;
 arg.buf = &big ;
 big.sem_base = sems ;
 printf( " --> %x\n" , big.sem_base ) ;
 rc = semctl( semid , 0 , IPC_STAT , &arg ) ;
 if( rc < 0 ){
    fprintf(stderr," Problem in semctl : %d\n" , rc ) ;
    exit(4);
 }
 printf( " semid says : %d\n" , rc ) ;
 printf( " nsem : %d\n" , big.sem_nsems  ) ;
 printf( " --> %x\n" , big.sem_base ) ;
 printf( " semval : %d\n" , big.sem_base -> semval  ) ;
 exit(0);

}
