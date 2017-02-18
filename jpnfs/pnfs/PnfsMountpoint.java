package jpnfs.pnfs ;
import  java.util.StringTokenizer;

public class PnfsMountpoint implements Cloneable {

   private int     _level = 0  ;
   private String  _real ;
   private String  _virt ;
   private String  _group = null ;
   private boolean _ioEnabled = false ;
   private boolean _isLink    = false ;
   
   public PnfsMountpoint( String virt , String real , 
                          boolean  io , int level   ){
     _virt      = virt ;
     _real      = real ;
     _level     = level ;
     _ioEnabled = io ;
   }
   public PnfsMountpoint( String virt , String real , int level ){
     _virt    = virt ;
     _real    = real ;
     _level   = level ;
   }
   public PnfsMountpoint( String virt , String real ){
     _virt    = virt ;
     _real    = real ;
   }
   public PnfsMountpoint( String virt , String real , boolean io ){
     _virt      = virt ;
     _real      = real ;
     _ioEnabled = io ;
   }
   public PnfsMountpoint( String coded ) throws IllegalArgumentException {
     StringTokenizer st = new StringTokenizer( coded ) ;
     int tokens = st.countTokens() ;
     if( ( tokens < 2 ) || ( tokens == 3 ) )
          throw new IllegalArgumentException("Wrong number of arguments = "+tokens ) ;
          
     _virt = st.nextToken() ;
     _real = st.nextToken() ;
     
     if( tokens == 2 ){
        int i = _real.indexOf( ':' ) ;
        if( i < 0 ){
           if( _real.charAt(0) == '/' ){
              _group = null ;
           }else{
              _group = _real ;
              _real  = null ;
           }
        }else{
           _group = _real.substring( 0 , i ) ;
           _real  = _real.substring( i + 1 ) ;
        }
        _isLink = true ;
     }else{
        int flags  = new Integer( st.nextToken() ).intValue() ;
        _ioEnabled = ( flags / 10 ) == 0  ;
        _level     =   flags % 10   ;
     }
   }
   public Object clone(){ return this.clone() ; }
   public String toString(){
      if( _isLink ){
         return _virt + "   "+
                (_group==null?"":_group)+
                ((_group!=null)&&(_real!=null)?":":"")+
                (_real==null?"":_real) ;
      }else{
         int flag = ( _ioEnabled ? 0 : 30 ) + ( _level % 10 ) ;

         return _virt + "   " + _real + "   " + flag + "     nooptions" ;
      }
   }
   public boolean isIoEnabled(){ return _ioEnabled ; }
   public void    setIoEnabled( boolean io ){ _ioEnabled = io ;  }
   public String  getRealMountpoint(){ return _real ; }
   public String  getVirtualMountpoint(){ return _virt ; }
   public String  getNewVirtualMountpoint(){ return _real ; }
   public String  getMountgroup(){ return _group ; }
   public boolean isLink(){ return _isLink ; }
   public int     getLevel(){ return _level ; }
   public boolean equals( Object obj ){
      return ((PnfsMountpoint)obj)._virt.equals( _virt ) ;
   }
   public int hashCode(){  return _virt.hashCode() ; }
   public static void main( String [] args ){
      PnfsMountpoint mp = new PnfsMountpoint( args[0] ) ;
      System.out.println( ""+mp ) ;
   }
}
