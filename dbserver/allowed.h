#ifndef ALLOWED__H__
#define ALLOWED__H__
/*
#define   mdWriteAllowed(attr,auth) \
    (((((attr)->mst_uid==(auth)->uid)&&((attr)->mst_mode&S_IWUSR))||\
      (((attr)->mst_gid==(auth)->gid)&&((attr)->mst_mode&S_IWGRP))||\
      ((attr)->mst_mode&S_IWOTH))&&(((auth)->uid)||((auth)->priv>10)))      
*/

#define   __mdBasicWriteAllowed(attr,auth) \
        ((((attr)->mst_uid==(auth)->uid)&&((attr)->mst_mode&S_IWUSR))||\
         (((attr)->mst_gid==(auth)->gid)&&((attr)->mst_mode&S_IWGRP))||\
         ((attr)->mst_mode&S_IWOTH))
         
#define   __mdBasicReadAllowed(attr,auth) \
     ((((attr)->mst_uid==(auth)->uid)&&((attr)->mst_mode&S_IRUSR))||\
      (((attr)->mst_gid==(auth)->gid)&&((attr)->mst_mode&S_IRGRP))||\
      ((attr)->mst_mode&S_IROTH)||(!(auth)->uid)) 
            
#ifdef honour_gids
int _mdBasicWriteAllowed( md_unix * attr , md_auth * auth ) ;
int _mdBasicReadAllowed( md_unix * attr , md_auth * auth ) ;
#define  mdBasicWriteAllowed(attr,auth) _mdBasicWriteAllowed(attr,auth)
#define  mdBasicReadAllowed(attr,auth)  _mdBasicReadAllowed(attr,auth)
#else
#define  mdBasicWriteAllowed(attr,auth)  __mdBasicWriteAllowed(attr,auth)
#define  mdBasicReadAllowed(attr,auth)   __mdBasicReadAllowed(attr,auth)
#endif

#define   mdWriteAllowed(attr,auth) \
          (  (  (  (auth)->uid  && mdBasicWriteAllowed(&((attr)->unixAttr),auth))\
              ||((!(auth)->uid) && ((auth)->priv>10)))\
           &&(  (!((attr)->flags&MD_INODE_FLAG_TRUSTED_WRITE))\
              ||((auth)->priv>13)))
     
/*          
#define   mdSetAttrAllowed(attr,auth) \
    (((attr)->mst_uid==(auth)->uid)||((auth)->priv>10))      
*/
/*
#define   mdSetAttrAllowed(attr,auth) \
          (  (  ((attr)->unixAttr.mst_uid==(auth)->uid)\
              ||((auth)->priv>10))\
           &&(  (!((attr)->flags&MD_INODE_FLAG_TRUSTED_WRITE))\
              ||((auth)->priv>13)))
*/
#define   mdSetAttrAllowed(attr,auth) \
          (  (  (  (auth)->uid  && ((attr)->unixAttr.mst_uid==(auth)->uid))		\
	      ||((!(auth)->uid) && ((auth)->priv>10)))		\
           &&(  (!((attr)->flags&MD_INODE_FLAG_TRUSTED_WRITE))\
              ||((auth)->priv>13)))

#define   mdReadAllowed(attr,auth) \
    (((((attr)->unixAttr.mst_uid==(auth)->uid)&&((attr)->unixAttr.mst_mode&S_IRUSR))||\
      (((attr)->unixAttr.mst_gid==(auth)->gid)&&((attr)->unixAttr.mst_mode&S_IRGRP))||\
      ((attr)->unixAttr.mst_mode&S_IROTH)||(!(auth)->uid))&&\
      ((!((attr)->flags&MD_INODE_FLAG_TRUSTED_READ))||((auth)->priv>13)))

#define   mdEverythingAllowed(auth)  ((!(auth)->uid)&&((auth)->priv>10))

#endif

