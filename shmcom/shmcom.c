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
#include <strings.h>
#include <stdlib.h>
#include <unistd.h>
#include "shmcom.h"
#include "myts.h"

extern int scl_install( key_t key , int c , int s );
extern int sclClientClose( SCL *scl );


main( int argc , char *argv[] )
{
  key_t key ;
  long size ;
  int rc , wt , clients , servers , shmid ;
  char *ptr ;
  typedef struct req_buffer {
      long type ;
      long request ;
  } reqBuffer ;
  

  if( argc < 2 )goto usage ;
  
  if( ! strcmp( argv[1] , "headcreate" ) ){
     if( argc != 4 )goto usage ;
     sscanf( argv[2] , "%x" , &key ) ; 
     sscanf( argv[3] , "%d" , &size ) ; 
     rc = scl_create( key , size ) ;
     printf( "shmid %d\n" , rc ) ;
     if( rc )exit(1) ;
  }else if( ! strcmp( argv[1] , "headdelete" ) ){
     if( argc != 3 )goto usage ;
     sscanf( argv[2] , "%x" , &key ) ; 
     rc = scl_discard( key  ) ;
     if( rc ){ fprintf(stderr, "error %d\n",rc) ; exit(1) ; } 
  }else if( ! strcmp( argv[1] , "install" ) ){
     if( argc != 5 )goto usage ;
     sscanf( argv[2] , "%x" , &key ) ; 
     sscanf( argv[3] , "%d" , &clients ) ; 
     sscanf( argv[4] , "%d" , &servers ) ; 
     rc = scl_install( key , clients , servers ) ;
     if( rc < 0 ){ fprintf(stderr, "scl_install :  %d %d\n",rc,errno) ; exit(1) ; }
  }else if( ! strcmp( argv[1] , "show" ) ){
     if( argc != 3 )goto usage ;
     if( *argv[2] == 's' ){
        sscanf( argv[2]+1 , "%d" , &shmid ) ; 
        rc = scl_sprint( shmid  ) ;
     }else{
        sscanf( argv[2] , "%x" , &key ) ; 
        rc = scl_print( key  ) ;
     }
     if( rc ){ fprintf(stderr, "scl_print :  %d %d\n",rc,errno) ; exit(1) ; }
  }else if( ! strcmp( argv[1] , "stat" ) ){
     if( argc != 3 )goto usage ;
     if( *argv[2] == 's' ){
        fprintf(stderr, "scl_xprint :  s mode not supported\n") ; exit(1) ; 
     }else{
        sscanf( argv[2] , "%x" , &key ) ; 
        rc = scl_xprint( key  ) ;
     }
     if( rc ){ fprintf(stderr, "scl_print :  %d %d\n",rc,errno) ; exit(1) ; }
  }else if( ! strcmp( argv[1] , "dump" ) ){
     if( argc != 3 )goto usage ;
     if( *argv[2] == 's' ){
        sscanf( argv[2]+1 , "%d" , &shmid ) ; 
        ptr =  scl_sattach( shmid ) ;
     }else{
        sscanf( argv[2] , "%x" , &key ) ; 
        ptr =  scl_attach( key ) ;
     }
     if( ! ptr ){ fprintf(stderr, "scl_(s)attach : can't attach\n") ; exit(1) ; } 

     scl_dump_memory( stdout , (unsigned char*)ptr, 0L , 
                      ((distShmHead*)ptr) -> gen.size );
     
  }else if( ! strcmp( argv[1] , "addclient" ) ){
     if( argc < 3 )goto usage ;
     sscanf( argv[2] , "%x" , &key ) ; 
     size = 1024 ;
     if( argc > 3 )sscanf( argv[3] , "%d" , &size ) ;
     ptr =  scl_attach( key ) ;
     if( ! ptr ){ fprintf(stderr, "scl_attach : can't attach\n") ; exit(1) ; } 

     rc = scl_add_client( ptr , size );
     if( rc ){ fprintf(stderr, "scl_add_client :  %d %d\n",rc,errno) ; exit(1) ; }
  }else if( ! strcmp( argv[1] , "rmclient" ) ){
     if( argc != 4 )goto usage ;
     sscanf( argv[2] , "%x" , &key ) ; 
     sscanf( argv[3] , "%d" , &size ) ;
     ptr =  scl_attach( key ) ;
     if( ! ptr ){ fprintf(stderr, "scl_attach : can't attach\n") ; exit(1) ; } 

     rc = scl_rm_client( ptr , size );
     if( rc ){ fprintf(stderr, "scl_rm_client :  %d %d\n",rc,errno) ; exit(1) ; }
  }else if( ! strcmp( argv[1] , "rmserver" ) ){
     if( argc != 4 )goto usage ;
     sscanf( argv[2] , "%x" , &key ) ; 
     sscanf( argv[3] , "%d" , &size ) ;
     ptr =  scl_attach( key ) ;
     if( ! ptr ){ fprintf(stderr, "scl_attach : can't attach\n") ; exit(1) ; } 

     rc = scl_rm_server( ptr , size );
     if( rc ){ fprintf(stderr, "scl_rm_server :  %d %d\n",rc,errno) ; exit(1) ; }
  }else if( ! strcmp( argv[1] , "addserver" ) ){
     if( argc != 4 )goto usage ;
     sscanf( argv[2] , "%x" , &key ) ; 
     sscanf( argv[3] , "%d" , &size ) ;
     ptr =  scl_attach( key ) ;
     if( ! ptr ){ fprintf(stderr, "scl_attach : can't attach\n") ; exit(1) ; } 

     rc = scl_add_server( ptr , size );
     if( rc ){ fprintf(stderr, "scl_add_server :  %d %d\n",rc,errno) ; exit(1) ; }
  }else if( ! strcmp( argv[1] , "xclient" ) ){
     int slot , count , bCounter ;
     SCL *scl ;
     SCLIO *sclio ;
     ts start , now , last ;
     int events ;
     reqBuffer *rb ;
  
     if( argc != 5 )goto usage ;
     sscanf( argv[2] , "%x" , &key ) ; 
     sscanf( argv[3] , "%d" , &slot ) ;
     sscanf( argv[4] , "%d" , &count ) ;
     scl = sclClientOpen(  key , 1024 , &rc );
     if( ! scl ){ fprintf(stderr, "sclClientOpen :%d %d\n",rc,errno) ; exit(1) ; } 
     tsGet( &start ) ;
     tsGet( &last ) ;
     events = 0 ;
     for( bCounter=0;;bCounter++){
     
        sclio = sclClientGetBuffer( scl , NULL ) ;
        rb = (reqBuffer *)sclioBuffer(sclio) ;
        rb -> type    = 555 ;
        rb -> request = bCounter ;
        
        sclio = sclClientPostAndWait( scl , sclio , slot , 10 , &rc ) ;
        if( ! sclio ){
            if( rc == SCL_TIMEOUT ){
               fprintf(stderr," sclClientPostAndWait : Timeout\n");
               continue ;
            }else{
               fprintf(stderr," sclClientPostAndWait : Problem %d(%d)\n",rc,errno);
               break ;
            }
        }else{
           rb = (reqBuffer *)sclioBuffer(sclio) ;
           if( ( rb -> type != 666 ) || ( rb -> request != ( bCounter+1) ) ){
              fprintf(stderr," PANIC : got wrong request answer\n");
           }
        }
        events++ ;
        tsGet( &now ) ;
        tsSub( &now , &last ) ;
        if( tsSec( &now ) > 1.0 ){
           tsGet( &last ) ;
           printf( " Interaction Rate : %f actions/sec\n" ,
                   ((float)events)/tsSec( &now ) ) ;
           fflush(stdout);
           events = 0 ; 
           tsGet( &now ) ;
           tsSub( &now , &start ) ;
           if( ((float)count) < tsSec( &now ) )break ;
        }
     }
     rc = sclClientClose( scl );
     if( rc ){ fprintf(stderr, "sclClose :  %d %d\n",rc,errno) ; exit(1) ; }
  }else if( ! strcmp( argv[1] , "xserver" ) ){
     int slot , bCounter;
     SCL *scl ;
     SCLIO *sclio ;
     ts start , now , last ;
     int events ;
     reqBuffer *rb ;
  
     if( argc != 4 )goto usage ;
     sscanf( argv[2] , "%x" , &key ) ; 
     sscanf( argv[3] , "%d" , &slot ) ;
     scl = sclServerOpen(  key , slot , &rc );
     if( ! scl ){ fprintf(stderr, "sclServerOpen :%d %d\n",rc,errno) ; exit(1) ; } 
     tsGet( &last ) ;
     events = 0 ;
     for(bCounter=0;;bCounter++){
        sclio = sclServerWait( scl , 10 , &rc ) ;
        if( ! sclio ){
            if( rc == SCL_TIMEOUT ){
               fprintf(stderr," sclServerWait : Timeout\n");
               continue ;
            }else{
               fprintf(stderr," sclServerWait : Problem %d(%d)\n",rc,errno);
               break ;
            }
        }
        rb = (reqBuffer*) sclioBuffer(sclio);
        if( rb -> type != 555 ){
           fprintf(stderr," PANIC : got wrong request structure\n");
        }else{
           rb -> type = 666 ;
           rb -> request ++ ;
        }
        events ++ ;
        rc = sclServerReady( scl , sclio );
        if(rc){
           fprintf(stderr," sclServerReady : Problem %d %d\n" , rc , errno ) ;
           break ;
        }
        tsGet( &now ) ;
        tsSub( &now , &last ) ;
        if( tsSec( &now ) > 1.0 ){
           tsGet( &last ) ;
           printf( " Interaction Rate : %f actions/sec\n" ,
                   ((float)events)/tsSec( &now ) ) ;
           fflush(stdout);
           events = 0 ; 
        }
     }
     rc = sclServerClose( scl );
     if( rc ){ fprintf(stderr, "scl_add_server :  %d %d\n",rc,errno) ; exit(1) ; }
  }else if( ! strcmp( argv[1] , "server" ) ){
     ts start , now , last ;
     int events ;
     if( argc != 4 )goto usage ;
     sscanf( argv[2] , "%x" , &key ) ; 
     sscanf( argv[3] , "%d" , &size ) ;
     ptr =  scl_attach( key ) ;
     if( ! ptr ){ fprintf(stderr, "scl_attach : can't attach\n") ; exit(1) ; } 

     rc = scl_add_server( ptr , size );
     if( rc ){ fprintf(stderr, "scl_add_server :  %d %d\n",rc,errno) ; exit(1) ; }
     tsGet( &last ) ;
     events = 0 ;
     while( 1){
        rc = scl_server_get( ptr , size , 10 ) ;
        if( rc == SCL_TIMEOUT ){ printf( " Timeout\n" ) ; continue ; }
        else if( rc != 0 ){ printf( " scl_server_get : %d %d\n" , rc , errno ) ; break ;}
        printf( " Got Message\n" ) ;
        /* bloody shm stuff */
        printf( " Answer Ready\n" ) ;
        rc = scl_client_done( ptr , size ) ;
        if( rc < 0 ){
            printf( " scl_client_done : %d %d\n" , rc , errno ) ;
        }else{
           printf( " Send answer ready for client slot %d\n" , rc ) ;
           rc = scl_answer_ready( ptr , size ) ;
           if( rc < 0 )printf( " scl_answer_ready : %d %d\n" , rc , errno ) ;
           events++ ;
        }
        tsGet( &now ) ;
        tsSub( &now , &last ) ;
        if( tsSec( &now ) > 1.0 ){
           tsGet( &last ) ;
           printf( " Interaction Rate : %f actions/sec\n" , 
                   ((float)events) / tsSec( &now ) ) ;
           events = 0 ; 
        }
     }
     rc = scl_rm_server( ptr , size );
     if( rc ){ fprintf(stderr, "scl_rm_server :  %d %d\n",rc,errno) ; exit(1) ; }
  }else if( ! strcmp( argv[1] , "client" ) ){
     int cSlot , count , j , events ;
     ts start , now , last ;
     if( argc < 4 )goto usage ;
     sscanf( argv[2] , "%x" , &key ) ; 
     sscanf( argv[3] , "%d" , &size ) ;
     if( argc > 4 ) sscanf( argv[4] , "%d" , &count ) ;
     else   count = 1 ; 
     ptr =  scl_attach( key ) ;
     if( ! ptr ){ fprintf(stderr, "scl_attach : can't attach\n") ; exit(1) ; } 
     
     cSlot = scl_add_client( ptr , 1024 );
     if( cSlot < 0 ){ fprintf(stderr, "scl_add_clientv :  %d %d\n",cSlot,errno) ; exit(1) ; }
     printf( " Client got Slot %d\n" , cSlot ) ;
     tsGet( &start ) ;
     tsGet( &last ) ;
     events = 0 ;
     for( ;; ){
        rc = scl_client_get( ptr , size , cSlot , 10 ) ;
        if( rc == SCL_TIMEOUT ){
           printf( " scl_client_get : Timeout\n" ) ;
        }else if( rc != 0 ){
           printf( " scl_client_get : %d %d\n" , rc , errno ) ; 
        }else{
           printf( " Sending message\n" ) ;
          /* sleep(5) ;*/
           rc = scl_server_done( ptr , size ) ;
           if( rc < 0 ){
              printf( " scl_server_done : %d %d\n" , rc , errno ) ;
           }else{
              printf( " Will call answer_wait with %d\n" , cSlot ) ;
              /*sleep(5);*/
              rc = scl_answer_wait( ptr , cSlot , 5 ) ;
              if( rc == SCL_TIMEOUT ){
                 printf( " scl_answer_wait : Timeout\n" ) ;
              }else if( rc ){
                 printf( " scl_answer_wait : %d %d\n" , rc , errno ) ;
              }else{
                 printf( " scl_answer_wait : Ready\n"  ) ;
                 events++ ;
              }
           }
        }
        tsGet( &now ) ;
        tsSub( &now , &last ) ;
        if( tsSec( &now ) > 1.0 ){
           tsGet( &last ) ;
           printf( " Interaction Rate : %f actions/sec\n" , 
                   ((float)events) / tsSec( &now ) ) ;
           events = 0 ; 
           tsGet( &now ) ;
           tsSub( &now , &start ) ;
           if( ((float)count) < tsSec( &now ) )break ;
        }
     }
     rc = scl_rm_client( ptr , cSlot );
     if( rc ){ fprintf(stderr, "scl_rm_client :  %d %d\n",rc,errno) ; exit(1) ; }
     
  }else if( ! strcmp( argv[1] , "getsemaphore" ) ){
     if( argc != 4 )goto usage ;
     sscanf( argv[2] , "%x" , &key ) ; 
     sscanf( argv[3] , "%d" , &wt ) ; 
     ptr =  scl_attach( key ) ;
     if( ! ptr ){ fprintf(stderr, "scl_attach : can't attach\n") ; exit(1) ; } 

     rc = scl_get_critical( ptr , 1 );
     if( rc ){ fprintf(stderr, "scl_get_critical :  %d %d\n",rc,errno) ; exit(1) ; }
     sleep(wt) ; 
     rc = scl_rel_critical( ptr  );
     if( rc ){ fprintf(stderr, "scl_rel_critical :  %d %d\n",rc,errno) ; exit(1) ; } 
  }else goto usage ;
  
  exit(0) ;
  
 usage :
 
   printf( " USAGE : %s headcreate <key> <size>\n" , argv[0] ) ;
   printf( " USAGE : %s headdelete <key>\n" , argv[0] ) ;
   printf( " USAGE : %s getsemaphore <key> <time>\n" , argv[0] ) ;
   printf( " USAGE : %s show <key>\n" , argv[0] ) ;
   printf( " USAGE : %s stat <key>\n" , argv[0] ) ;
   printf( " USAGE : %s dump <key>\n" , argv[0] ) ;
   printf( " USAGE : %s install <key> <clients> <servers>\n" , argv[0] ) ;
   printf( " USAGE : %s addclient <key> [<size>]\n" , argv[0] ) ;
   printf( " USAGE : %s rmclient <key> <slot>\n" , argv[0] ) ;
   printf( " USAGE : %s addserver <key> <slot>\n" , argv[0] ) ;
   printf( " USAGE : %s rmserver <key> <slot>\n" , argv[0] ) ;
   printf( " USAGE : %s server <key> <slot>\n" , argv[0] ) ;
   printf( " USAGE : %s client <key> <slot>\n" , argv[0] ) ;
   
   exit(1);

}
