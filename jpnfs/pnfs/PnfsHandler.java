package jpnfs.pnfs ;

import java.util.* ;
import java.io.* ;


public class PnfsHandler {

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
       String [] mps = PnfsHandler.getMountpoints() ;
       for( int i = 0 ; i < mps.length ; i++ ){
         System.out.println( mps[i] ) ;
       }
   }

}
