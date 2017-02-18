#include <jni.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

#ifdef _WIN32
#define PATH_SEPARATOR ';'
#else /* UNIX */
#define PATH_SEPARATOR ':'
#endif

#define SYSTEM_PATH "/usr/lib/java/lib/classes.zip" /* where Prog.class is */

    JNIEnv *env;
    JavaVM *jvm;
    
static FILE * _outputDevice = NULL ;
static void _openOutputDevice(){
   if( _outputDevice == NULL ){
       _outputDevice = fopen( "/tmp/waste" , "w" ) ;
   }
} 
static int 
_shmClientPrintf( FILE *stream , const char *format , va_list args  ) 
{
   _openOutputDevice() ;
   printf( "Producing output\n" ) ;
   if( _outputDevice != NULL ){
      vfprintf( _outputDevice , format, args) ;
      fflush( _outputDevice ) ;
   }
   return 0 ;

}

static
void
handleErrors(void)
{
        if ((*env)->ExceptionOccurred(env)) {
                (*env)->ExceptionDescribe(env);
                (*env)->ExceptionClear(env);
                (*jvm)->DetachCurrentThread(jvm);
        }
}
static int _lastSignal = 0 ;
static 
void 
termSignal( int signal )
/* ====================================================================== */
{
    printf( "Signal Called %d\n" , _lastSignal  ) ;
   _lastSignal = signal ;
}
int getLastSignal(){ return _lastSignal ; }
void clearSignal(){ _lastSignal = 0 ; } 

main( int argc , char * argv[]  ) {
    JDK1_1InitArgs vm_args;
    jint res;
    jclass cls;
    jmethodID mid;
    jstring jstr[128];
    jobjectArray args;
    int argOffset = 1 ; /* set to 2 if first arg. should be the class */
    char classpath[1024];
    char * extpath = NULL ;
    char starterClass[512] ; 
    char stdOutput[512] ; 
    char * domainName = NULL ;
    int i ;
    int argPos    = 0 ;
    int isDeamon  = 1 ;
    int useSignal = 0 ;
     
    if( argc < 2 ){
       fprintf(stderr,"Usage : %s <domainName> [<domainOptions>]\n",argv[0]);
       exit(4);
    }
    /*
     * initialize the vm arguments
     */
    vm_args.version = 0x00010001;

    JNI_GetDefaultJavaVMInitArgs(&vm_args);

    if( ( extpath = getenv( "CLASSPATH" ) ) != NULL )   
        sprintf( classpath , "%s:%s",SYSTEM_PATH,extpath ) ;
    else
        strcpy( classpath , SYSTEM_PATH ) ;
    
    strcpy( starterClass , "dmg/cells/services/Domain" ) ; 
    strcpy( stdOutput    , "/dev/null" ) ; 
    
    vm_args.classpath       = classpath;
    vm_args.enableVerboseGC = 1 ;
    vm_args.disableAsyncGC  = 0 ;
 /* vm_args.vfprintf        = _shmClientPrintf ; */
 
    printf( "Classpath : %s\n",vm_args.classpath ) ;
    
    {
      /*
       * split the arguments into program options , domain name and
       * domain options.
       */
      char key[512] ;
      char * value ;
      int l ;
      for( argPos = 1 ; ( argPos < argc ) && ( (argv[argPos])[0] == '-' ) ; argPos++ ){
         strncpy( key , argv[argPos] , 511 ) ;
         key[511] = '\0' ;
         if( ( value = strchr( key , '=' ) ) != NULL ){
           *value = '\0' ;
            value++ ;
         }
         if( ! strcmp( key , "-c" ) ){
            int m ;
            if( value == NULL )continue ;
            strcpy( starterClass , value ) ;
            m = strlen(starterClass) ;
            for(l=0;l<m;l++)if( starterClass[l] == '.' )starterClass[l] = '/' ;
         }else if( ! strcmp( key , "-d" ) ){
            if( value == NULL )continue ;
            if( ! strcmp( value , "no" ) )isDeamon = 0 ;
            else isDeamon = 1 ;
         }else if( ! strcmp( key , "-s" ) ){
            if( value == NULL )continue ;
            if( ! strcmp( value , "no" ) )useSignal = 0 ;
            else useSignal = 1 ;
         }else if( ! strcmp( key , "-o" ) ){
            if( value == NULL )continue ;
            strcpy( stdOutput , value ) ;
         }else if( ! strcmp( key , "-ns" ) ){
            if( value == NULL )continue ;
            sscanf( value , "%d" , &nativeStack ) ;
         }

      }
      if( argPos < argc )domainName = argv[argPos++] ;
      
    }

    
    
    
    
    /* Create the Java VM */
    res = JNI_CreateJavaVM(&jvm,&env,&vm_args);
    if (res < 0) {
        fprintf(stderr, "Can't create Java VM\n");
        exit(1);
    }
    cls = (*env)->FindClass(env, starterClass );
    if (cls == 0) {
        fprintf(stderr, "Can't find %s class\n", starterClass );
        exit(1);
    }
 
    mid = (*env)->GetStaticMethodID(env, cls, "main", "([Ljava/lang/String;)V");
    if (mid == 0) {
        fprintf(stderr, "Can't find 'main'\n");
        exit(1);
    }
    
    args = (*env)->NewObjectArray(env, 
                                  argc-argPos, 
                                  (*env)->FindClass(env, "java/lang/String"), 
                                  NULL);
    if (args == 0) {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }
                                  
    for( i = 0 ; i < (argc-argPos) ; i++ ){
        jstr[i] = (*env)->NewStringUTF(env, argv[i+argPos] );
        if (jstr[i] == 0) {
           fprintf(stderr, "Out of memory\n");
           exit(1);
        }
       (*env)->SetObjectArrayElement(env,args,i,jstr[i]);
    }
                   

    if( isDeamon ){
       if( fork() )exit(0);
       close(0);
       freopen( stdOutput , "w" , stdout ) ;
       freopen( stdOutput , "w" , stderr ) ;
       setpgrp() ;
    }

    

    if( useSignal ){
       struct sigaction newAction ;

       printf( "Installing signal handler\n" ) ;
       memset( (char *)&newAction , 0 , sizeof(newAction) ) ;
       newAction.sa_handler = termSignal ;

       sigaction( SIGTERM , &newAction , NULL ) ;

    }
 

    printf( "Going into Call....\n") ;
    (*env)->CallStaticVoidMethod(env, cls, mid, args);
    handleErrors() ;
    printf( "exiting Call....\n") ;
    /*
    (*jvm)->DetachCurrentThread(jvm);
   */
   
    (*jvm)->DestroyJavaVM(jvm);
    
}

