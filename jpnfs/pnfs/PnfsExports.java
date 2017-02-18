package jpnfs.pnfs ;
import  java.io.* ;
import  java.util.* ;

public class PnfsExports implements PnfsExportsDatabase  {

  File      _exportDir  = null ;
  File      _trustedDir = null ;
  File      _mountDir   = null ;
  Hashtable _mpHash     = new Hashtable() ;
  PnfsMountgroupSet _mountSet = null ;
  
  public PnfsExports( String exportDirectory ) throws IOException {
  
     _exportDir = new File( exportDirectory ) ;
     
     if( ( ! _exportDir.exists()      ) ||
         ( ! _exportDir.isDirectory() ) ||
         ( ! _exportDir.canRead()     )     )
         throw new IOException( "Can't access directory" ) ;
         
     _trustedDir = new File( _exportDir , "trusted" ) ;
     if( ! _trustedDir.exists() )_trustedDir = null ; 
     
     _mountDir = new File( _exportDir , "mountpoints" ) ;
     if( ! _mountDir.exists() )_mountDir = null ; 
  
     String [] files     = _exportDir.list() ;
     Hashtable linkHash  = new Hashtable() ;
     
     for( int i = 0 ; i < files.length ; i++ ){
        File f = new File( _exportDir , files[i] ) ;
        if( f.isFile() ){
           File c = new File( f.getCanonicalPath() ) ;
           //
           // destinquish between links and files
           //
           if( c.getName().equals( f.getName() ) ){
//              System.out.println( " Files : "+files[i] ) ;
              //
              //  create a pnfsMountpoint per file
              //  and add it to the mountpointHash 
              //
              PnfsExportEntry mp = new PnfsExportEntry( c.getName() ) ;
              try{
                  BufferedReader in = new BufferedReader( 
                                      new FileReader(  c ) ) ;
                  String line ;
                  while( ( line = in.readLine() ) != null ){
                     if( ( line.length()  == 0 ) ||
                         ( line.charAt(0) == '#'     )    )continue ;
                     try{
                       mp.addMountpoint( new pnfsMountpoint( line ) ) ;
                     }catch( Exception ex ){ continue ; }
                  
                  }
                  in.close() ;
              }catch( IOException ee ){
                  continue ;
              }
              if( _trustedDir != null ){
                 try{
                     BufferedReader in = new BufferedReader( 
                                         new FileReader(  
                                         new File( _trustedDir , 
                                                   c.getName() ) ) ) ;
                     String line ;
                     if( ( line = in.readLine() ) != null ){                        
                         mp.setTrustness( new Integer(line).intValue() ) ;
                     }
                     in.close() ;
                 }catch( Exception ee ){}
              }
              _mpHash.put( c.getName() , mp ) ;
           }else{              
//              System.out.println( " Link  : "+ f.getName()+" -> "+c.getName() ) ;
              //
              // just remember the links, we need them later
              //
              linkHash.put( f.getName() , c.getName() ) ;
           }
        }
//        else if( f.isDirectory() ){
//           System.out.println( " Dir ? : "+files[i] ) ;
//        }else{
//           System.out.println( " ??? ? : "+files[i] ) ;
//        }
     }
     //
     // now add the links to the mountpoints
     //
     Enumeration e = linkHash.keys() ;
     for( ; e.hasMoreElements() ; ){
        String linkName = (String)e.nextElement() ;
        String fileName = (String)linkHash.get( linkName ) ;
        if( fileName == null )continue ; // can't happen ;-)
        PnfsExportEntry mp = (PnfsExportEntry)_mpHash.get( fileName ) ;
        if( mp == null )continue ;
        mp.addHostName( linkName ) ;
     }
      
     for( e = _mpHash.elements() ; e.hasMoreElements() ; )
        ((PnfsModifiable)e.nextElement()).wasChanged(false) ;

     if( _mountDir != null )_mountSet = new PnfsMountgroupSet( _mountDir ) ;
  }
  public Enumeration getGroupNames(){ 
      return _mountSet==null?null:_mountSet.groupNames() ; 
  }
  public Enumeration getMountpoints(String groupName ){ 
      return _mountSet==null?null:_mountSet.getMountpoints(groupName) ; 
  }
  public PnfsMountpoint getMountpoint( String groupName , String vmp ){
      return _mountSet==null?null:_mountSet.getMountpoint(groupName,vmp) ; 
  }
 public String toString(){
     StringBuffer sb = new StringBuffer() ;
     Enumeration e = _mpHash.elements() ;
     for( ; e.hasMoreElements() ; ){
        PnfsExportEntry ee = (PnfsExportEntry) e.nextElement() ;
        sb.append( ee.toString() ) ;
     
     }
     return sb.toString() ;
  }
  public static void main( String [] args ){
     if( args.length < 1 ){
       System.err.println( " USAGE : ... <exportsDir> " ) ;
       System.exit(9) ;
     }
     try{
        PnfsExports pnfs = new PnfsExports( args[0] ) ;
        System.out.println( pnfs.toString() ) ;
     }catch( IOException ioe ){
        System.out.println( " Exception : "+ioe ) ;
        ioe.printStackTrace() ;
     }
     
  }

}

 
