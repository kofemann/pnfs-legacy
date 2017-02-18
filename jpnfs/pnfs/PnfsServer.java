package jpnfs.pnfs ;

import java.util.* ;
import java.io.* ;


public class PnfsServer {
   private String         _pmountpoint = null ;
   private PnfsDbEntry [] _dbEntry     = null ;
   public PnfsServer( String pmountpoint ) throws IOException {

      FileInputStream pnfs = null ;
      try{
         pnfs = new FileInputStream( 
                     new File( _pmountpoint = pmountpoint , 
                               ".(const)(x)" )                 ) ;
      }catch( Exception ie3 ){
         throw new IOException( "Not a pnfs mountpoint : "+pmountpoint );
      }
      try{ pnfs.close() ; }catch(Exception iie ){}
      
      initializeServer() ;
   }

   private void initializeServer() throws IOException {
      Vector v    = new Vector() ;
      for( int i = 0 ; ; i++ ){
         try{
            v.addElement( new PnfsDbEntry( i , _pmountpoint ) ) ;
         }catch( Exception ee ){
            break ;
         }
      }
      _dbEntry = new PnfsDbEntry[ v.size() ] ;
      v.copyInto( _dbEntry ) ;      
   }
   public PnfsDbEntry getDbEntry( int i ){
      if( ( i >= _dbEntry.length ) || ( i < 0 ) )
         throw new
         IllegalArgumentException( "not in range : "+i ) ;
      return _dbEntry[i] ;
   }
   public int size(){ return _dbEntry.length ; }
   //
   //
   //   get the pnfs mountpoints
   //
   public static String [] getMountpoints() throws FileNotFoundException {
   
      String [] mountpoints = null ;  
      try{   
          mountpoints = getPnfsMountpointsFrom( "/etc/mntttab" )  ;
      }catch( FileNotFoundException ee ){
          try{
              mountpoints = getPnfsMountpointsFrom( "/etc/mtab" ) ;
          }catch( FileNotFoundException ff ){
              throw new FileNotFoundException( "Not found mnttab,mtab" ) ;
          }
      }
      return mountpoints ;
   }
   private static String [] getPnfsMountpointsFrom( String mtab )
           throws FileNotFoundException {

      BufferedReader  in ;
      String          line , raw , mp , type ;
      StringTokenizer st ;
      Vector          mountpoints = new Vector() ;
      //
      //
      in = new BufferedReader(new FileReader( mtab ) );
      
      try{
         while(  ( line = in.readLine() ) != null    ){
            try{
               st = new StringTokenizer( line ) ;
               if(  st.countTokens() < 4 )continue ;
               raw  = st.nextToken() ;
               mp   = st.nextToken() ;
               type = st.nextToken() ;
            }catch(Exception ae ){
               continue ;
            }
            
            if( ! type.equals( "nfs" ) )continue ;
            FileInputStream pnfs = null ;
            File mpf = new File( mp ) ;
            try{
               pnfs = new FileInputStream( new File( mpf , ".(const)(x)" ) ) ;
            }catch( Exception ie3 ){
               continue ;
            }finally{
               try{ pnfs.close() ; }catch(Exception iie ){}
            }
            mountpoints.addElement( mp );
         }
      }catch( Exception e ){
         return new String[0] ;
      }finally{
         try{ in.close()  ; }catch(IOException xioe ){}
      }
      String [] out = new String[mountpoints.size()] ;
      mountpoints.copyInto(out);
      return out ;

   }
   public static void main( String [] args ) throws Exception {
       String [] mps = PnfsServer.getMountpoints() ;
       if( mps.length == 0 )System.exit(0) ;
       System.out.println( "Pnfs Mountpoints : ");
       for( int i = 0 ; i < mps.length ; i++ )
          System.out.println( "   mp["+i+"] "+mps[i] ) ;
       System.out.println( "Chosing : "+mps[0] ) ;  
       
       
       PnfsServer  ps = new PnfsServer( mps[0] ) ;
       
       System.out.println( "Number of databases : "+ps.size() ) ;
      
       PnfsDbEntry admin = ps.getDbEntry(0) ;
       if( ! admin.isActive() ){
           System.err.println( "Admin database not active" ) ;
           System.exit(4);
       } 
       System.out.print( "Trying to determine update time " ) ;
       PnfsDbCounters c1 = null , c2 = null , cd = null ;
       c1 = admin.getPnfsDbCounters() ;
       long diff = 0 ;
       while(true){
           try{ Thread.currentThread().sleep(1000) ; }
           catch(InterruptedException ie){ System.exit(45); }
           System.out.print("." ) ;
           c2 = admin.getPnfsDbCounters() ;
           if( ( diff = ( c2.getTimestamp() - c1.getTimestamp() ) ) > 0 )break ;
       }
       diff = diff / 10 + 1 * 10 ;
       System.out.println( " = "+diff ) ;
       //
       while( true ) {
           c2 = admin.getPnfsDbCounters() ;
           cd = c2.substract( c1 ) ;
           System.out.println( cd.toString() );
           System.out.println("NULL : "+cd.getAccessCount("NULL"));
           c1 = c2 ;
           try{ Thread.currentThread().sleep(diff*1000) ; }
           catch(InterruptedException ie){ System.exit(45); }
       }
       /*
       for( int i = 0 ; i < ps.size() ; i++ ){
           PnfsDbEntry e = ps.getDbEntry(i) ;
           if( ! e.isActive() )continue ;
           PnfsDbCounters c = e.getPnfsDbCounters() ;
           System.out.println( c.toString() ) ;
       }
       */
   }
}
