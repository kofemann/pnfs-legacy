import dmg.util.Fifo ;
import java.util.* ;

public class LockTest implements Runnable {

    public static void main( String [] args ){
       
       LockTest m = new LockTest() ;

    }
    private Thread _send = new Thread( this ) ;
    private Thread _recv = new Thread( this ) ;
    private Fifo   _fifo = new Fifo() ;
    public LockTest(){
        _send.start() ;
        _recv.start() ;
    }
    public static void interrupt( int i ){
       System.out.println( "Interrupt : "+i ) ;
    }
    public void run(){
       Thread current = Thread.currentThread() ;
       if( current == _send )
          run_send() ;
       else
          run_recv() ;
        
    }
    public void run_send(){
         System.out.println( "Send Thread started" ) ;
         while(true){
             _fifo.push( new Date() ) ;
             try{
               Thread.currentThread().sleep(1000) ;
             }catch(Exception e ){}
         }
    
    }
    public void run_recv(){
         System.out.println( "Recv Thread started" ) ;
         while(true){
            Object obj = _fifo.pop() ;
            System.out.println( "Got : "+obj.toString() ) ;
         }
    
    }
}
