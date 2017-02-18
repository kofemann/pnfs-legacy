package jpnfs.pnfs ;
import java.io.* ;

public class CheckDB implements Runnable {

   Object  _waitLock   = new Object() ;
   Thread  _openThread = new Thread( this ) ;
   String  _filename   = null ;
   int     _ok         = 0 ;
   
   public CheckDB( String pnfs , int dbSlot  ){
      _filename = pnfs + "/.(get)(counters)("+dbSlot+")" ;
      
      synchronized( _waitLock ){
         _openThread.start() ;
         try { 
            System.out.println( " Going to wait " ) ;
            _waitLock.wait( 5000 ) ; 
            System.out.println( " Wait finished" ) ;
         }
         catch( InterruptedException ie ) {} 
         _openThread.stop() ;
         
      }
   }
   public int  isActive(){ return _ok ; } 
   public void run(){
      if( Thread.currentThread() == _openThread ){
        try{
          System.out.println(" Trying to open "+_filename ) ;
          FileInputStream in = new FileInputStream( _filename ) ; 
          System.out.println(" File opened" ) ;  
          _ok = 1 ;
          in.close() ;
          synchronized( _waitLock ){ _waitLock.notify() ; } ;
        }catch( Exception fnf ){
          _ok= -1 ;
        }
      }
   
   }
   public static void main( String [] args ){
      if( args.length < 2 )System.exit(3);
      int slot = new Integer( args[1] ).intValue() ;
      
      CheckDB db = new CheckDB( args[0] , slot ) ;
      int result = db.isActive() ;
      System.out.println( " result "+result ) ;
   
   }

}
