package jpnfs.dbfs ;

import jpnfs.shmcom.* ;
import java.util.Date ;
import java.io.* ;

public class JmdUnix implements Serializable {
     private long   _ino  = 0 ;
     private int    _dev  = 0;
     private int    _rdev = 0 ;
     private int    _mode = 0 ;
     private int    _nlink = 0 ;
     private int    _uid   = 0 ;
     private int    _gid   = 0 ;
     private long   _size  = 0 ;
     private long   _atime = 0 ;
     private long   _mtime = 0 ;
     private long   _ctime = 0 ;
     private long   _blksize = 512 ;
     private long   _blocks  = 0 ;
     public JmdUnix(){}
     
     public void setUid( int uid ){ _uid = uid ; }
     public void setGid( int gid ){ _gid = gid ; }
     public void setMode( int mode ){ _mode = mode ; }
     public void setAccessTime( Date time ){ 
        _atime = time.getTime() / 1000 ;
     }
     public void setModificationTime( Date time ){ 
        _mtime = time.getTime() / 1000 ;
     }
     public void setCreationTime( Date time ){
        _ctime = time.getTime() / 1000 ;
     }
     public void setSize( long size ){ _size = size ; }
     public int getUid(){ return _uid ; }
     public int getGid(){ return _gid ; }
     public int getMode(){ return _mode ; }
     public Date getAccessTime(){ return new Date( _atime*1000 ) ; }
     public Date getModificationTime(){ return new Date( _mtime*1000 ) ; }
     public Date getCreationTime(){ return new Date( _ctime*1000 ) ; }
     
     public String toString(){ 
       return "(uid="+_uid+";gid="+_gid+
              ";mode="+Integer.toOctalString(_mode) +")" ;
     }
}  
