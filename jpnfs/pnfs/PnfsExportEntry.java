package jpnfs.pnfs ;
import  java.util.* ;

public class PnfsExportEntry implements PnfsModifiable {

   String     _ipNumber ;
   Hashtable  _hostNames   = new Hashtable() ;
   Hashtable  _mountpoints = new Hashtable() ;
   int        _trustness   = 0 ;
   boolean    _modi        = false ;
   
   public PnfsExportEntry( String ipNumber ){
      _ipNumber = ipNumber ;
      wasChanged(true) ;
   }
   public void removeHostName( String host ){
      _hostNames.remove( host ) ;
      wasChanged(true) ;
   }
   public void addHostName( String host ){
      _hostNames.put( host , host ) ;
      wasChanged(true) ;
   }
   public void removeMountpoint( String vmp ){
      _mountpoints.remove( vmp ) ;
      wasChanged(true) ;
   }
   public void addMountpoint( pnfsMountpoint mp ){
      _mountpoints.put( mp.getVirtualMountpoint() ,  mp ) ;
      wasChanged(true) ;
   }
   public Enumeration hosts(){ return _hostNames.elements() ; }
   public Enumeration mountpoints(){ return _mountpoints.elements() ; }
   public void setTrustness( int t ){ _trustness = t ; }
   public void wasChanged( boolean t ){  _modi = t ; }
   public boolean wasChanged(){ return _modi ; }
   public String toString(){
      StringBuffer sb = new StringBuffer() ;
      sb.append( " PnfsExportEntry : "+_ipNumber+"\n" ) ;
      sb.append( " Trustness : "+_trustness+"\n" ) ;
      sb.append( " Hostnames : \n" ) ;
      for( Enumeration e = _hostNames.elements() ; e.hasMoreElements();){
         sb.append( "     "+e.nextElement().toString()+"\n" ) ;
      }
      sb.append( " Mountpoints : \n" ) ;
      for( Enumeration e = _mountpoints.elements() ; e.hasMoreElements();){
         sb.append( "     "+e.nextElement().toString()+"\n" ) ;
      }
      return sb.toString() ;
   }
}
