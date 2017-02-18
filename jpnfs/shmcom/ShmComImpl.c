#include <jni.h>
#include "jpnfs_shmcom_ShmCom.h"
#include "shmcom.h"
#include "md2types.h"
#include "sdef.h"
#include <stdio.h>
#include "JmdBinding.h"

/*
 * Class:     ShmCom
 * Method:    initShmCom
 * Signature: ()V
 */
JNIEXPORT jlong JNICALL Java_jpnfs_shmcom_ShmCom_initShmCom
  (JNIEnv * env , jobject obj , jlong key , jint size ) {
  
    SCL  *  scl ;
    int     rc  ;
    jfieldID fid ;
     
    jclass cls = (*env)->GetObjectClass(env, obj);
    fid = (*env)->GetFieldID(env, cls, "_rc", "I");
    if( fid == 0 ){
      return 0;
    }      
    (*env)->SetIntField(env, cls, fid, 0);
    
    scl = sclClientOpen( key , size , &rc ) ;
    if( scl == NULL ){
       (*env)->SetIntField(env, obj, fid, rc );
       return 0 ;
    }
    
    return (long) scl  ;
}
JNIEXPORT void JNICALL Java_jpnfs_shmcom_ShmCom_closeShmCom
  (JNIEnv *env , jobject obj , jlong  base ){

    SCL  *  scl = (SCL*) (long)base ;
    
    sclClose( scl ) ;  
}
JNIEXPORT jlong JNICALL Java_jpnfs_shmcom_ShmCom_postAndWait__JJII
  (JNIEnv *env , jobject obj , jlong base , jlong ioBase , jint slot , jint wt){
   
    int     rc ;
    SCL   * scl = (SCL*) (long)base ;
    SCLIO * sclio = sclClientPostAndWait( scl , 
                                  (SCLIO*)(int)ioBase , 
                                  slot , 
                                  wt , 
                                  &rc ) ;
                                  
    if( sclio == NULL ){
       jclass   cls   = (*env)->GetObjectClass(env, obj);
       jfieldID fid   = (*env)->GetFieldID(env, cls, "_rc", "I");
       (*env)->SetIntField(env, obj, fid, rc );
       return 0 ;
    }

    return (long) sclio ;
  
}

JNIEXPORT jlong JNICALL Java_jpnfs_shmcom_ShmCom_clientGetBuffer
  (JNIEnv * env , jobject obj , jlong base ){
  
    int      rc ;
    SCL   *  scl   = (SCL*) (long)base ;
    SCLIO *  sclio = sclClientGetBuffer( scl , &rc )  ;
    
    
    if( sclio == NULL ){
       jclass   cls   = (*env)->GetObjectClass(env, obj);
       jfieldID fid   = (*env)->GetFieldID(env, cls, "_rc", "I");
       (*env)->SetIntField(env, obj, fid, rc );
       return 0 ;
    }
    
    return (long) sclio ;
      
}
JNIEXPORT jint JNICALL Java_jpnfs_shmcom_ShmCom_postAndWait__JII_3BIII
  (JNIEnv * env , jobject obj , 
   jlong base , jint slot , jint command ,
   jbyteArray array  , jint offset , jint length , jint waitTime ){
   
  
    int        rc ;
    reqBuffer *rb ;
    char      *buffer , * jBuffer ;
    SCL   *  scl   = (SCL*) (long)base ;
    SCLIO *  sclio = sclClientGetBuffer( scl , &rc )  ;
    jclass   cls   = (*env)->GetObjectClass(env, obj);
    
    jfieldID fid_rc       = (*env)->GetFieldID(env, cls, "_rc", "I");
    jfieldID fid_status   = (*env)->GetFieldID(env, cls, "_status", "I");
    jfieldID fid_answer   = (*env)->GetFieldID(env, cls, "_answer", "I");
    
    
    if( sclio == NULL ){
       (*env)->SetIntField(env, obj, fid_rc, rc );
       return rc ;
    }
    
    rb = (reqBuffer *)sclioBuffer(sclio) ;
    setReqBufferHead( rb , command , length ) ;
    
    jBuffer = (*env)->GetByteArrayElements( env , array , NULL ) ;
    buffer  = &(rb->data[0]) ;
    
    memcpy( buffer , &jBuffer[offset] , length ) ;
    
    sclio = sclClientPostAndWait(scl ,sclio , slot  , waitTime ,&rc ) ;
    if( sclio == NULL ){
       (*env)->ReleaseByteArrayElements( env , array , jBuffer , JNI_ABORT) ;
       (*env)->SetIntField(env, obj, fid_rc, rc );
       return rc ;
    }
    
    rb = (reqBuffer *)sclioBuffer(sclio) ;
    buffer  = &(rb->data[0]) ;
    
    memcpy( &jBuffer[offset] , buffer ,length ) ;
    
    (*env)->SetIntField(env, obj, fid_status , rb->status );
    (*env)->SetIntField(env, obj, fid_answer , rb->answer );
   
    (*env)->ReleaseByteArrayElements( env , array , jBuffer , 0 ) ;
    return rb->status ;
}
JNIEXPORT void JNICALL Java_jpnfs_shmcom_ShmCom_set4Data
  (JNIEnv *env , jclass cl , jbyteArray array , jint offset , jint value ){
  
    char * jBuffer = (*env)->GetByteArrayElements( env , array , NULL ) ;

#ifdef _LITTLE_ENDIAN
    jBuffer[offset+0] = value & 0xFF ;
    jBuffer[offset+1] = ( value >>  8 ) & 0xFF ;
    jBuffer[offset+2] = ( value >> 16 ) & 0xFF ;
    jBuffer[offset+3] = ( value >> 24 ) & 0xFF ;
#else
    jBuffer[offset+3] = value & 0xFF ;
    jBuffer[offset+2] = ( value >>  8 ) & 0xFF ;
    jBuffer[offset+1] = ( value >> 16 ) & 0xFF ;
    jBuffer[offset+0] = ( value >> 24 ) & 0xFF ;
#endif
    (*env)->ReleaseByteArrayElements( env , array , jBuffer , 0 ) ;
  
}
JNIEXPORT jint JNICALL Java_jpnfs_shmcom_ShmCom_get4Data
  (JNIEnv *env , jclass cl, jbyteArray array , jint offset ){
  
    char * jBuffer = (*env)->GetByteArrayElements( env , array , NULL ) ;
    int value = 0 ;
#ifdef _LITTLE_ENDIAN
     value =
     ( (  jBuffer[offset+3] & 0xFF )  << 24 ) |
     ( (  jBuffer[offset+2] & 0xFF )  << 16 ) |
     ( (  jBuffer[offset+1] & 0xFF )  <<  8 ) |
     ( (  jBuffer[offset+0] & 0xFF )  <<  0 )   ;
#else
     value =
     ( (  jBuffer[offset+0] & 0xFF )  << 24 ) |
     ( (  jBuffer[offset+1] & 0xFF )  << 16 ) |
     ( (  jBuffer[offset+2] & 0xFF )  <<  8 ) |
     ( (  jBuffer[offset+3] & 0xFF )  <<  0 )   ;
#endif
    (*env)->ReleaseByteArrayElements( env , array , jBuffer , 0 ) ;
  
    return value ;
  
}
typedef struct {
    jobject    request ;
    jclass     class_request ;
    jfieldID   fid_command , fid_timeout , fid_status , fid_answer ;
    jfieldID   fid_id , fid_name , fid_auth , fid_item , fid_unix , fid_perm ;
    jobject    object_id , object_name , object_auth , object_item ,
               object_unix , object_perm ; 

    int        command , timeout ;
} resObject ;
static void 
resolveRequestObject( JNIEnv * env , jobject request , resObject * res ){
   memset( (char*) res , 0 , sizeof( resObject ) ) ;

   res->class_request = (*env)->GetObjectClass(env, request ) ;  

   res->fid_command  = (*env)->GetFieldID(env, res->class_request, 
                        "_command", "I");
   res->fid_timeout  = (*env)->GetFieldID(env, res->class_request, 
                        "_timeout", "I");
   res->fid_status   = (*env)->GetFieldID(env, res->class_request, 
                        "_status", "I");
   res->fid_answer   = (*env)->GetFieldID(env, res->class_request, 
                        "_answer", "I");
                        
   res->command = (*env)->GetIntField(env,request,res->fid_command) ;
   res->timeout = (*env)->GetIntField(env,request,res->fid_timeout) ;
   
   res->fid_auth  = (*env)->GetFieldID(env, res->class_request, 
                        "_auth", "Ljpnfs/dbfs/JmdAuth;");
   res->fid_id    = (*env)->GetFieldID(env, res->class_request, 
                        "_id", "Ljpnfs/dbfs/JmdId;");
   res->fid_perm  = (*env)->GetFieldID(env, res->class_request,
                        "_permission", "Ljpnfs/dbfs/JmdPermission;");
   res->fid_name  = (*env)->GetFieldID(env, res->class_request,
                        "_name", "Ljava/lang/String;");
   res->fid_item  = (*env)->GetFieldID(env, res->class_request,
                        "_item", "Ljpnfs/dbfs/JmdDirItem;");
   res->fid_unix  = (*env)->GetFieldID(env, res->class_request,
                        "_attr", "Ljpnfs/dbfs/JmdUnix;");
   (*env)->ExceptionClear( env ) ;
           
   if( res->fid_id != NULL ) 
      res->object_id   = (*env)->GetObjectField( env , request , res->fid_id ) ;
   if( res->fid_name != NULL ) 
      res->object_name = (*env)->GetObjectField( env , request , res->fid_name ) ;
   if( res->fid_perm != NULL ) 
      res->object_perm = (*env)->GetObjectField( env , request , res->fid_perm ) ;
   if( res->fid_auth != NULL ) 
      res->object_auth = (*env)->GetObjectField( env , request , res->fid_auth ) ;
   if( res->fid_item != NULL ) 
      res->object_item = (*env)->GetObjectField( env , request , res->fid_item ) ;
   if( res->fid_unix != NULL ) 
      res->object_unix = (*env)->GetObjectField( env , request , res->fid_unix ) ;
}
JNIEXPORT jint JNICALL 
  Java_jpnfs_shmcom_ShmCom_postAndWait__JLjpnfs_dbserver_ReqObject_2
  (JNIEnv *env , jobject obj , jlong base  , jobject request  ){
  
    int        rc  ;
    SCL   *    scl   = (SCL*) (long)base ;
    SCLIO *    sclio = sclClientGetBuffer( scl , &rc )  ;
    reqBuffer *rb    = (reqBuffer *)sclioBuffer(sclio) ;
    jclass     class_this    = (*env)->GetObjectClass(env, obj);
    resObject  ro ; 
    
    resolveRequestObject( env , request , &ro ) ;   

    switch( ro.command ){
       case RBR_LOOKUP : 
       case RBR_LOOKUP_ONLY : 
       {
           reqLookup  *lu = (reqLookup *)&(rb->data[0]) ;
           memset( (char*) lu , 0 , sizeof( reqLookup ) ) ;

           _o2n_JmdAuth( env , &( lu -> auth ) , ro.object_auth ) ;
           _o2n_JmdId( env , &( lu -> id ) , ro.object_id ) ;
           _o2n_JmdPermission( env , &( lu -> perm ) , ro.object_perm ) ;
           _o2n_String( env , lu -> name , ro.object_name , 1024 ) ;

           setReqBufferHead( rb , ro.command , sizeof(reqLookup)  ) ;
           sclio = sclClientPostAndWait(scl ,sclio , lu->id.db  , ro.timeout ,&rc ) ;
           if( sclio == NULL ){
              (*env)->SetIntField(env,request,ro.fid_answer,rc) ;
              (*env)->SetIntField(env,request,ro.fid_status,SRB_ERROR) ;
              return rc ;
           }
           rb = (reqBuffer *)sclioBuffer(sclio) ;
           lu = (reqLookup *)&(rb->data[0]) ;
           
           (*env)->SetIntField(env,request,ro.fid_answer,rb->answer) ;
           (*env)->SetIntField(env,request,ro.fid_status,rb->status) ;
           
           if( rb->status != SRB_OK )return rb -> answer ;
           
           _n2o_JmdDirItem( env , &( lu -> item ) , ro.object_item ) ;
           _n2o_JmdUnix( env , &( lu -> attr ) , ro.object_unix ) ;
           
           return 0 ;
       }
       case RBR_READDIR : 
       {
           reqReadDir  * lu    = (reqReadDir *)&(rb->data[0]) ;
           jfieldID fid_items  = (*env)->GetFieldID(env, ro.class_request,
                                 "_items", "[Ljpnfs/dbfs/JmdDirItem;");
           jfieldID fid_cookie = (*env)->GetFieldID(env,ro.class_request,
                                 "_cookie" , "J" ) ;
           jfieldID fid_count  = (*env)->GetFieldID(env,ro.class_request,
                                 "_count" , "I" ) ;
           jobject obj_items = (*env)->GetObjectField(env,request,fid_items);
           int     cookie    = (*env)->GetIntField(env,request,fid_cookie) ;
           int     count     = (*env)->GetArrayLength(env,obj_items) ;
           int     i ;
           
           memset( (char*) lu , 0 , sizeof( reqReadDir ) ) ;

           _o2n_JmdAuth( env , &( lu -> auth ) , ro.object_auth ) ;
           _o2n_JmdId( env , &( lu -> id ) , ro.object_id ) ;
           _o2n_JmdPermission( env , &( lu -> perm ) , ro.object_perm ) ;
           
           lu -> cookie = cookie ;
           lu -> count  = count ; 
           
           setReqBufferHead( rb , ro.command , sizeof(reqReadDir)  ) ;
           sclio = sclClientPostAndWait(scl ,sclio , lu->id.db  , ro.timeout ,&rc ) ;
           if( sclio == NULL ){
              (*env)->SetIntField(env,request,ro.fid_answer,rc) ;
              (*env)->SetIntField(env,request,ro.fid_status,SRB_ERROR) ;
              return rc ;
           }
           rb = (reqBuffer *)sclioBuffer(sclio) ;
           lu = (reqReadDir *)&(rb->data[0]) ;
           
           count = lu -> count ; 
           
           (*env)->SetIntField(env,request,ro.fid_answer,rb->answer) ;
           (*env)->SetIntField(env,request,ro.fid_status,rb->status) ;
           
           if( rb->status != SRB_OK )return rb -> answer ;
           {
               jclass class_item  = (*env)->FindClass(env,
                                    "jpnfs/dbfs/JmdDirItem" ) ;
               jfieldID fid_id    = (*env)->GetFieldID( env , class_item , 
                                    "_id"  , "Ljpnfs/dbfs/JmdId;" ) ;
               jfieldID fid_perm  = (*env)->GetFieldID( env , class_item , 
                                    "_permission"  , "Ljpnfs/dbfs/JmdPermission;" ) ;
               jfieldID fid_name  = (*env)->GetFieldID( env , class_item , 
                                    "_name"  , "Ljava/lang/String;" ) ;
               jfieldID fid_cky   = (*env)->GetFieldID( env , class_item , 
                                    "_cookie"  , "J" ) ;


               for( i = 0 ; i < count ; i ++ ){
                   jobject element = 
                        (*env)->GetObjectArrayElement(env,obj_items,i) ;
                   jobject object_id   = 
                        (*env)->GetObjectField( env , element , fid_id ) ;
                   jobject object_perm = 
                        (*env)->GetObjectField( env , element , fid_perm ) ;
                   jobject object_name = 
                        (*env)->GetObjectField( env , element , fid_name ) ;

                   jobject str = _n2o_String( env , lu->e[i].item.name ) ;
                   
                   _n2o_JmdId( env , &( lu->e[i].item.ID ) , object_id ) ;
                   _n2o_JmdPermission( env , 
                                       &( lu->e[i].item.perm ) , 
                                       object_perm ) ;
                   (*env)->SetObjectField(env,element,fid_name,str);
                   (*env)->SetLongField(env,
                                        element,
                                        fid_cky ,
                                        (jlong)lu->e[i].cookie) ;

               }
           }
           (*env)->SetIntField(env,request,fid_count,count) ;
           
           return 0 ;
       }
    }
    return 0 ; 
}

