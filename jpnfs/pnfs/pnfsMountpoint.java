package jpnfs.pnfs ;
import  java.util.StringTokenizer;

public class pnfsMountpoint implements Cloneable {

   private int     _level = 0  ;
   private String  _real ;
   private String  _virt ;
   private boolean _ioEnabled = false ;
   
   public pnfsMountpoint( String virt , String real , int level ){
     _virt    = virt ;
     _real    = real ;
     _level   = level ;
   }
   public pnfsMountpoint( String virt , String real ){
     _virt    = virt ;
     _real    = real ;
   }
   public pnfsMountpoint( String virt , String real , boolean io ){
     _virt      = virt ;
     _real      = real ;
     _ioEnabled = io ;
   }
   public pnfsMountpoint( String coded ) throws IllegalArgumentException {
     StringTokenizer st = new StringTokenizer( coded ) ;
     if( st.countTokens() < 3 )
          throw new IllegalArgumentException("Not enought arguments" ) ;
     _virt = st.nextToken() ;
     _real = st.nextToken() ;
     int flags  = new Integer( st.nextToken() ).intValue() ;
     _ioEnabled = ( flags / 10 ) == 0  ;
     _level     =   flags % 10   ;

   }
   public Object clone(){ return this.clone() ; }
   public String toString(){
      int flag = ( _ioEnabled ? 0 : 30 ) + ( _level % 10 ) ;
      
      return _virt + "   " + _real + "   " + flag + "     nooptions" ;
   }
   public boolean isIoEnabled(){ return _ioEnabled ; }
   public void    setIoEnabled( boolean io ){ _ioEnabled = io ;  }
   public String  getRealMountpoint(){ return _real ; }
   public String  getVirtualMountpoint(){ return _virt ; }
   
   public boolean equals( Object obj ){
      return ((pnfsMountpoint)obj)._virt.equals( _virt ) ;
   }
   public int hashCode(){  return _virt.hashCode() ; }
}
