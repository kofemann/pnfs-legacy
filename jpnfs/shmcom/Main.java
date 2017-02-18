

public class Main implements Runnable {

    public static void test( int i ){
       System.out.println( "Hallo, I got an "+i ) ;
       
       Main m = new Main() ;
       Thread t = new Thread( m ) ;
       t.start() ;
       try{
          Thread.currentThread().sleep( 10000 ) ;
       }catch( Exception e ){
          System.out.println( "Got an exception" ) ;

       }
       System.out.println( "Hallo, I got an "+i ) ;
    }
    public static void interrupt( int i ){
       System.out.println( "Interrupt : "+i ) ;
    }
    public void run(){
         System.out.println( "Thread started" ) ;
         while(true){
             System.out.println( "Ping" ) ;
             try{
               Thread.currentThread().sleep(1000) ;
             }catch(Exception e ){}
         }
        
    }
}
