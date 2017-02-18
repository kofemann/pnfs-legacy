#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <ctype.h>

void db_intr_handler( int sig )
{
  fprintf(stderr," Process %d got signal %d\n",getpid(),sig);
  return ; 
}


main()
{
     struct sigaction newSigAction ;
     
     newSigAction.sa_handler = db_intr_handler ;
     newSigAction.sa_flags   = 0 ;
     sigemptyset(&newSigAction.sa_mask);
     /*sigaddset(&newSigAction.sa_mask,SIGBUS); */
     sigaction( SIGTERM , &newSigAction , NULL ) ;
     
     
     while(1)sleep(2);
}
