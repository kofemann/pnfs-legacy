package jpnfs.dbfs ;

import  jpnfs.shmcom.* ;

import java.io.* ;

public class JmdId implements Serializable {
    private int  _db    = 0 ;
    private int  _ext   = 0 ;
    private long _high  = 0 ;
    private long _low   = 0 ;

    public static final int ROOT  = 0x100 ;
    public static final int FIRST = 0x1000 ;
    public static final int NULL  = 0 ;
    public JmdId(){}
    public JmdId( int type ){ 
       _low = type  ;
    }
    public JmdId( int type , int db ){ 
       _db  = db  ;
       _low = type ;
      
    }
    public JmdId( String id ){
    
       byte [] a = new byte[24] ;
       
       if( a.length == 0 )return ;
       int len = id.length() ; 
       int rev ;
       for( int i = 0 ; ( i < 24 ) && ( i < len ) ; i++ )
          a[i] = (byte)Integer.parseInt( id.substring(len-i-1,len-i) , 16) ;
       
        
       _db   = ( ( a[23] & 0xf ) << 12 ) | ( ( a[22] & 0xf ) << 8 ) |
               ( ( a[21] & 0xf ) <<  4 ) | ( a[20] & 0xf ) ; 
       _ext  = ( ( a[19] & 0xf ) << 12 ) | ( ( a[18] & 0xf ) << 8 ) |
               ( ( a[17] & 0xf ) <<  4 ) | ( a[16] & 0xf ) ; 
               
       _high = ( ( a[15] & 0xf ) << 28 ) | ( ( a[14] & 0xf ) << 24 ) |
               ( ( a[13] & 0xf ) << 20 ) | ( ( a[12] & 0xf ) << 16 ) |
               ( ( a[11] & 0xf ) << 12 ) | ( ( a[10] & 0xf ) << 8 ) |
               ( ( a[9] & 0xf ) <<  4 ) | ( a[8] & 0xf ) ; 
        
       _low  = ( ( a[7] & 0xf ) << 28 ) | ( ( a[6] & 0xf ) << 24 ) |
               ( ( a[5] & 0xf ) << 20 ) | ( ( a[4] & 0xf ) << 16 ) |
               ( ( a[3] & 0xf ) << 12 ) | ( ( a[2] & 0xf ) << 8 ) |
               ( ( a[1] & 0xf ) <<  4 ) | ( a[0] & 0xf ) ; 
        
        
    }
    
    public String toString(){ 
        return  _tostring(_db,4)+
                _tostring(_ext,4)+
                _tostring(_high,8)+
                _tostring(_low,8) ;
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
        if( args.length < 1 ){
           System.err.println( "Usage : ... <pnfsid>" ) ;
           System.exit(5);
        }
        System.out.println( new JmdId( args[0] ) ) ;
    }
}
