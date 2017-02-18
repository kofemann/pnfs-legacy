#include <stdio.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include <dirent.h>

static int checkDirectoryOk( const char * message ,char * dir ) ;
static char * getIdByPath( const char * path ) ;
static int processDirectory( const char * sourcePath , 
                             const char * destinationPath ,
                             const char * content ) ;
                             
static int  createDirectory( const char * path , const struct stat * stat );
static int  createFile( const char * path , struct stat * stat , const char * content , char ** hsm);
static char *  concatDirectories( const char * dir , const char * base  ) ;
static char ** splitPath( const char * path ) ;                            
static void freeSplitPath( char ** split ) ;
char * stat2string( const struct stat * stat ) ;
char ** getStorageInfo( const char * path ) ;
void freeStorageInfo( char ** hsm ) ;

int main( int argc , char * argv[] ){

   char * sourceBase , * sourceRoot , * destinationPath, * sourcePath ;
   
   if( argc < 3 ){
      fprintf(stderr,"Usage : %s <sourceBase> <sourcePath> <destintationBase>\n",
              argv[0] ) ;
      return 3 ;
   } 

   sourceBase = argv[1] ;
   sourceRoot = argv[2] ;
   sourcePath = (char *)malloc( strlen(argv[1]) + strlen(argv[2]) + 2 ) ;
   strcpy(sourcePath , sourceBase ) ;
   strcat(sourcePath , "/" ) ;
   strcat(sourcePath , sourceRoot ) ;
   destinationPath = argv[3] ;

   /* check if valid */
   
   if( checkDirectoryOk( "Problem with sourceBase ",sourceBase ) )return 1 ;
   if( checkDirectoryOk( "Problem with sourcePath ",sourcePath ) )return 1 ;
   if( checkDirectoryOk( "Problem with destinationPath ",destinationPath ) )return 1 ;

   return processDirectory( sourcePath , destinationPath , sourceRoot ) ;
   

}
int processDirectory( const char * sourcePath , 
                      const char * destinationPath ,
                      const char * contentPath ){
    struct dirent * entry ;    
    char * name ,* source , * destination , * content ;
    char ** storageInfo = NULL ;
    struct stat value ;
    int res = 0 ;      
            
    DIR * directory = opendir( sourcePath ) ;
    if( directory == NULL ){
        fprintf(stderr,"Problem reading directory %s : ",sourcePath) ;
        perror(NULL);
        return 1;
    }
    
    if( ( storageInfo = getStorageInfo( destinationPath ) ) == NULL ){
       fprintf(stderr,"PANIC : can't get storage info of : %s\n",sourcePath) ;
       goto bad ;
    }
    
    while( ( entry = readdir( directory ) ) != NULL ){
    
        name = entry->d_name ;
        
        if( ! strcmp( name , "." ) )continue ;
        if( ! strcmp( name , ".." ) ) continue ;
        
        source      = concatDirectories( sourcePath      , name ) ;
        destination = concatDirectories( destinationPath , name ) ;
        content     = concatDirectories( contentPath     , name ) ;
        
        if( res = lstat( source , &value ) ){
            perror("lstat");
            fprintf(stderr,"Can't get info about : %s at %s\n",name,sourcePath);
            goto bad; ;
        }

        if( S_ISDIR( value.st_mode ) ){
            printf("Directory : %s\n",source);
            if( ! access( destination , F_OK ) ){
                fprintf(stderr,"WARNING : already exists : %s\n",destination);
            }else{
                if( createDirectory( destination , &value  ) ){
                    fprintf(stderr,"PANIC : couldn't create directory : %s\n",destination);
                }
            }
            processDirectory( source , destination , content ) ;
            
        }else if( S_ISREG( value.st_mode ) ){
            printf("File : %s\n",source);
            if( ! access( destination , F_OK ) ){
                fprintf(stderr,"WARNING : already exists : %s\n",destination);
            }else{
                if( createFile( destination , &value , content , storageInfo ) ){
                    fprintf(stderr,"PANIC : couldn't create file : %s\n",destination);
                }
            }
        }else{
            fprintf(stderr,"Warning : not regular : %s\n",source);
        }
        free((void*)source);
        free((void*)destination);
        free((void*)content);
    }
    freeStorageInfo(storageInfo);
    closedir(directory) ;
    return 0 ; 
    
    bad :
        if( storageInfo != NULL )freeStorageInfo(storageInfo);
        closedir(directory) ;
        free((void*)source);
        free((void*)destination);
        free((void*)content);
        return 1;      
}
char ** getStorageInfo( const char * path ){

   char ** hsm = (char **)malloc( 2 * sizeof( char * ) ) ;
   char * virtualPath = (char *)malloc( strlen(path) + 128 ) ;
   char * tmp , store[128] ;
   FILE * stream ;
   int i , len ;
   
   sprintf(virtualPath,"%s/.(tag)(sGroup)",path);
   stream  = fopen( virtualPath , "r" ) ;
   if( stream == NULL ){
      fprintf(stderr,"PANIC : not sGroup found in %s\n",path);
      goto bad ;
   }
   hsm[0] = (char *)malloc(128) ;
   tmp = fgets(hsm[0],128,stream) ;
   fclose(stream);
   
   if( ( tmp == NULL ) || ( (len = strlen(tmp) ) == 0 ) )goto bad1 ;
   if( tmp[len-1] == '\n' )tmp[len-1] = '\0';
   
   sprintf(virtualPath,"%s/.(tag)(OSMTemplate)",path);
   stream  = fopen( virtualPath , "r" ) ;
   if( stream == NULL ){
      fprintf(stderr,"PANIC : not sGroup found in %s\n",path);
      goto bad ;
   }
   hsm[1] = (char *)malloc(128) ;
   tmp = fgets(store,128,stream) ;
   fclose(stream);
   
   if( ( tmp == NULL ) || ( (len = strlen(tmp) ) == 0 ) )goto bad1 ;
   if( tmp[len-1] == '\n' )tmp[len-1] = '\0';
   len = strlen(tmp);
   if( len == 0 )goto bad1 ;
   for(i=0;(i<len)&&(tmp[i]!=' ');i++);
   if( i == len )goto bad1; 
   for(;(i<len)&&(tmp[i]==' ');i++);
   if( i == len )goto bad1; 
   hsm[1] = strdup(tmp+i);
   
   free((void*)virtualPath);
   return hsm ;
   bad1 :
      free( (void*)hsm[0] ) ;
   bad :
      free((void*)virtualPath);
      free((void*)hsm);
      return NULL ;
}
void freeStorageInfo( char ** hsm ){
   if( hsm == NULL )return ;
   if( hsm[0] != NULL )free( (void *)hsm[0] ) ;
   if( hsm[1] != NULL )free( (void *)hsm[1] ) ;
   free( (void *)hsm );
   return ;
}
char ** splitPath( const char * path ){

   char **result = (char **) malloc( 2 * sizeof( char * ) ) ;
   
   char * pos = strrchr( path , '/' ) ;
   int len ; 
   if( path == NULL )goto bad ;
   
   len = strlen( path ) ;
   
   if( pos == NULL ){
      result[0] = strdup(".");
      result[1] = strdup(path);
   }else{
      int diff = (int)( pos - path ) ;
      if( diff == 0 ){
          if( len == 1 )goto bad ; 
          result[0] = strdup("");
          result[1] = strdup(path+1);
      }else if( diff == ( len - 1 ) ){
          goto bad ;
      }else{
          result[0] = strdup( path ) ;
          result[0][diff] = '\0' ;
          result[1] = strdup(path + diff + 1 ) ;
     }
   
   }
   return result ;
   bad :
     free( (void*) result ) ;
     return NULL ;
}
void freeSplitPath( char ** split ){
   if( split == NULL )return ;
   
   if( split[0] != NULL )free( (void *)split[0] ) ;
   if( split[1] != NULL )free( (void *)split[1] ) ;
   
   free( (void*)split ) ;
}
char * getIdByPath( const char * path ){
   int len ;
   char * pos , *result   ;
   FILE * stream ;
   char ** split  = splitPath( path ) ;
   
   if( split == NULL )return NULL ;
 

   result = (char *)malloc( ( len = strlen(path) ) + 128 ) ;

   sprintf( result , "%s/.(id)(%s)" , split[0] , split[1] ) ;

   freeSplitPath(split);
   
   if( ( stream  = fopen( result , "r" ) ) == NULL  )goto bad ;
   result = fgets( result , 128 , stream ) ;
   fclose(stream) ;
   if( result == NULL )goto bad ;
   if( ( ( len = strlen(result) ) > 0 ) && ( result[len-1] == '\n' ) )result[len-1] = '\0' ;
    
   return result ;
   
   bad :
          free(result);
          return NULL ;
   
}
char * getIdBySplitPath( const char * dir , const char * base ){
   int len ;
   char * pos , *result   ;
   FILE * stream ;

   if( ( dir == NULL ) || ( base == NULL) )return NULL ;
   
   result = (char *)malloc( strlen(dir) + strlen(base) + 128 ) ;

   sprintf( result , "%s/.(id)(%s)" , dir , base ) ;
   
   if( ( stream  = fopen( result , "r" ) ) == NULL  )goto bad ;
   result = fgets( result , 128 , stream ) ;
   fclose(stream) ;
   if( result == NULL )goto bad ;
   if( ( ( len = strlen(result) ) > 0 ) && ( result[len-1] == '\n' ) )result[len-1] = '\0' ;
    
   return result ;
   
   bad :
          free(result);
          return NULL ;
   
}
char * stat2string( const struct stat * stat ){
    char * result = (char *)malloc(128);
    sprintf(result,"%o:%i:%i:%x:%x:%x",
            stat->st_mode,
            stat->st_uid,
            stat->st_gid,
            stat->st_ctime,
            stat->st_mtime,
            stat->st_atime);
    return result;
}
int createDirectory( const char * path , const struct stat * stat ){
    char * pnfsId , * virtualFile , * statString;
    int res ;
    char ** split = splitPath(path) ;
    if( split == NULL )return 1 ;
    
     
    if( res = mkdir( path , 0 ) ){
       perror(NULL);
       freeSplitPath(split);
       return 1 ;
    }
    pnfsId = getIdBySplitPath( split[0] , split[1] ) ;
    if( pnfsId == NULL ){
        rmdir( path ) ;
        freeSplitPath(split);
        return 1;
    }
    statString = stat2string( stat ) ;
    
    virtualFile = (char *)malloc( strlen(split[0]) + 128 ) ;
    sprintf(virtualFile,"%s/.(pset)(%s)(attr)(0)(%s)",split[0],pnfsId,statString) ;
    
    freeSplitPath(split);
    free((void*)pnfsId);
    free((void*)statString);
    /*
    printf("DEBUG : opening virtual file %s\n",virtualFile);
    */
    /*
     * open will fail but will do what we need.
     */
    res = open( virtualFile , O_CREAT | O_TRUNC |O_WRONLY , 0 );
    if( res >= 0 )close(res);
    free((void*)virtualFile);
    return 0 ;
}
int createFile( const char * path , struct stat * stat , const char * content , char ** hsm ){
    char * pnfsId , * virtualFile , * statString;
    int res , i ;
    char ** split = splitPath(path) ;
    FILE * stream ;
    if( split == NULL )return 1 ;
    
     
    if( ( res = open( path , O_CREAT | O_WRONLY , 0 ) ) < 0 ){
       perror(NULL);
       freeSplitPath(split);
       return 1 ;
    }
    close(res);
    
    pnfsId = getIdBySplitPath( split[0] , split[1] ) ;
    if( pnfsId == NULL ){
        rmdir( path ) ;
        freeSplitPath(split);
        return 1;
    }
    
    virtualFile = (char *)malloc( strlen(split[0]) + strlen(split[1]) + 128 ) ;
    /*
     * copy the meta data
     */
    statString = stat2string( stat ) ;
    for( i = 0 ; i < 7 ; i++ ){ 
    
       if( i == 2 ){
         free((void *)statString);
         stat->st_uid = 0 ;
         stat->st_gid = 0 ;
         statString = stat2string( stat ) ;
       }
       sprintf(virtualFile,"%s/.(pset)(%s)(attr)(%d)(%s)",split[0],pnfsId,i,statString) ;
/*
       printf("DEBUG : opening virtual file %s\n",virtualFile);
*/
       /*
        * open will fail but will do what we need.
        */
       res = open( virtualFile , O_CREAT | O_TRUNC |O_WRONLY , 0 );
       if( res >= 0 )close(res);
    }
    /*
     * copy the filesize 
     */
    sprintf(virtualFile,"%s/.(pset)(%s)(size)(%d)",split[0],pnfsId,stat->st_size);
    res = open(virtualFile,O_CREAT|O_WRONLY,0);
    if( res >=0 )close(res);
    
    /*
     * fill level 1
     */
    sprintf(virtualFile,"%s/.(puse)(%s)(1)",split[0],pnfsId);
    stream = fopen(virtualFile,"w");
    if( stream == NULL ){
        perror("Open level 1");
        fprintf(stderr,"WARNING : Couldn't write into level one of %s\n",virtualFile);
    }
    fprintf(stream,"%s %s %s\n",hsm[1],hsm[0],content);
    fclose(stream);
    
    free((void*)virtualFile);
    freeSplitPath(split);
    free((void*)pnfsId);
    free((void*)statString);
    return 0 ;
}
char * concatDirectories( const char * dir , const char * base  ){
    char * source = (char *)malloc( strlen(dir) + strlen(base) + 2 ) ;
    strcpy( source , dir ) ;
    strcat( source , "/" ) ;
    strcat( source , base ) ;
    return source ;

}
int checkDirectoryOk( const char * message , char * dir ){
    struct stat value ;
    int res = stat( dir , &value ) ;
    
    if( res != 0 ){
        perror(message) ;
        return 1 ;
    }
    if( ! S_ISDIR( value.st_mode ) ){
        fprintf(stderr,"%s: Not a directory : %s\n",message,dir);
        return 2 ;
    }
    /*
    {
       char * r = stat2string(&value);
       printf("STAT : %s\n",r);
       free((void*)r);
    }
    */
    return 0 ;
}
