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

typedef int (*md_func)( int argc , char *argv[] , MDL *mdl ) ;

typedef struct _func_item {
   char    *name ;
   md_func  func ;
   char    *help ;
} mdFuncItem ;


int md_whatever( int argc , char *argv[] , char *doit ,
                 MDL *mdl , mdFuncItem *list ) ;
int md_dummy( int argc , char *argv[] , MDL *mdl );
int md_getid( int argc , char *argv[] , MDL *mdl );
int md_mkdir( int argc , char *argv[] , MDL *mdl );
int md_mkfile( int argc , char *argv[] , MDL *mdl );
int md_shrink_dir( int argc , char *argv[] , MDL *mdl );
int md_scan( int argc , char *argv[] , MDL *mdl );
int md_read( int argc , char *argv[] , MDL *mdl );
int md_write( int argc , char *argv[] , MDL *mdl );
int md_scan_dir( int argc , char *argv[] , MDL *mdl );
int md_find_id( int argc , char *argv[] , MDL *mdl );
int md_dump_dirs( int argc , char *argv[] , MDL *mdl );
int md_scan_dirs( int argc , char *argv[] , MDL *mdl );
int md_scan_header( int argc , char *argv[] , MDL *mdl );
int md_scan_free( int argc , char *argv[] , MDL *mdl );
int md_ls_l( int argc , char *argv[] , MDL *mdl );
int md_copy( int argc , char *argv[] , MDL *mdl );
int md_mod_links( int argc , char *argv[] , MDL *mdl );
int md_removefromdirhash( int argc , char *argv[] , MDL *mdl );
int md_modify_direntrycount( int argc , char *argv[] , MDL *mdl );

int md_copytopnfs( MDL * mdl , char *name ,  md_id_t id , int level );
int md_copyfrompnfs( MDL * mdl , char *name ,  md_id_t id , int level );

int mayBeId( char *id );

mdFuncItem mdFuncList[] = {

{ "dummy"      , md_dummy         , "<key> <slot>" } ,
{ "mkdir"      , md_mkdir         , "<dirID> <name> <uid> <gid> <perm>" } ,
{ "getid"      , md_getid         , "<path>" } ,
{ "mkfile"     , md_mkfile        , "<dirID> <name> <uid> <gid> <perm>" } ,
{ "ls"         , md_ls_l          , "<dirID>" } ,
{ "find"       , md_find_id       , "<dirID> <searchID>" } ,
{ "shrinkdir"  , md_shrink_dir    , "<dirID>" } ,
{ "scan"       , md_scan          , "" } ,
{ "read"       , md_read          , "<inputFile>" } ,
{ "write"      , md_write         , "<outputFile>" } ,
{ "dumpdir"    , md_dump_dirs     , "" } ,
{ "scandir"    , md_scan_dir      , "" } ,
{ "scandirs"   , md_scan_dirs     , "" } ,
{ "scanheader" , md_scan_header   , "" } ,
{ "scanfree"   , md_scan_free     , "<dirID>" } ,
{ "copy"       , md_copy          , "<fileID> <localPath>|<localPath> <fileID"},
{ "copy"       , md_copy          , "<dirID>" } ,
{ "modlinks"   , md_mod_links     , "<fileID> <linkDiff>" } ,
{ "removefromdirhash"   , md_removefromdirhash     , "<hashID> <toBeRemovedId>" } ,
#ifdef hallo
{ "getroot"    , md_getroot       , "<db> | <db> <newConfigID>" } ,
#endif
{ "modifydirentrycount" , md_modify_direntrycount  , "<dirID> <newDirentryCount" } ,
{ NULL , NULL , NULL } 

} ;

main( int argc ,char *argv[] )
{
 
  int rc  ;
  MDL *mdl ;
  
  mdl = NULL ;
  if( argc < 3 ){
    md_whatever( argc , argv , NULL , NULL , mdFuncList ) ;
    exit(1) ;
  }
  
   rc = 0 ;
   if( ! ( mdl = md2OpenReadWrite( argv[2] , &rc, 0 ) ) ){
       fprintf( stderr," Can't open %s : %d\n" , argv[2] , rc );
       exit(1);
   }
  

   rc = md_whatever( argc , argv , argv[1] , mdl , mdFuncList ) ;
   if( rc == -22 )rc = 1 ;
   else if( rc ){
     fprintf(stderr," Problem %d detected\n",rc);
     rc = 3 ;
   }
   
   if( mdl )md2Close( mdl ) ;
   exit(rc); 

}
int md_whatever(int argc ,char *argv[] ,char *doit ,MDL *mdl ,mdFuncItem *list)
{
   mdFuncItem *cursor ;
   int rc ;
   
   if(  doit == NULL ){
   
      for( cursor = list ;  cursor -> name != NULL  ;  cursor ++ ) 
         fprintf(stderr," USAGE : %s %s <db> %s\n",
                 argv[0],cursor->name,cursor->help      ) ;
      return -22 ;
   
   }
   for( cursor = list ; 
        ( cursor -> name != NULL ) && ( strcmp( cursor->name, doit ) ) ;
       cursor ++ ) ;
 
   if( cursor -> name == NULL )return -23 ;
   
   rc = (*(cursor->func))( argc , argv , mdl ) ; 
   if( rc == -22 ){
   
      fprintf(stderr," USAGE : %s %s <dbName> %s\n",
              argv[0],cursor->name,cursor->help      ) ;
              
   }
   return rc ;
}
int md_dummy( int argc , char *argv[] , MDL *mdl )
{
   int i ;
   for( i = 0 ; i < argc ; i++ )printf( " %d : %s\n",i,argv[i]);
   
   return 0 ;
}
int md_removefromdirhash( int argc , char *argv[] , MDL *mdl )
{
   md_id_t  hashID , removeID ;
   mdRecord r ;
   int rc , i ;

   if( argc < ( 3 + 2 ) )return -22 ;
   
   md2ScanId( argv[3] , &hashID ) ;
   md2ScanId( argv[4] , &removeID ) ;

   r.head.ID = hashID ;
   if( rc = md2GetRecord( mdl , &r , 0 ) ){
       fprintf(stderr," Problem %d reading id %s\n",rc,mdStringID(r.head.ID));
       return rc ;
   }

   for( i = 0 ; i < HASH_POINTERS ; i++ ){

       printf("%s <-> ",mdStringID(r.body.dirHash.hashPointer[i]));
       printf("%s\n",mdStringID(removeID));

       if( ! memcmp( (void*)&r.body.dirHash.hashPointer[i] , (void*)&removeID , sizeof(removeID) ) ){
          memset( (void*)&r.body.dirHash.hashPointer[i] , 0 , sizeof(removeID) )  ;
          if( rc = md2PutRecord( mdl , &r , 0 ) ){
             fprintf(stderr,"Problem %d writing id %s\n",rc,mdStringID(r.head.ID));
          }
          break ;
       }
   }
   if( i == HASH_POINTERS ){
      fprintf(stderr,"ID %s not found in", mdStringID(removeID) );
      fprintf(stderr," %s\n",mdStringID(hashID));
      return 3 ;
   }

   return 0;


}
int md_scan( int argc , char *argv[] , MDL *mdl )
{
   mdRecord r ;
   int rc ;
   struct stat_detail {
     int inode , data , hash ;
   };
   struct stat_struct {
     struct stat_detail directory , file , link , tag ;
     int total ;
   } stat ;
   memset( (char*)&stat , 0 , sizeof( stat ) ) ;
   for( md2GetFirstKey( mdl , &r.head.ID ) ; 
        ! mdIsNullID( r.head.ID ) ;
        md2GetNextKey( mdl , r.head.ID , &r.head.ID ) ){
       
      fprintf(stderr," ID : %s\n",mdStringID(r.head.ID)); 
      if( rc = md2GetRecord( mdl , &r , 0 ) ){
         fprintf(stderr," Problem %d at id %s\n",rc,mdStringID(r.head.ID));
      }else{
         /*
         printf(" %s\n",mdStringID( r.head.ID ) ) ;
         */
           if( mdIsType(r.head.type,mdtDirectory) ){
             if(  mdIsType(r.head.type,mdtInode) ){
               stat.directory.inode ++ ;
             }else if(  mdIsType(r.head.type,mdtData ) ){
               stat.directory.data ++ ;
             }else if(  mdIsType(r.head.type,mdtHash ) ){
               stat.directory.hash ++ ;
             }
           }else if( mdIsType(r.head.type,mdtTag) ){
             if(  mdIsType(r.head.type,mdtInode) ){
               stat.tag.inode ++ ;
             }else if(  mdIsType(r.head.type,mdtData ) ){
               stat.tag.data ++ ;
             }else if(  mdIsType(r.head.type,mdtHash ) ){
               stat.tag.hash ++ ;
             }
           }else if( mdIsType(r.head.type,mdtRegular) ){
             if(  mdIsType(r.head.type,mdtInode) ){
               stat.file.inode ++ ;
             }else if(  mdIsType(r.head.type,mdtData ) ){
               stat.file.data ++ ;
             }else if(  mdIsType(r.head.type,mdtHash ) ){
               stat.file.hash ++ ;
             }
           }else if( mdIsType(r.head.type,mdtLink) ){
             if(  mdIsType(r.head.type,mdtInode) ){
               stat.link.inode ++ ;
             }else if(  mdIsType(r.head.type,mdtData ) ){
               stat.link.data ++ ;
             }else if(  mdIsType(r.head.type,mdtHash ) ){
               stat.link.hash ++ ;
             }
           }
           stat.total ++ ;
      }
   }
      printf( " Total Objects : %d\n",stat.total ) ;
      printf( " Directories\n" ) ;
      printf( "   inode  : %d\n", stat.directory.inode) ;
      printf( "   data   : %d\n", stat.directory.data) ;
      printf( "   hash   : %d\n", stat.directory.hash) ;
      printf( " Files\n" ) ;
      printf( "   inode  : %d\n", stat.file.inode) ;
      printf( "   data   : %d\n", stat.file.data) ;
      printf( "   hash   : %d\n", stat.file.hash) ;
      printf( " Links\n" ) ;
      printf( "   inode  : %d\n", stat.link.inode) ;
      printf( "   data   : %d\n", stat.link.data) ;
      printf( "   hash   : %d\n", stat.link.hash) ;
      printf( " Tags\n" ) ;
      printf( "   inode  : %d\n", stat.tag.inode) ;
      printf( "   data   : %d\n", stat.tag.data) ;
      printf( "   hash   : %d\n", stat.tag.hash) ;
        
   
   
   return 0 ;
}
int md_write( int argc , char *argv[] , MDL *mdl )
{
   mdRecord r ;
   int rc , counter = 0 , size = sizeof( r ) , out  ;


   if( argc < ( 3 + 1 ) )return -22 ; 
   if( ! strcmp( argv[3] , "-" ) ){
      out = 3 ;
   }else{
      out = open( argv[3] , O_WRONLY | O_TRUNC | O_CREAT , 0644 ) ;
      if( out < 0 ){
         fprintf(stderr,"Problem %d opening %s\n",errno,argv[3] ) ;
         return -101 ;
      }
   }
   for( md2GetFirstKey( mdl , &r.head.ID ) ; 
        ! mdIsNullID( r.head.ID ) ;
        md2GetNextKey( mdl , r.head.ID , &r.head.ID ) ){
       
      if( rc = md2GetRecord( mdl , &r , 0 ) ){
         fprintf(stderr," Problem %d at id %s\n",rc,mdStringID(r.head.ID));
         return -101 ;
      }else{
         rc = write( out , (char*)&counter , sizeof( counter ) ) ;
         if( rc != sizeof( counter ) ){
             fprintf(stderr," Problem writing counter %d (%d)\n" , rc , errno ) ;
             return -101 ;
         }
         rc = write( out , (char*)&size , sizeof( size ) ) ;
         if( rc != sizeof( size ) ){
             fprintf(stderr," Problem writing size %d (%d)\n" , rc , errno ) ;
             return -101 ;
         }
         rc = write( out , (char*)&r , sizeof( r ) ) ;
         if( rc != sizeof( r ) ){
             fprintf(stderr," Problem writing record %d (%d)\n" , rc , errno ) ;
             return -101 ;
         }
         counter++ ;
         fprintf(stdout,"%s\n",mdStringID(r.head.ID)); 
      }
   }
   close( out ) ;
   return 0 ;
}
int md_read( int argc , char *argv[] , MDL *mdl )
{
   mdRecord r ;
   int rc , counter , size , isCounter = 0 , in ;
   if( argc < ( 3 + 1 ) )return -22 ;
   if( ! strcmp( argv[3] , "-" ) ){
      in = 0 ;
   }else{
      in = open( argv[3] , O_RDONLY ) ;
      if( in < 0 ){
         fprintf(stderr,"Problem %d opening %s\n",errno,argv[3] ) ;
         return -101 ;
      }
   }
   while(1){
       rc = read( in , (char*)&counter , sizeof( counter ) ) ;
       if( rc != sizeof( counter ) ){
          fprintf(stderr,"Problem reading counter %d (%d)\n",rc , errno ) ;
          rc = -101 ; break ;
       }
       if( counter != isCounter ){
         fprintf(stderr,"Problem : counter mismatch , found %d, should %d\n",
                 counter , isCounter ) ;
          rc = -101 ; break ;
       }
       rc = read( in , (char*)&size , sizeof( size ) ) ;
       if( rc != sizeof( size ) ){
          fprintf(stderr,"Problem reading size %d (%d)\n",rc , errno ) ;
          rc = -101 ; break ;
       }
       if( size != sizeof( r ) ){
          fprintf(stderr,"Problem : Object size mismatch, found %d, should %d\n",
                  size, sizeof( r ) ) ;
          rc = -101 ; break ;
       }
       rc = read( in , (char*)&r , sizeof( r ) ) ;
       if( rc != sizeof( r ) ){
          fprintf(stderr,"Problem reading record %d (%d)\n",rc , errno ) ;
          rc = -101 ; break ;
       }
       if( rc = md2PutRecord( mdl , &r , 0 ) ){
         fprintf(stderr,"Problem %d writing id %s\n",rc,mdStringID(r.head.ID));
          rc = -101 ; break ;
       }
       fprintf(stdout,"%s\n",mdStringID(r.head.ID)); 
       isCounter ++ ;
   }
   close( in ) ;
   return rc ;
}
int md_modify_direntrycount( int argc , char *argv[] , MDL *mdl )
{
   int rc  ;
   int oldDirentryCount = 0 , newDirentryCount = 0 ;
   mdRecord r ;
   md_id_t dirID ;
      
   if( argc < ( 3 + 2 ) )return -22 ;
   
   md2ScanId( argv[3] , &dirID ) ;
   sscanf( argv[4] , "%d" , &newDirentryCount ) ;
   
   r.head.ID = dirID ;
   if( rc = md2GetRecord( mdl , &r , 0 ) ){
       fprintf(stderr," Problem %d reading id %s\n",rc,mdStringID(r.head.ID));
       return -1 ;
   }else if( ( ! mdIsType(r.head.type,mdtDirectory) ) || 
       ( ! mdIsType(r.head.type,mdtInode)     )    ){
       fprintf(stderr," Id %s is not a directory INODE\n",mdStringID(r.head.ID));
       return -1 ;
   }
   
   
   oldDirentryCount = r.body.dirInode.attr.entries ;
   r.body.dirInode.attr.entries = newDirentryCount ;
   
   if( rc = md2PutRecord( mdl , &r , 0 ) ){
       fprintf(stderr," Problem %d reading id %s\n",rc,mdStringID(r.head.ID));
       return -1 ;
   }

   printf("Directory entry count changed from %d to %d\n", 
          oldDirentryCount , newDirentryCount);
   
   return 0 ;  
}
int md_scan_free( int argc , char *argv[] , MDL *mdl )
{
   int rc , i , k , j ;
   int usedFileCounter , unusedBlocksSum, hashPosition ,
       usedBlocksSum ,  blockCounter , unusedBlocks ;
   mdRecord r , hash , data ;
   md_id_t dirID , resId ;
   md_dir_inode *dirInode ;
   md_dir_data  *dirData ;
   md_dir_hash  *dirHash ;
   
   if( argc < ( 3 + 1 ) )return -22 ;
   
   md2ScanId( argv[3] , &dirID ) ;
   
   r.head.ID = dirID ;
   if( rc = md2GetRecord( mdl , &r , 0 ) ){
       fprintf(stderr," Problem %d reading id %s\n",rc,mdStringID(r.head.ID));
       return -1 ;
   }else if( ( ! mdIsType(r.head.type,mdtDirectory) ) || 
       ( ! mdIsType(r.head.type,mdtInode)     )    ){
       fprintf(stderr," Id %s is not a directory INODE\n",mdStringID(r.head.ID));
       return -1 ;
   }
   hashPosition = 0 ;
   unusedBlocksSum = 0 ;
   usedBlocksSum = 0 ;
   dirInode = &r.body.dirInode ;
   for( i = 0 ; i < dirInode -> hashInfo.rows ; i++ ){

       hash.head.ID = dirInode -> hashHandle[i] ;
       if( mdIsNullID( hash.head.ID ) ){
          fprintf(stderr," Hash id ZERO\n");
          continue ;
       }else if( rc = md2GetRecord( mdl , &hash , 0 ) ){
          fprintf(stderr," Problem %d reading hash id %s\n",rc,
                  mdStringID(hash.head.ID));
          continue ;
       }else if( ( ! mdIsType(hash.head.type,mdtDirectory) ) || 
                 ( ! mdIsType(hash.head.type,mdtHash)     )    ){
          fprintf(stderr," Hash Id %s is not a directory hash\n",
                  mdStringID(hash.head.ID));
          continue ;
       }
       dirHash = &hash.body.dirHash ;
       for( j = 0 ; j < dirHash -> hashHead.entries ; j++ ){
           data.head.ID = dirHash -> hashPointer[j] ;
           /*printf( " --- Hash Position : %d\n" , hashPosition ) ;*/
           blockCounter = 0 ;
           unusedBlocks = 0 ;
           usedFileCounter = 0 ;
           while( ! mdIsNullID( data.head.ID ) ){
               if( rc = md2GetRecord( mdl , &data , 0 ) ){
                   fprintf(stderr," Problem %d reading data id %s\n",rc,
                           mdStringID(data.head.ID));
                   break ;
               }else if( ( ! mdIsType(data.head.type,mdtDirectory) ) || 
                         ( ! mdIsType(data.head.type,mdtData)     )    ){
                   fprintf(stderr," Data Id %s is not a directory data\n",
                           mdStringID(data.head.ID));
                   break ;
               }
               dirData = &data.body.dirData ;
               
               for( k = 0 ; k < dirData -> dirHead.maxEntries ; k++ ){
                   /*
                   printf( "%s : %s\n" , 
                           mdStringID(dirData -> dirItem[k].ID),
                           dirData -> dirItem[k].name ) ;
                   */
                   if( mdIsNullID( dirData -> dirItem[k].ID ) )continue ;
                   usedFileCounter ++ ;
               }
               if( dirData -> dirHead.entries == 0 )unusedBlocks ++ ;
               blockCounter ++ ;
               data.head.ID = data.head.nextID ;

           }
           printf( " hash %3d  files %5d blocks %d unusedblocks %d\n" ,
                   hashPosition , usedFileCounter ,blockCounter , unusedBlocks ) ;
           unusedBlocksSum += unusedBlocks ;
           usedBlocksSum += blockCounter ;
           hashPosition ++ ;
       }
   }
   printf( " UnusedBlocksSum %d\n" , unusedBlocksSum ) ;
   printf( "   UsedBlocksSum %d\n" , usedBlocksSum ) ;
   return 0 ;
}
int md_shrink_dir( int argc , char *argv[] , MDL *mdl )
{
   int rc , i , k , j , wasModified , hashModified ;
   int  unusedBlocksSum, hashPosition , unusedBlocks ;
   mdRecord r , hash , data ,lastData ;
   md_id_t dirID , resId , *idPtr ;
   md_dir_inode *dirInode ;
   md_dir_data  *dirData ;
   md_dir_hash  *dirHash ;
   
   if( argc < ( 3 + 1 ) )return -22 ;
   
   md2ScanId( argv[3] , &dirID ) ;
   
   r.head.ID = dirID ;
   if( rc = md2GetRecord( mdl , &r , 0 ) ){
       fprintf(stderr," Problem %d reading id %s\n",rc,mdStringID(r.head.ID));
       return -1 ;
   }else if( ( ! mdIsType(r.head.type,mdtDirectory) ) || 
       ( ! mdIsType(r.head.type,mdtInode)     )    ){
       fprintf(stderr," Id %s is not a directory INODE\n",mdStringID(r.head.ID));
       return -1 ;
   }
   hashPosition = 0 ;
   unusedBlocksSum = 0 ;
   dirInode = &r.body.dirInode ;
   for( i = 0 ; i < dirInode -> hashInfo.rows ; i++ ){

       hash.head.ID = dirInode -> hashHandle[i] ;
       if( mdIsNullID( hash.head.ID ) ){
          fprintf(stderr," Hash id ZERO\n");
          return -100 ;
       }else if( rc = md2GetRecord( mdl , &hash , 0 ) ){
          fprintf(stderr," Problem %d reading hash id %s\n",rc,
                  mdStringID(hash.head.ID));
          return -100 ;
       }else if( ( ! mdIsType(hash.head.type,mdtDirectory) ) || 
                 ( ! mdIsType(hash.head.type,mdtHash)     )    ){
          fprintf(stderr," Hash Id %s is not a directory hash\n",
                  mdStringID(hash.head.ID));
          return -100 ;
       }
       hashModified = 0 ;
       dirHash      = &hash.body.dirHash ;
       for( j = 0 ; j < dirHash -> hashHead.entries ; j++ ){
           
           unusedBlocks = 0 ;
           idPtr        = &dirHash -> hashPointer[j] ;
           wasModified  = 0 ;
           mdSetNullID( lastData.head.ID ) ;
           
           while( ! mdIsNullID( *idPtr ) ){

               data.head.ID = *idPtr ;
               
               if( rc = md2GetRecord( mdl , &data , 0 ) ){
                   fprintf(stderr," Problem %d reading data id %s\n",rc,
                           mdStringID(data.head.ID));
                   return -100 ;
               }else if( ( ! mdIsType(data.head.type,mdtDirectory) ) || 
                         ( ! mdIsType(data.head.type,mdtData)     )    ){
                   fprintf(stderr," Data Id %s is not a directory data\n",
                           mdStringID(data.head.ID));
                   return -100 ;
               }
               
               dirData = &data.body.dirData ;
               
               if( dirData -> dirHead.entries == 0 ){
                     if( rc = md2DeleteRecord( mdl , *idPtr , 0 ) ){
                         fprintf(stderr," md2DeleteRecord %s failed\n",
                                 mdStringID(lastData.head.ID));
                         return -100 ;
                     }
                     unusedBlocks ++ ;
                     *idPtr      = data.head.nextID ;
                     wasModified = 1 ;
               }else{
                  if( wasModified ){ 
                     if(  ! mdIsNullID( lastData.head.ID  ) ){
                        if( rc = md2PutRecord( mdl , &lastData , 0 ) ){
                            fprintf(stderr," md2PutRecord %s failed\n",
                                    mdStringID(lastData.head.ID));
                            return -100 ;
                        }
                     }else{
                        hashModified = 1 ;
                     }
                     wasModified = 0 ;
                  } 
                  memcpy((void*)&lastData,(void*)&data,sizeof(data));
                  idPtr = &lastData.head.nextID ;
               }
               
               data.head.ID = data.head.nextID ;

           }
           if( wasModified ){ 
              if(  ! mdIsNullID( lastData.head.ID  ) ){
                   if( rc = md2PutRecord( mdl , &lastData , 0 ) ){
                            fprintf(stderr," md2PutRecord %s failed\n",
                                    mdStringID(lastData.head.ID));
                            return -100 ;
                   }
               }else{
                   hashModified = 1 ;
               }
           } 
           printf( " hash %3d  %d blocks removed \n" , hashPosition ,unusedBlocks ) ;
           unusedBlocksSum += unusedBlocks ;
           hashPosition ++ ;
       }
       if( hashModified && ( rc = md2PutRecord( mdl , &hash , 0 ) ) ){
              fprintf(stderr," md2PutRecord %s failed\n", mdStringID(hash.head.ID));
              return -100 ;
       }
   }
   return 0 ;
}
int md_scan_header( int argc , char *argv[] , MDL *mdl )
{
   mdRecord r ;
   int rc , counter , ourDbId ;
   counter = 0 ;
   mdSetRootID( r.head.ID ) ;
   if( rc = md2GetRecord( mdl , &r , 0 ) ){
      fprintf(stderr," Problem %d at id %s\n",rc,mdStringID(r.head.ID));
      return -1 ;
   }
   ourDbId = r.body.root.DB ;
   for( md2GetFirstKey( mdl , &r.head.ID ) ; 
        ! mdIsNullID( r.head.ID ) ;
        md2GetNextKey( mdl , r.head.ID , &r.head.ID ) ){
       
      if( rc = md2GetRecord( mdl , &r , 0 ) ){
         fprintf(stderr," Problem %d at id %s\n",rc,mdStringID(r.head.ID));
      }else{
         /*
         printf(" %s\n",mdStringID( r.head.ID ) ) ;
         */
         if( ( r.head.baseID.db > 40 ) ||
             ( r.head.backID.db > 40 ) ||
             ( r.head.parentID.db > 40 ) ||
             ( r.head.nextID.db > 40 )      )
             fprintf( stdout , " Inconsistent record : %s\n" , mdStringID(r.head.ID) ) ;
         counter ++ ;
      }
   }
   printf( " Scanned DB id : %d ; %d records found\n",ourDbId, counter ) ;
   
   return 0 ;
}
int md_dump_dirs( int argc , char *argv[] , MDL *mdl )
{
   mdRecord r ;
   int rc , counter , ourDbId ;
   counter = 0 ;
   mdSetRootID( r.head.ID ) ;
   if( rc = md2GetRecord( mdl , &r , 0 ) ){
      fprintf(stderr," Problem %d at id %s\n",rc,mdStringID(r.head.ID));
      return -1 ;
   }
   ourDbId = r.body.root.DB ;
   for( md2GetFirstKey( mdl , &r.head.ID ) ; 
        ! mdIsNullID( r.head.ID ) ;
        md2GetNextKey( mdl , r.head.ID , &r.head.ID ) ){
       
      if( rc = md2GetRecord( mdl , &r , 0 ) ){
         fprintf(stderr," Problem %d at id %s\n",rc,mdStringID(r.head.ID));
      }else{
         /*
         */
         if( mdIsType(r.head.type,mdtDirectory) &&
             mdIsType(r.head.type,mdtInode)        ){
                printf("%s\n",mdStringID( r.head.ID ) ) ;             
         }
      }
   }
   
   return 0 ;
}
int md_scan_dir( int argc , char *argv[] , MDL *mdl )
{
   mdRecord r ;
   int ourDbId , i ;
   md_dir_inode *dirInode ;
   md_dir_data  *dirData ;
   md_dir_hash  *dirHash ;
   
   int rc ;
   /*
    * get the root record to learn about our database id.
    */
   mdSetRootID( r.head.ID ) ;
   if( rc = md2GetRecord( mdl , &r , 0 ) ){
      fprintf(stderr," Problem %d at id %s\n",rc,mdStringID(r.head.ID));
      return -1 ;
   }
   ourDbId = r.body.root.DB ;
   printf( " Scanning DB id : %d\n",ourDbId ) ;
   
   for( md2GetFirstKey( mdl , &r.head.ID ) ; 
        ! mdIsNullID( r.head.ID ) ;
        md2GetNextKey( mdl , r.head.ID , &r.head.ID ) ){
       
/*      fprintf(stderr," ID : %s\n",mdStringID(r.head.ID)); */
      if( rc = md2GetRecord( mdl , &r , 0 ) ){
         fprintf(stderr," Problem %d at id %s\n",rc,mdStringID(r.head.ID));
      }else{
         if( mdIsType(r.head.type,mdtDirectory) ){
           if(  mdIsType(r.head.type,mdtInode) ){
              printf("Scanning Directory Inode %s\n",mdStringID( r.head.ID ) ) ;
              dirInode = &r.body.dirInode ;
              for( i = 0 ; i < dirInode -> hashInfo.rows ; i++ ){
                 if( dirInode -> hashHandle[i].db != ourDbId )
                    fprintf( stdout ,"   External Reference at %d : %s\n",
                             i ,  mdStringID(dirInode -> hashHandle[i]) ) ;
                 if( dirInode -> hashHandle[i].db > 40 )
                    fprintf( stdout ,"  XExternal Reference at %d : %s\n",
                             i ,  mdStringID(dirInode -> hashHandle[i]) ) ;
              }
           }else if(  mdIsType(r.head.type,mdtData ) ){
              printf("Scanning Directory Data  %s\n",mdStringID( r.head.ID ) ) ;
              dirData  = &r.body.dirData ;
              for( i = 0 ; i < dirData -> dirHead.entries ; i++ ){
                if( dirData -> dirItem[i].ID.db != ourDbId )
                  fprintf( stdout ,"   External Reference at %d : %s for %s\n",i,
                    mdStringID(dirData -> dirItem[i].ID),
                    dirData -> dirItem[i].name  ) ;
                if( dirData -> dirItem[i].ID.db > 40 )
                  fprintf( stdout ,"  XExternal Reference at %d : %s for %s\n",i,
                    mdStringID(dirData -> dirItem[i].ID),
                    dirData -> dirItem[i].name  ) ;
              }
           }else if(  mdIsType(r.head.type,mdtHash ) ){
              printf("Scanning Directory Hash  %s\n",mdStringID( r.head.ID ) ) ;
              dirHash  = &r.body.dirHash ;
              for( i = 0 ; i < dirHash -> hashHead.entries ; i++ ){
                 if(( ! mdIsNullID( dirHash -> hashPointer[i] ) ) &&
                    ( dirHash -> hashPointer[i].db != ourDbId )   )
                 fprintf( stdout,"   External Reference at %d : %s\n",
                       i,mdStringID(dirHash -> hashPointer[i]) ) ;
                 if( dirHash -> hashPointer[i].db > 40 )
                 fprintf( stdout,"  XExternal Reference at %d : %s\n",
                       i,mdStringID(dirHash -> hashPointer[i]) ) ;
              }
           }
         }
      }
   }
   
   return 0 ;
}
int md_mod_links( int argc , char *argv[] , MDL *mdl )
{
   md_id_t fileID  ;
   int diff , rc ;
   
   if( argc < ( 3 + 2 ) )return -22 ;
   
   md2ScanId( argv[3] , &fileID ) ;
   sscanf( argv[4] , "%d" , &diff ) ;
   
   rc = md2ModifyLinkCount( mdl , fileID , &diff ) ;
   if( rc )return rc ;
   printf( "New nlink of %s is %d\n" , mdStringID(fileID) , diff ) ;
   
   return 0 ;
}
int md_scan_dirs( int argc , char *argv[] , MDL *mdl )
{
   mdRecord r ;
   int ourDbId , i ;
   md_dir_inode *dirInode ;
   md_dir_data  *dirData ;
   md_dir_hash  *dirHash ;
   
   int rc ;
   /*
    * get the root record to learn about our database id.
    */
   mdSetRootID( r.head.ID ) ;
   if( rc = md2GetRecord( mdl , &r , 0 ) ){
      fprintf(stderr," Problem %d at id %s\n",rc,mdStringID(r.head.ID));
      return -1 ;
   }
   ourDbId = r.body.root.DB ;
   printf( " Scanning DB id : %d\n",ourDbId ) ;
   
   for( md2GetFirstKey( mdl , &r.head.ID ) ; 
        ! mdIsNullID( r.head.ID ) ;
        md2GetNextKey( mdl , r.head.ID , &r.head.ID ) ){
       
/*      fprintf(stderr," ID : %s\n",mdStringID(r.head.ID)); */
      if( rc = md2GetRecord( mdl , &r , 0 ) ){
         fprintf(stderr,"Problem %d at id %s\n",rc,mdStringID(r.head.ID));
      }else{
         if( mdIsType(r.head.type,mdtDirectory) && mdIsType(r.head.type,mdtInode) ){
            /*printf("Scanning Directory Inode %s\n",mdStringID( r.head.ID ) ) ;*/
            do_the_dir_scan( &r , mdl ) ;
         }
      }
   }
   
   return 0 ;
}
int do_the_dir_scan( mdRecord *r , MDL *mdl  ){ 
   int rc , i , k , j ;
   int usedFileCounter , unusedBlocksSum, hashPosition ,
       usedBlocksSum ,  blockCounter , unusedBlocks ;
   mdRecord hash , data ;
   md_id_t dirID , resId ;
   md_dir_inode *dirInode ;
   md_dir_data  *dirData ;
   md_dir_hash  *dirHash ;
   
   hashPosition    = 0 ;
   unusedBlocksSum = 0 ;
   usedBlocksSum   = 0 ;
   dirInode = &r -> body.dirInode ;
   for( i = 0 ; i < dirInode -> hashInfo.rows ; i++ ){

       hash.head.ID = dirInode -> hashHandle[i] ;
       if( mdIsNullID( hash.head.ID ) ){
          fprintf(stderr," Hash id ZERO\n");
          continue ;
       }else if( rc = md2GetRecord( mdl , &hash , 0 ) ){
          fprintf(stderr," Problem %d reading hash id %s\n",rc,
                  mdStringID(hash.head.ID));
          continue ;
       }else if( ( ! mdIsType(hash.head.type,mdtDirectory) ) || 
                 ( ! mdIsType(hash.head.type,mdtHash)     )    ){
          fprintf(stderr," Hash Id %s is not a directory hash\n",
                  mdStringID(hash.head.ID));
          continue ;
       }
       dirHash = &hash.body.dirHash ;
       for( j = 0 ; j < dirHash -> hashHead.entries ; j++ ){
           data.head.ID = dirHash -> hashPointer[j] ;
           /*printf( " --- Hash Position : %d\n" , hashPosition ) ;*/
           blockCounter    = 0 ;
           unusedBlocks    = 0 ;
           usedFileCounter = 0 ;
           while( ! mdIsNullID( data.head.ID ) ){
               if( rc = md2GetRecord( mdl , &data , 0 ) ){
                   fprintf(stderr," Problem %d reading data id %s\n",rc,
                           mdStringID(data.head.ID));
                   break ;
               }else if( ( ! mdIsType(data.head.type,mdtDirectory) ) || 
                         ( ! mdIsType(data.head.type,mdtData)     )    ){
                   fprintf(stderr," Data Id %s is not a directory data\n",
                           mdStringID(data.head.ID));
                   break ;
               }
               dirData = &data.body.dirData ;
#ifdef detail
               for( k = 0 ; k < dirData -> dirHead.maxEntries ; k++ ){
                   /*
                   printf( "%s : %s\n" , 
                           mdStringID(dirData -> dirItem[k].ID),
                           dirData -> dirItem[k].name ) ;
                   */
                   if( mdIsNullID( dirData -> dirItem[k].ID ) )continue ;
                   usedFileCounter ++ ;
               }
#endif               
               if( dirData -> dirHead.entries == 0 )unusedBlocks ++ ;
               blockCounter ++ ;
               data.head.ID = data.head.nextID ;

           }
           /*
           printf( " hash %3d  files %5d blocks %d unusedblocks %d\n" ,
                   hashPosition , usedFileCounter ,blockCounter , unusedBlocks ) ;
          */
           unusedBlocksSum += unusedBlocks ;
           usedBlocksSum   += blockCounter ;
           hashPosition ++ ;
       }
   }
   printf("Directory Inode %s ; blocks %4d used %4d unused %4d\n",
          mdStringID( r->head.ID ) ,
          usedBlocksSum,usedBlocksSum-unusedBlocksSum,unusedBlocksSum  ) ;
   

}
int do_get_parent( md_id_t dirID , md_id_t *parentID , MDL * mdl ){
   mdRecord r ; 
   int rc ;
   r.head.ID = dirID ;
   if( rc = md2GetRecord( mdl , &r , 0 ) ){
       fprintf(stderr," Problem %d reading id %s\n",rc,mdStringID(r.head.ID));
       return -1 ;
   }
   *parentID = r.head.parentID ;
   return 0 ;

}
int do_find_id( md_id_t dirID , md_id_t id ,
                md_id_t *parentID ,  char * out ,  MDL *mdl  ){
   mdRecord r ; 
   int rc , i , k , j ;
   mdRecord hash , data ;
   md_id_t resId ;
   md_dir_inode *dirInode ;
   md_dir_data  *dirData ;
   md_dir_hash  *dirHash ;
   
   r.head.ID = dirID ;
   if( rc = md2GetRecord( mdl , &r , 0 ) ){
       fprintf(stderr," Problem %d reading id %s\n",rc,mdStringID(r.head.ID));
       return -1 ;
   }else if( ( ! mdIsType(r.head.type,mdtDirectory) ) || 
             ( ! mdIsType(r.head.type,mdtInode)     )    ){
       fprintf(stderr," Id %s is not a directory INODE\n",mdStringID(r.head.ID));
       return -1 ;
   }
   dirInode = &r.body.dirInode ;
   for( i = 0 ; i < dirInode -> hashInfo.rows ; i++ ){

       hash.head.ID = dirInode -> hashHandle[i] ;
       if( mdIsNullID( hash.head.ID ) ){
          fprintf(stderr," Hash id ZERO\n");
          continue ;
       }else if( rc = md2GetRecord( mdl , &hash , 0 ) ){
          fprintf(stderr," Problem %d reading hash id %s\n",rc,
                  mdStringID(hash.head.ID));
          continue ;
       }else if( ( ! mdIsType(hash.head.type,mdtDirectory) ) || 
                 ( ! mdIsType(hash.head.type,mdtHash)     )    ){
          fprintf(stderr," Hash Id %s is not a directory hash\n",
                  mdStringID(hash.head.ID));
          continue ;
       }
       dirHash = &hash.body.dirHash ;
       for( j = 0 ; j < dirHash -> hashHead.entries ; j++ ){
           data.head.ID = dirHash -> hashPointer[j] ;
           /*printf( " --- Hash Position : %d\n" , hashPosition ) ;*/
           while( ! mdIsNullID( data.head.ID ) ){
               if( rc = md2GetRecord( mdl , &data , 0 ) ){
                   fprintf(stderr," Problem %d reading data id %s\n",rc,
                           mdStringID(data.head.ID));
                   break ;
               }else if( ( ! mdIsType(data.head.type,mdtDirectory) ) || 
                         ( ! mdIsType(data.head.type,mdtData)     )    ){
                   fprintf(stderr," Data Id %s is not a directory data\n",
                           mdStringID(data.head.ID));
                   break ;
               }
               dirData = &data.body.dirData ;
               for( k = 0 ; k < dirData -> dirHead.maxEntries ; k++ ){
                   /*
                   printf( "%s : %s\n" , 
                           mdStringID(dirData -> dirItem[k].ID),
                           dirData -> dirItem[k].name ) ;
                   */
                   if( mdIsNullID( dirData -> dirItem[k].ID ) )continue ;
                   if( mdIsEqualID( dirData -> dirItem[k].ID , id ) ){
                       strcpy( out ,dirData -> dirItem[k].name ) ;
                       *parentID = r.head.parentID ;
                       return 1 ; 
                   }
               }
               data.head.ID = data.head.nextID ;

           }
       }
   }
   return 0 ;
}
int md_find_id( int argc , char *argv[] , MDL *mdl )
{
   int rc ;
   md_id_t dirID , id , parentID ;
   char filename[1024] ;
  
   
   if( argc < ( 3 + 1 ) )return -22 ;
   
   md2ScanId( argv[3] , &dirID ) ;
   
   if( argc == ( 3 + 1 ) ){
     rc = do_get_parent( dirID , &parentID , mdl );
     if( rc < 0 ){
        fprintf( stderr," parent not found for %s\n",
                 mdStringID(dirID) ) ;
        return rc ;
     }
     printf( "%s" , mdStringID(parentID) ) ;
     printf( " %s\n" , mdStringID(dirID) ) ;
     
   }else{
   
      md2ScanId( argv[4] , &id ) ;
 
      rc = do_find_id( dirID , id , &parentID , filename ,  mdl  ) ; 
      if( rc == 0 ){
         fprintf(stderr," %s not found in %s\n" ,
                 mdStringID(id),mdStringID(dirID) ) ;
         return -100 ;
      }else if( rc < 0 ){
         fprintf(stderr," Problem %d while searchind %s in %s\n" ,rc,
                 mdStringID(id),mdStringID(dirID) ) ;
         return -100 ;
      }
      printf( "%s " , mdStringID(parentID) );
      printf( "%s " , mdStringID(dirID) );
      printf( "%s " , mdStringID(id) );
      printf( "%s\n" , filename ) ;
      
   }
   return 0;
}
int md_mkdir( int argc , char *argv[] , MDL *mdl )
{
   int rc ;
   md_id_t dirID , resId ;
   char *name ;
   md_unix attr ;
   
   if( argc < ( 3 + 5 ) )return -22 ;
   
   md2ScanId( argv[3] , &dirID ) ;
   name = argv[4] ;
   sscanf(argv[5] , "%d" , &attr.mst_uid ) ;
   sscanf(argv[6] , "%d" , &attr.mst_gid ) ;
   sscanf(argv[7] , "%ho" , &attr.mst_mode ) ;
   attr.mst_atime = md_no_time ;
   attr.mst_mtime = md_no_time ;
   attr.mst_ctime = md_no_time ;
   attr.mst_size  = md_no_size ;
   
   rc = md2Lookup( mdl , dirID , name , &resId ) ;
   if( ! rc ){ 
     fprintf(stderr," File exists\n");
     return MDEexists ;
   }
   rc = md2MakeDirectory( mdl , dirID , name , &resId );
   if( rc ){
     fprintf(stderr," md2MakeDirectory %d\n",rc);
     return rc ;
   }
   rc = md2ModifyUnixAttr( mdl , resId , 0 | mdpAllLevels , &attr );
   if( rc ){
     fprintf(stderr," md2ModifyUnixAttr %d\n",rc);
     return rc ;
   }
   printf( "%s %s -> %s\n",argv[3],name,mdStringID( resId ) );
   return 0 ;
}
int md_ls_l( int argc , char *argv[] , MDL *mdl )
{
  int rc ;
  md_id_t dirID ;
  
   if( argc < ( 3 + 1 ) )return -22 ;
   
   md2ScanId( argv[3] , &dirID ) ;
  
   if( rc = md2PrintDirectory( mdl , dirID , 1,stdout ) ){
     fprintf(stderr," md2PrintDirectory %d\n",rc);
     return rc ;
   }
   return 0 ;

}
int md_getid( int argc , char *argv[] , MDL *mdl )
{
  int rc ;
   md_id_t resId ;
   char *name ;
    
   if( argc < ( 3 + 1 ) )return -22 ;
   if( ! strncmp( argv[3] , "/0/" , 3 ) ){
      name = argv[3] + 2 ;
   }else{
      name = argv[3] ;
   }
   rc = md2ObjectToId( mdl , resId , name , &resId ) ;
   if( rc < 0 ){
     fprintf( stderr," md2ObjectToId : %d\n",rc);
     return rc ;
   }
   printf( "%s %s\n",mdStringID(resId),argv[3]);
   return 0 ;     
}
int md_mkfile( int argc , char *argv[] , MDL *mdl )
{
  int rc ;
   md_id_t dirID , resId ;
   char *name ;
   md_unix attr ;
   md_permission perm ;
   
   if( argc < ( 3 + 5 ) )return -22 ;
   
   md2ScanId( argv[3] , &dirID ) ;
   name = argv[4] ;
   sscanf(argv[5] , "%d" , &attr.mst_uid ) ;
   sscanf(argv[6] , "%d" , &attr.mst_gid ) ;
   sscanf(argv[7] , "%ho" , &attr.mst_mode ) ;
   attr.mst_atime = md_no_time ;
   attr.mst_mtime = md_no_time ;
   attr.mst_ctime = md_no_time ;
   attr.mst_size  = md_no_size ;
   
   rc = md2Lookup( mdl , dirID , name , &resId ) ;
   if( ! rc ){ 
     fprintf(stderr," File exists\n");
     return MDEexists ;
   }
   memset((char*)&perm,0,sizeof(md_permission));
   rc = md2MakeFilePerm( mdl , dirID , perm, name , &resId , NULL ) ;
   if( rc ){
     fprintf(stderr," md2MakeDirectory %d\n",rc);
     return rc ;
   }
   rc = md2ModifyUnixAttr( mdl , resId , 0 | mdpAllLevels , &attr );
   if( rc ){
     fprintf(stderr," md2ModifyUnixAttr %d\n",rc);
     return rc ;
   }
   printf( "%s %s -> %s\n",argv[3],name,mdStringID( resId ) );
   return 0 ;
}
int md_copy( int argc , char *argv[] , MDL *mdl )
{
  int rc , f2 , f1 ;
   md_id_t id ;
   
   if( argc < ( 3 + 2 ) )return -22 ;
   f1 = mayBeId( argv[3] ) ;
   f2 = mayBeId( argv[4] ) ;
   if( ( f1 && f2 ) || ! ( f1 || f2 ) )return -22 ; 
   
   if( f1 ){
      md2ScanId( argv[3] , &id ) ;
      rc = md_copyfrompnfs( mdl , argv[4] , id , 0 ) ;      
   }else{
      md2ScanId( argv[4] , &id ) ;
      rc = md_copytopnfs( mdl , argv[3] , id , 0 ) ;  
   }  
   return rc ;
}
static char buffer[1024] ;

int md_copytopnfs( MDL * mdl , char *name ,  md_id_t id , int level )
{
  int rc , fh , rc2 ;
  md_fs_block p ;
  
  if( rc = mdFs_Open( mdl , &p , id , level , "w" )  ){
     fprintf(stderr," mdFs_Open (write) failed : %d\n",rc );
     return rc ;
  }
  if( ( fh = open( name , O_RDONLY ) ) < 0 ){
     fprintf(stderr," Can't open %s : %d\n" , name , errno ) ;
     return -200 ;
  }
  while( ( rc = read( fh , buffer , 1024 ) ) > 0 ){	
    if( ( rc2 = mdFs_Write( &p , buffer , rc ) ) < 0 )break ;  
  }
  if(rc<0){
    fprintf(stderr," Read failed from local fs : %d\n",errno );
  }else if( rc > 0 ){
    fprintf(stderr," Write failed to pnfs : %d\n",rc );
  }
  close(fh);
  return 0 ;
}
int md_copyfrompnfs( MDL * mdl , char *name ,  md_id_t id , int level )
{
  int rc , rc2 , fh;
  md_fs_block p ;
  
  if( rc = mdFs_Open( mdl , &p , id , level , "r" )  ){
     fprintf(stderr," mdFs_Open (read) failed : %d\n",rc );
     return rc ;
  }
  if( ( fh = open( name , O_WRONLY | O_CREAT | O_TRUNC  , 0600 ) ) < 0 ){
     fprintf(stderr," Can't open %s : %d\n" , name , errno ) ;
     return -200 ;
  }
  while( ( rc = mdFs_Read( &p , buffer , 1024 ) ) > 0 ){	
    if( ( rc2 = write( fh , buffer , rc ) ) < 0 )break ;  
  }

  if(rc<0){
    fprintf(stderr," Read failed from pnfs : %d\n",rc );
  }else if( rc > 0 ){
    fprintf(stderr," Write failed to local fs : %d\n",errno );
  }
  close(fh);
  return 0 ;
}

int mayBeId( char *id )
{
 char *x ;
 
  for(x=id;(*x!='\0')&&isxdigit(*x);x++);
  return *x=='\0'?1:0;
}
