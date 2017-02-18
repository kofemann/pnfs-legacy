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
#include <sys/stat.h>
#include <sys/time.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include "DiffEx.h"

#define KEEP_OPEN   DEX_KEEP_OPEN
#define P_EXIT(n)   {e=(n);goto BAD;}


FILE * debugOutput = NULL ;
DiffEx * newDiffEx( char * dbName , char * backupDir , int mode  , int * error ){

   DiffEx * de ;
   int e = 0 , rc = 0 ;
   
   if( debugOutput == NULL )debugOutput = fopen("/tmp/DiffEx.log","a" ) ;
   if( debugOutput != NULL ){
      fprintf(debugOutput,"newDiffEx ...\n");
      fflush(debugOutput);
   }
   
   de = (DiffEx*)malloc(sizeof(DiffEx)) ;
   de -> read   = DE_read ; 
   de -> write  = DE_write ; 
   de -> free   = DE_free ; 
   de -> sync   = DE_sync ; 
   de -> setMaxWriteCount  = DE_setMaxWriteCount ; 
   de -> getMaxWriteCount  = DE_getMaxWriteCount ; 
   de -> setMaxWriteTime   = DE_setMaxWriteTime ; 
   de -> getMaxWriteTime   = DE_getMaxWriteTime ; 
   de -> setKeepOpen       = DE_setKeepOpen ;
   de -> isEnabled         = DE_isEnabled ;
   
   de -> openMode      = mode ;
   de -> enabled       = 1 ;
   de -> maxWriteCount = 100 ;
   de -> maxWriteTime  = 5 * 60 ;
   
   de -> writing       = 0 ;
   de -> creationTime  = (time_t)0 ;
   de -> flags         = 0 ;
   de -> handle        = -1 ;
   
   de -> fileName      = NULL ;
   de -> dbName        = strdup(dbName) ;
   de -> backupDir     = NULL ;
   de -> tmpFileName   = NULL ;
   
   if( mode & DEX_READ ){
      de -> tmpFileName   = strdup( de -> dbName ) ;
      if( ( rc = DE__openFileRead(de) ) )P_EXIT(rc) ;
      return de ;
   }else{
      de -> backupDir     = strdup(backupDir);
      de -> tmpFileName   = (char*)malloc(strlen(dbName)+
                                          strlen(backupDir)+
                                          strlen("current")+8 ) ;
      sprintf( de->tmpFileName , 
               "%s/%s.current",
               backupDir ,
               dbName             ) ;

      if( ( rc = DE__createFile( de , mode ) ) == EEXIST ){
         DE__createNewPath( de ) ;
         if( rename( de -> tmpFileName , de -> fileName ) )P_EXIT(1001) ;
      }else if( rc != 0 ){
         P_EXIT(rc) ;
      }


      return de ;
   }
   BAD :  if( error != NULL )*error = rc ;
          de->free(de) ;
          return NULL ;
}
static void  DE__freeMemory( DiffEx * de ){
   if( de->fileName    != NULL )free(de->fileName) ;
   if( de->tmpFileName != NULL )free(de->tmpFileName) ;
   if( de->dbName      != NULL )free(de->dbName) ;
   if( de->backupDir   != NULL )free(de->backupDir) ;
   free(de);
}
static void  DE_free( DiffEx * de ){
    DE__closeFile(de) ;
    
    if( ! ( de -> openMode & DEX_READ ) ){
       DE__createNewPath( de ) ;
       rename( de -> tmpFileName , de -> fileName ) ;
    }
    DE__freeMemory(de) ;
    return ;
}
static void  DE__createNewPath( DiffEx * de ){
#ifdef USE_SECONDS_ONLY
   time_t now = time(NULL);
   de -> fileName = (char*)malloc(strlen(de->dbName)+
                                  strlen(de->backupDir)+16) ;
   sprintf( de->fileName , 
            "%s/%s.%d",
            de->backupDir ,
            de->dbName ,
            now  ) ;
#else
   struct timeval now ;
   gettimeofday( &now , NULL ) ;
   de -> fileName = (char*)malloc(strlen(de->dbName)+
                                  strlen(de->backupDir)+32) ;
   sprintf( de->fileName , 
            "%s/%s.%ld%03ld",
            de->backupDir ,
            de->dbName ,
            now.tv_sec , now.tv_usec/1000 ) ;
#endif
   return;
}
#define DE_NOT_ENABLED   (-1003) 
static int  DE__createFile( DiffEx * de , int mode ){
   int handle = -1 ;
   handle = open( de -> tmpFileName , O_CREAT | O_EXCL | O_WRONLY , mode ) ;
   if( debugOutput != NULL ){
      fprintf(debugOutput,"DE__createFile %d\n",handle);
      fflush(debugOutput);
   }
   if( handle < 0 )return errno ;
   close(handle);
   return 0 ;
}
static int  DE__openFile( DiffEx * de ){
   if( ! de->enabled )return DE_NOT_ENABLED ;
   if( de -> handle < 0 ){
      if( ! ( de -> writing ) ){
         time( &(de -> creationTime) ) ;
         de -> writeCount = 0 ;
         de -> writing    = 1 ;
      }
      de -> handle = open( de -> tmpFileName , 
                           O_CREAT | O_APPEND | O_WRONLY ,
                           de -> openMode  ) ;
      if( debugOutput != NULL ){
         fprintf(debugOutput,"DE__openFile %d\n",de->handle);
         fflush(debugOutput);
      }
      if( de -> handle < 0 )return errno ;
   }
   return 0 ;
}
static int  DE__openFileRead( DiffEx * de ){
   if( ! de->enabled )return DE_NOT_ENABLED ;
   if( de -> handle < 0 ){
      de -> handle = open( de -> tmpFileName , O_RDONLY , 0 ) ;
      if( debugOutput != NULL ){
         fprintf(debugOutput,"DE__openFileRead %d\n",de->handle);
         fflush(debugOutput);
      }
      if( de -> handle < 0 )return errno ;
   }
   return 0 ;
}
static int DE__closeFile( DiffEx * de ){
      if( debugOutput != NULL ){
         fprintf(debugOutput,"DE__closeFile %d\n",de->handle);
         fflush(debugOutput);
      }
   if( de -> handle >= 0 ){
      close( de -> handle ) ;
      de -> handle = -1 ;
   }
   return 0 ;
}
static void  DE_setKeepOpen( DiffEx * de, int keepOpen ){
   if( keepOpen )de -> flags |= KEEP_OPEN ;
   else de -> flags &= ~ KEEP_OPEN  ;
   return ;
}
static int   DE_isEnabled( DiffEx * de  ){
   return de -> enabled ;
}
static int   DE_read( DiffEx * de , char * data , int * len ){
    int rc = 0  ;
    int blockSize = 0 ;
    if( ! de->enabled )return DE_NOT_ENABLED ;
    rc = read( de -> handle , (char *)&blockSize , sizeof( blockSize ) ) ;
    if( debugOutput != NULL ){
       fprintf(debugOutput,"DE_read %d got=%d bs=%d\n",de->handle,rc,blockSize);
       fflush(debugOutput);
    }
    if( rc == 0 ){
       de -> enabled = 0 ;
       DE__closeFile(de) ;
       return -1 ;
    }
    if( rc != sizeof(blockSize) ){
       int e = errno ;
       de -> enabled = 0 ;
       DE__closeFile(de) ;
       return e ;
    }
    if( blockSize > 4*1024 ){
       de -> enabled = 0 ;
       DE__closeFile(de) ;
       return 1006 ;
    }
    if( blockSize > (*len) )return 1007 ;
    
    rc = read( de -> handle , data , blockSize ) ;
    if( debugOutput != NULL ){
       fprintf(debugOutput,"DE_read %d got=%d\n",de->handle,rc);
       fflush(debugOutput);
    }
    if( rc != blockSize ){
       int e = errno ;
       de -> enabled = 0 ;
       DE__closeFile(de) ;
       return e ;
    }
    (*len) = blockSize ;
    return 0;
}
static int   DE_write( DiffEx * de , char * data , int len ){
    int rc = 0  ;
    if( ! de->enabled )return DE_NOT_ENABLED ;
    if( (rc = DE__openFile(de)) ){
       de -> enabled = 0 ;
       return rc ;
    }
    
      if( debugOutput != NULL ){
         fprintf(debugOutput,"DE_write %d len=%d rc=%d\n",de->handle,len,rc);
         fflush(debugOutput);
      }
    rc = write( de->handle , (char*)&len , sizeof(len) ) ;
    if( rc != sizeof(len) ){
       int e = errno ;
       de -> enabled = 0 ;
       DE__closeFile(de) ;
       return e ;
    }
    
    rc = write( de->handle , data , len ) ;
    
    if( rc != len ){
       int e = errno ;
       de -> enabled = 0 ;
       DE__closeFile(de) ;
       return e ;
    }
    if( ! (de->openMode&KEEP_OPEN) )DE__closeFile(de) ;
    
    de -> writeCount ++ ;
    return DE_sync(de,0);
}
static int  DE_sync( DiffEx * de , int forceNewCycle ){
    time_t now = time(NULL);
    
    if( ! de -> writing )return 0 ;
    
    if( (   de -> writeCount > 0                                   ) &&
        ( ( forceNewCycle                                     ) ||
          ( de -> writeCount >= de -> maxWriteCount           ) ||
          ( now > ( de -> creationTime + de -> maxWriteTime ) )    )
        ){
        
       DE__closeFile(de) ;
       DE__createNewPath( de ) ;
       if( rename( de -> tmpFileName , de -> fileName ) ){
          de->enabled = 0 ;
          return 1001 ;
       }
       de -> writing = 0 ;
    }
    return 0 ;
}
static void  DE_setMaxWriteCount( DiffEx * de , int writeCount ){
    de -> maxWriteCount = writeCount ;
    return ;
}
static int   DE_getMaxWriteCount( DiffEx * de ){
    return de -> maxWriteCount ;
}
static void  DE_setMaxWriteTime( DiffEx * de , int maxSeconds ){
    de -> maxWriteTime = maxSeconds ;
    return ;
}
static int  DE_getMaxWriteTime( DiffEx * de ){
    return de -> maxWriteTime ;
}
#ifdef _DIFF_EX_USE_MAIN 
int main( int argc , char * argv[] ){
   int result = 0 , i = 0 , size , rc ;
   char * x = "hallo du dickes ei" ;
   char data[1024] ;
   DiffEx * de = NULL ;
   
   if( argc < 2 ){
      fprintf(stderr,"Usage : %s <filename>\n",argv[0] ) ;
      exit(4);
   }
   de = newDiffEx( argv[1] , NULL , DEX_READ , &result ) ;
   if( de == NULL ){
      fprintf( stderr , "NewDiff failed : %d\n",result) ;
      exit(4);
   }
   while( 1 ){
      size = sizeof( data ) -1 ;
      rc = de->read(de,data,&size) ;
      if( rc )break ;
      data[size] = '\0' ;
      printf( "Got %d : %s\n",size,data) ;
   }
   printf( "Terminated by : %d\n",rc ) ;
   de->free(de) ;
   /*
   DiffEx * de = newDiffEx( "db-1" , "." , 0600 , &result ) ;
   if( de == NULL ){
      fprintf( stderr , "NewDiff failed : %d\n",result) ;
      exit(4);
   }
   de->setMaxWriteCount(de , 30 ) ;
   de->setMaxWriteTime(de , 3 ) ;
   for( i = 0 ; i < 10 ; i++ ){
       if( result = de->write(de , x , strlen(x) ) )break;
       sleep(1);
   }
   if( result )fprintf( stderr , "Failed : %d\n", result ) ;
   de->free(de) ;
   */
   return 0 ;
}
#endif
