package jpnfs.dbfs ;

import  jpnfs.shmcom.* ;

import java.io.* ;

public class JmdDirItem implements Serializable {
    private JmdId         _id         = null ;
    private JmdPermission _permission = null ;
    private long          _expire     = 0 ;
    private JmdId         _tag        = null ;
    private String        _name       = null ;
    private long          _cookie     = 0 ;

    public JmdDirItem( JmdId id , String name ){
        _id   = id ;
        _name = name ;  
        _permission = new JmdPermission() ;
          
    }
    public JmdDirItem(){
        _id   = new JmdId() ;
        _name = "" ;  
        _permission = new JmdPermission() ;
          
    }
    public JmdId         getId(){ return _id ; }
    public JmdPermission getPermission(){ return _permission ; }
    public String        getName(){ return _name ; }
    public long          getCookie(){ return _cookie ; }
    public void          setCookie( long cookie ){ _cookie = cookie ; }
    public String toString(){
       return "(id="+_id+";p="+_permission+";n="+_name+";c="+_cookie+")" ;
    }
}  
