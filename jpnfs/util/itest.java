import java.net.* ;

public class itest {

    public static void main( String [] args ){
       InetAddress [] inet = null ;
       try{
          inet = InetAddress.getAllByName( args[0] ) ;
          for( int i = 0 ; i < inet.length ; i++ ){
             System.out.println( " Address : "+inet[i].toString() ) ;
          }
       }catch( UnknownHostException uhe ){
          System.err.println( " Exception "+uhe ) ;
       }
    
    }

}
