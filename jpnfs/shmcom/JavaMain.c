#include <jni.h>
#include <signal.h>

static JavaVM *jvm;       /* denotes a Java VM */
static JNIEnv *env;       /* pointer to native method interface */
void termSignal( int signal )
{
   jclass cls;
   jmethodID mid;
   JNIEnv  *thisEnv ;
   int rc ;
      printf( "termSignal %d\n",signal ) ;
      rc = (*jvm)->AttachCurrentThread( jvm , &thisEnv , NULL ) ;
      printf( "Return from attach %d\n",rc);
      if( rc ){
         printf( "Attachment failed : %d\n" , rc ) ;
         thisEnv = env ;
      }
      cls = (*thisEnv)->FindClass( thisEnv , "Main");
      printf( "Main class : %x\n" , cls ) ;
      if( cls == NULL ){
         printf( "Main not found in interrupt handler\n" ) ;
         return ;
      }
      mid = (*thisEnv)->GetStaticMethodID( thisEnv , cls, "interrupt", "(I)V");
      printf( "interrupt function : %x\n",mid ) ;
      if( mid == NULL ){
         printf( "Main.interrupt not found in interrupt handler\n" ) ;
         return ;
      }
      
      (*thisEnv)->CallStaticVoidMethod(thisEnv,cls, mid, signal);
}
void main( int argc , char * argv[] ){


      jclass cls;
      JDK1_1InitArgs vm_args; /* JDK 1.1 VM initialization arguments */
      jmethodID mid;
{
   struct sigaction newAction ;
   
   memset( (char *)&newAction , 0 , sizeof(newAction) ) ;
   newAction.sa_handler = termSignal ;
   newAction.sa_flags   = SA_RESTART ;
   
   sigaction( SIGTERM , &newAction , NULL ) ;
   
   
}
      
      vm_args.version = 0x00010001; /* New in 1.1.2: VM version */
      /* Get the default initialization arguments and set the class 
       * path */
      JNI_GetDefaultJavaVMInitArgs(&vm_args);
      vm_args.classpath = "/usr/lib/java/lib/classes.zip:/home/patrick/cvs-pnfs/pnfs/jpnfs/shmcom" ;

      /* load and initialize a Java VM, return a JNI interface 
       * pointer in env */
      JNI_CreateJavaVM(&jvm, &env, &vm_args);

      /* invoke the Main.test method using the JNI */
      
      cls = (*env)->FindClass( env , "Main");
      printf( "Main class : %x\n" , cls ) ;
      mid = (*env)->GetStaticMethodID( env , cls, "test", "(I)V");
      printf( "test function : %x\n",mid ) ;
      
      (*env)->CallStaticVoidMethod(env,cls, mid, 100);

      /* We are done. */
      (*jvm)->DestroyJavaVM( jvm );
      
      exit(0);

}
 
