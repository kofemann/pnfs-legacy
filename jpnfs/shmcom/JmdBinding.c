#include <jni.h>
#include "jpnfs_shmcom_ShmCom.h"
#include "shmcom.h"
#include "md2types.h"
#include "sdef.h"
#include <stdio.h>
#include "JmdBinding.h"

void _o2n_JmdId( JNIEnv *env , md_id_t * id , jobject jid ){
 
    jfieldID fid_id_ext , fid_id_high , fid_id_low , fid_id_db ; 
    jclass class_id  = (*env)->GetObjectClass( env , jid ) ;
    
    fid_id_db   = (*env)->GetFieldID( env , class_id , "_db"   , "I" ) ;
    fid_id_ext  = (*env)->GetFieldID( env , class_id , "_ext"  , "I" ) ;
    fid_id_high = (*env)->GetFieldID( env , class_id , "_high" , "J" ) ;
    fid_id_low  = (*env)->GetFieldID( env , class_id , "_low"  , "J" ) ;

    memset( (char*) id , 0 , sizeof( md_id_t ) ) ;

    id -> db   = (*env)->GetIntField( env , jid , fid_id_db ) ; 
    id -> ext  = (*env)->GetIntField( env , jid , fid_id_ext ) ; 
    id -> high = (jint)(*env)->GetLongField( env , jid , fid_id_high ) ; 
    id -> low  = (jint)(*env)->GetLongField( env , jid , fid_id_low ) ;
    return ;
}
void _n2o_JmdId( JNIEnv *env , md_id_t * id , jobject jid ){
 
    jfieldID fid_id_ext , fid_id_high , fid_id_low , fid_id_db ; 
    jclass class_id  = (*env)->GetObjectClass( env , jid ) ;
    
    fid_id_db   = (*env)->GetFieldID( env , class_id , "_db"   , "I" ) ;
    fid_id_ext  = (*env)->GetFieldID( env , class_id , "_ext"  , "I" ) ;
    fid_id_high = (*env)->GetFieldID( env , class_id , "_high" , "J" ) ;
    fid_id_low  = (*env)->GetFieldID( env , class_id , "_low"  , "J" ) ;

    (*env)->SetIntField(  env , jid , fid_id_db ,id -> db ) ; 
    (*env)->SetIntField(  env , jid , fid_id_ext ,id -> ext) ; 
    (*env)->SetLongField( env , jid , fid_id_high ,id -> high) ; 
    (*env)->SetLongField( env , jid , fid_id_low ,id -> low) ;
    return ;
}
void _o2n_JmdPermission( JNIEnv *env , md_permission * perm , jobject jperm ){
 
    jfieldID fid_perm_high , fid_perm_low ; 
    jclass class_perm  = (*env)->GetObjectClass( env , jperm ) ;
    
    fid_perm_high = (*env)->GetFieldID( env , class_perm , "_high" , "J" ) ;
    fid_perm_low  = (*env)->GetFieldID( env , class_perm , "_low"  , "J" ) ;

    memset( (char*) perm , 0 , sizeof( md_permission ) ) ;
    perm -> high = (jint)(*env)->GetLongField( env , jperm , fid_perm_high ) ; 
    perm -> low  = (jint)(*env)->GetLongField( env , jperm , fid_perm_low ) ;
    return ;
}
void _n2o_JmdPermission( JNIEnv *env , md_permission * perm , jobject jperm ){
 
    jfieldID fid_perm_high , fid_perm_low ; 
    jclass class_perm  = (*env)->GetObjectClass( env , jperm ) ;
    
    fid_perm_high = (*env)->GetFieldID( env , class_perm , "_high" , "J" ) ;
    fid_perm_low  = (*env)->GetFieldID( env , class_perm , "_low"  , "J" ) ;

    (*env)->SetLongField( env , jperm , fid_perm_high ,perm -> high) ; 
    (*env)->SetLongField( env , jperm , fid_perm_low ,perm -> low) ;
    return ;
}
void _o2n_JmdAuth( JNIEnv *env , md_auth * auth , jobject jauth ){
    jclass class_auth  = (*env)->GetObjectClass( env , jauth ) ;
    jfieldID fid_priv  = (*env)->GetFieldID( env , class_auth , "_priv" , "I" ) ;
    
    memset( (char*) auth , 0 , sizeof( md_auth ) ) ;
    
    auth -> priv  = (*env)->GetIntField( env , jauth , fid_priv ) ;
}
void _n2o_JmdAuth( JNIEnv *env , md_auth * auth , jobject jauth ){
    jclass class_auth  = (*env)->GetObjectClass( env , jauth ) ;
    jfieldID fid_priv  = (*env)->GetFieldID( env , class_auth , "_priv" , "I" ) ;
    
    (*env)->SetIntField( env , jauth , fid_priv , auth -> priv ) ;
}
void _o2n_JmdUnix( JNIEnv *env , md_unix * unixc , jobject junix ){

    jclass class_unix  = (*env)->GetObjectClass( env , junix ) ;
    jfieldID fid_uid   = (*env)->GetFieldID( env , class_unix , "_uid"  , "I" ) ;
    jfieldID fid_gid   = (*env)->GetFieldID( env , class_unix , "_gid"  , "I" ) ;
    jfieldID fid_mode  = (*env)->GetFieldID( env , class_unix , "_mode" , "I" ) ;
    jfieldID fid_atime = (*env)->GetFieldID( env , class_unix , "_atime" , "J" ) ;
    jfieldID fid_mtime = (*env)->GetFieldID( env , class_unix , "_mtime" , "J" ) ;
    jfieldID fid_ctime = (*env)->GetFieldID( env , class_unix , "_ctime" , "J" ) ;
    
    memset( (char*) unixc , 0 , sizeof( md_unix ) ) ;
    
    unixc -> mst_uid   = (*env)->GetIntField( env , junix , fid_uid ) ;
    unixc -> mst_gid   = (*env)->GetIntField( env , junix , fid_gid ) ;
    unixc -> mst_mode  = (*env)->GetIntField( env , junix , fid_mode ) ;
    
    unixc -> mst_atime = (*env)->GetLongField( env , junix , fid_atime ) ;
    unixc -> mst_mtime = (*env)->GetLongField( env , junix , fid_mtime ) ;
    unixc -> mst_ctime = (*env)->GetLongField( env , junix , fid_ctime ) ;
    
    return ;
}
void _n2o_JmdUnix( JNIEnv *env , md_unix * unixc, jobject junix ){
    jclass class_unix  = (*env)->GetObjectClass( env , junix ) ;
    jfieldID fid_uid   = (*env)->GetFieldID( env , class_unix , "_uid"  , "I" ) ;
    jfieldID fid_gid   = (*env)->GetFieldID( env , class_unix , "_gid"  , "I" ) ;
    jfieldID fid_mode  = (*env)->GetFieldID( env , class_unix , "_mode" , "I" ) ;
    jfieldID fid_atime = (*env)->GetFieldID( env , class_unix , "_atime" , "J" ) ;
    jfieldID fid_mtime = (*env)->GetFieldID( env , class_unix , "_mtime" , "J" ) ;
    jfieldID fid_ctime = (*env)->GetFieldID( env , class_unix , "_ctime" , "J" ) ;
        
    (*env)->SetIntField( env , junix , fid_uid , unixc -> mst_uid) ;
    (*env)->SetIntField( env , junix , fid_gid , unixc -> mst_gid) ;
    (*env)->SetIntField( env , junix , fid_mode , unixc -> mst_mode) ;
    
    (*env)->SetLongField( env , junix , fid_atime , unixc -> mst_atime) ;
    (*env)->SetLongField( env , junix , fid_mtime , unixc -> mst_mtime) ;
    (*env)->SetLongField( env , junix , fid_ctime , unixc -> mst_ctime) ;
    
    return ;
}
void _o2n_String(  JNIEnv *env , char * str , const jobject jstring , int size ){
    const char * utfChars = (*env)->GetStringUTFChars( env , jstring , NULL ) ;
    int    utfLen   = (*env)->GetStringUTFLength( env , jstring ) ;
    size -= 1 ;
    utfLen =  ( size < utfLen ? size : utfLen ) ;
    strncpy( str , utfChars , utfLen ) ;
    str[utfLen] = '\0' ;
    (*env)->ReleaseStringUTFChars(env , jstring , utfChars ) ;

}
jobject _n2o_String(  JNIEnv *env , const char * str  ){
    return (*env)->NewStringUTF( env , str ) ;

}
void _o2n_JmdDirItem( JNIEnv *env , md_dir_item * dirItem , jobject jdirItem ){
   jclass class_item  = (*env)->GetObjectClass( env , jdirItem ) ;
   jfieldID fid_id    = (*env)->GetFieldID( env , class_item , 
                        "_id"  , "Ljpnfs/dbfs/JmdId;" ) ;
   jfieldID fid_perm  = (*env)->GetFieldID( env , class_item , 
                        "_permission"  , "Ljpnfs/dbfs/JmdPermission;" ) ;
   jfieldID fid_name  = (*env)->GetFieldID( env , class_item , 
                        "_name"  , "Ljava/lang/String;" ) ;

   jobject object_id   = (*env)->GetObjectField( env , jdirItem , fid_id ) ;
   jobject object_perm = (*env)->GetObjectField( env , jdirItem , fid_perm ) ;
   jobject object_name = (*env)->GetObjectField( env , jdirItem , fid_name ) ;

   memset( (char*) dirItem , 0 , sizeof( md_dir_item ) ) ;
   
   _o2n_JmdId( env , &( dirItem->ID ) , object_id ) ;
   _o2n_JmdPermission( env , &( dirItem-> perm ) , object_perm ) ;
   _o2n_String( env , dirItem->name , object_name , MD_MAX_NAME_LENGTH ) ;
}
void _n2o_JmdDirItem( JNIEnv *env , md_dir_item * dirItem , jobject jdirItem ){
   jclass class_item  = (*env)->GetObjectClass( env , jdirItem ) ;
   jfieldID fid_id    = (*env)->GetFieldID( env , class_item , 
                        "_id"  , "Ljpnfs/dbfs/JmdId;" ) ;
   jfieldID fid_perm  = (*env)->GetFieldID( env , class_item , 
                        "_permission"  , "Ljpnfs/dbfs/JmdPermission;" ) ;
   jfieldID fid_name  = (*env)->GetFieldID( env , class_item , 
                        "_name"  , "Ljava/lang/String;" ) ;

   jobject object_id   = (*env)->GetObjectField( env , jdirItem , fid_id ) ;
   jobject object_perm = (*env)->GetObjectField( env , jdirItem , fid_perm ) ;
   jobject object_name ;
   
   _n2o_JmdId( env , &( dirItem->ID ) , object_id ) ;
   _n2o_JmdPermission( env , &( dirItem-> perm ) , object_perm ) ;
   object_name =_n2o_String( env , dirItem->name  ) ;
   (*env)->SetObjectField( env , jdirItem , fid_name , object_name ) ;
}













