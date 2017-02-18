#ifndef JMD_BINDING
#define JMD_BINDING

void _o2n_JmdId( JNIEnv *env , md_id_t * id , jobject jid );
void _n2o_JmdId( JNIEnv *env , md_id_t * id , jobject jid );
void _o2n_JmdPermission( JNIEnv *env , md_permission * perm , jobject jperm );
void _n2o_JmdPermission( JNIEnv *env , md_permission * perm , jobject jperm );
void _o2n_JmdAuth( JNIEnv *env , md_auth * auth , jobject jauth ) ;
void _n2o_JmdAuth( JNIEnv *env , md_auth * auth , jobject jauth ) ;
void _o2n_JmdUnix( JNIEnv *env , md_unix * unixc , jobject junix ) ;
void _n2o_JmdUnix( JNIEnv *env , md_unix * unixc , jobject junix ) ;
void _o2n_JmdDirItem( JNIEnv *env , md_dir_item * dirItem , jobject jdirItem ) ;
void _n2o_JmdDirItem( JNIEnv *env , md_dir_item * dirItem , jobject jdirItem ) ;
jobject _n2o_String(  JNIEnv *env , const char * str  ) ;
void _o2n_String(  JNIEnv *env, char * str,const jobject jstring,int size );

#endif
