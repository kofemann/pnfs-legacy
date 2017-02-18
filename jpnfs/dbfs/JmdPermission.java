package jpnfs.dbfs ;

import  jpnfs.shmcom.* ;

import java.io.* ;

public class JmdPermission implements Serializable {
    private long _high  = 0 ;
    private long _low   = 0 ;

    public JmdPermission(){}
    public JmdPermission( int level ){
       _low = level ;
    }
    public void setLevel( int level ){
       _low = ( _low & ~0x7 ) | ( level & 0x7 ) ;
    }
    public int  getLevel(){ return (int)(_low & 0x7) ; }
    public void setModifyLevel( boolean set ){     
        _low = ( _low & ~ 0x8 ) | ( set ? 0x8 : 0 ) ;
    }
    public boolean isModifyLevel(){
       return ( _low & 0x8 ) > 0 ;
    }
    public void setModifyNoIo( boolean noio ){     
        _low = ( _low & ~ 0x20 ) | ( noio ? 0x20 : 0 ) ;
    }
    public boolean isModifyNoIo(){
       return ( _low & 0x20 ) > 0 ;
    }
    public void setNoIo( boolean noio ){     
        _low = ( _low & ~ 0x10 ) | ( noio ? 0x10 : 0 ) ;
    }
    public boolean isNoIo(){
       return ( _low & 0x10 ) > 0 ;
    }
    public void setSpecialIdDetail( int detail ){
       _high = ( _high & ~0x3f ) | ( detail & 0x3f ) ;
    }
    public int  getSpecialIdDetail(){ return (int)(_high & 0x3f) ; }
    
    public String toString(){ 
        int det = getSpecialIdDetail() ;
        return  
                "(l="+getLevel()+
                (isModifyLevel()?";ml":"")+
                (isNoIo()?";noio":"")+
                (isModifyNoIo()?";mnoio":"")+
                (det>0?";d="+getSpecialIdDetail():"")+")" ;
    }
    private String _tostring( long x , int n ){
       String t = Integer.toHexString((int)(x&0xffffffff));
       StringBuffer sb = new StringBuffer() ;
       int len = n - t.length() ;
       for( int i = 0 ; i < len  ; i++ )sb.append( '0' ) ;
       sb.append( t ) ;
       return sb.toString() ;
    }
    public static void main( String [] args ){
        JmdPermission perm = new JmdPermission() ;
        System.out.println( perm ) ;
        perm.setLevel(4) ;
        System.out.println( perm ) ;
        perm.setModifyLevel(true) ;
        System.out.println( perm ) ;
        perm.setNoIo(true) ;
        System.out.println( perm ) ;
        perm.setModifyNoIo(true) ;
        System.out.println( perm ) ;
        perm.setSpecialIdDetail(40) ;
        System.out.println( perm ) ;
        perm.setNoIo(false) ;
        System.out.println( perm ) ;
    }
} 
