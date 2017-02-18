package jpnfs.graphics ;

import  java.util.Hashtable ;

public class SwitchBoard implements Runnable {

   private static Hashtable __listener ;
   private static Thread    __postman ;
   private static Fifo      __fifo ;
   static {
      __listener = new Hashtable() ;
      __fifo     = new Fifo() ;
      new SwitchBoard() ;
      System.out.println( "SwitchBoard : Globals initiated ... " ) ;
   }
   
   public SwitchBoard( String name , SwitchBoardListener l ){
       __listener.put( name , l ) ;
       System.out.println( "SwitchBoard : Listener added : "+name ) ;
   }
   public void sendEvent( String dest , SwitchBoardEvent event ){
       event.setDestination( dest ) ;
       event.setSource( this ) ;
       __fifo.push( event ) ;
   }
   public void sendMessage( String dest , String message ){
      sendEvent( dest , new SwitchBoardEvent( message ) ) ;
   }
   //
   // the event delivery part
   //
   public SwitchBoard(){
      __postman = new Thread(this) ;
      __postman.start() ;
   }
   public void run(){
     if( Thread.currentThread() == __postman ){
        Object obj ;
        while( ( obj = __fifo.pop() ) != null ){
           SwitchBoardEvent    event = (SwitchBoardEvent)obj ;
           SwitchBoardListener dest  = (SwitchBoardListener)__listener.get( event.getDestination() ) ;
           if( dest != null )dest.eventArrived( event ) ;
        
        }   
     }
   }
   

}
