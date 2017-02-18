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
#include <fcntl.h>

#include "md2types.h"
#include "md2fs.h"

void check( MDL *mdl , mdRecord * mdr ) ;
void checkDirHash( MDL * mdl , mdRecord * mdr ) ;
void checkFileData( MDL * mdl , mdRecord * mdr ) ;
void checkDirData( MDL * mdl , mdRecord * mdr ) ;
void checkDirInode( MDL * mdl , mdRecord * mdr ) ;
void checkFileInode( MDL * mdl , mdRecord * mdr ) ;

main( int argc ,char *argv[] )
{

  int rc  ;
  mdRecord r ;
  MDL *mdlIn ;
  char *infile ;

  infile = argv[1] ;

  mdlIn  = NULL ;
 
  if( argc < 2 ){
    fprintf(stderr," USAGE : %s <dbFile>\n",argv[0]);
    exit(1) ;
  }

   rc = 0 ;
   if( ! ( mdlIn = md2OpenReadWrite( infile , &rc, 0 ) ) ){
       fprintf( stderr," Can't open %s : %d\n" , infile , rc );
       exit(1);
   }
   for( md2GetFirstKey( mdlIn , &r.head.ID ) ;
        ! mdIsNullID( r.head.ID ) ;
        md2GetNextKey( mdlIn , r.head.ID , &r.head.ID ) ){


      if( rc = md2GetRecord( mdlIn , &r , 0 ) ){
         fprintf(stdout," Problem %d at id %s\n",rc,mdStringID(r.head.ID));
      }else{
         fprintf(stdout,"%s %s\n",mdStringID(r.head.ID),md2PrintTypes( r.head.type ));
         check( mdlIn , &r ) ;
      }
   }


   if( mdlIn )md2Close( mdlIn ) ;
   exit(0);
}
void check( MDL *mdl , mdRecord * mdr ){
    if( mdIsType( mdr->head.type , mdtInode ) ){

       if( mdIsType( mdr->head.type , mdtDirectory ) ){
           checkDirInode( mdl , mdr ) ;
       }else if( mdIsType( mdr->head.type , mdtRegular ) ){
           checkFileInode( mdl , mdr ) ;
       }else if( mdIsType( mdr->head.type , mdtLink ) ){
       }else if( mdIsType( mdr->head.type , mdtTag ) ){
       }

    }else if(   mdIsType( mdr->head.type , mdtRoot ) ){
       fprintf(stdout," Next Free ID  : %s\n", mdStringID(mdr -> body.root.nextFreeID) ) ;
       fprintf(stdout," Database ID   : %d\n", mdr -> body.root.DB ) ;
       fprintf(stdout," Config ID     : %s\n", mdStringID(mdr -> body.root.configID) ) ;
       fprintf(stdout," Stat Dirs     : %d\n", mdr -> body.root.statistics.dirObjects ) ;
       fprintf(stdout," Stat Files    : %d\n", mdr -> body.root.statistics.fileObjects ) ;
       return ;
    }else if(   mdIsType( mdr->head.type , mdtData ) ){
       if( mdIsType( mdr->head.type , mdtDirectory ) ){
           checkDirData( mdl ,  mdr  ) ;
       }else if( mdIsType( mdr->head.type , mdtRegular ) ){
           checkFileData( mdl , mdr ) ;
       }
    }else if(   mdIsType( mdr->head.type , mdtHash ) ){
       if( mdIsType( mdr->head.type , mdtDirectory ) ){
           checkDirHash( mdl , mdr ) ;
       }else if( mdIsType( mdr->head.type , mdtRegular ) ){
           checkDirHash( mdl , mdr ) ;
       }
    }

}
void checkFileInode( MDL * mdl , mdRecord * mdr ){
   int i ,rc ;
   mdRecord r ;
   md_file_inode *fileInode = &( mdr -> body.fileInode ) ;
   for( i = 0 ; i < 8 ; i++ ){
      r.head.ID = fileInode -> attr[i].chain ;
      if( mdIsNullID( r.head.ID  ) )continue ;
      if( rc = md2GetRecord( mdl , &r , 0 ) ){

         fprintf(stdout,"%s %s missed ",
              mdStringID(mdr->head.ID),
              md2PrintTypes(mdr->head.type ) ) ;
         fprintf(stdout,"%s (%d)\n",
              mdStringID(r.head.ID),
              rc ) ;
       }
   }
   
   
}
void checkDirInode( MDL * mdl , mdRecord * mdr ){

   int i ,rc ;
   mdRecord r ;
   md_dir_inode * dirInode = &(mdr -> body.dirInode) ;
   for( i = 0 ; i < dirInode -> hashInfo.rows ; i++ ){
      r.head.ID = dirInode -> hashHandle[i] ;
      if( mdIsNullID( r.head.ID  ) )continue ;
      if( rc = md2GetRecord( mdl , &r , 0 ) ){

         fprintf(stdout,"%s %s missed ",
              mdStringID(mdr->head.ID),
              md2PrintTypes(mdr->head.type ) ) ;
         fprintf(stdout,"%s (%d)\n",
              mdStringID(r.head.ID),
              rc ) ;
       }
   }
}
void checkDirData( MDL * mdl , mdRecord * mdr ){
   int i ,rc ;
   mdRecord r ;
   
   md_dir_data  * dirData = &( mdr -> body.dirData ) ;
   
   for( i = 0 ; i < dirData -> dirHead.entries ; i++ ){
   
      r.head.ID = dirData -> dirItem[i].ID ;
      if( mdIsNullID( r.head.ID  ) )continue ;
      
      if( rc = md2GetRecord( mdl , &r , 0 ) ){

         fprintf(stdout,"%s %s missed ",
              mdStringID(mdr->head.ID),
              md2PrintTypes(mdr->head.type ) ) ;
         fprintf(stdout,"%s %s (%d)\n",
              mdStringID(dirData -> dirItem[i].ID),
              dirData -> dirItem[i].name ,
              rc ) ;
       }
   }
   return ;

}
void checkFileData( MDL * mdl , mdRecord * mdr ){

}
void checkDirHash( MDL * mdl , mdRecord * mdr ){
   int i , rc ;
   mdRecord r ;
   
   md_dir_hash  *dirHash = &(mdr -> body.dirHash) ; 
   
   for( i = 0 ; i < dirHash -> hashHead.entries ; i++ ){
   
      r.head.ID = dirHash -> hashPointer[i] ;
      
      if( mdIsNullID( r.head.ID  ) )continue ;
      
      if( rc = md2GetRecord( mdl , &r , 0 ) ){
         fprintf(stdout,"%s %s missed ",
              mdStringID(mdr->head.ID),
              md2PrintTypes(mdr->head.type ) ) ;
         fprintf(stdout,"%s (%d)\n",
              mdStringID(r.head.ID) ,
              rc
              );
      }
   }
   return ;

}
