package jpnfs.graphics ;
import  java.util.Vector ;
 
/**
  *  
  *
  * @author Patrick Fuhrmann
  * @version 0.1, 15 Feb 1998
  */
 public class Fifo extends Vector {
 
   
   public synchronized void push( Object o ){
      addElement( o ) ;
      notifyAll() ;
   } 
   public synchronized Object pop() {
      while( true ){ 
         if( ! isEmpty() ){
            Object tmp = firstElement() ;
            removeElementAt(0) ;
            return tmp ;
         }else{
            try{ wait() ; }
            catch( InterruptedException e ){}
         }
      }
   } 
   public synchronized Object pop( long timeout ) {
      while( true ){ 
         if( ! isEmpty() ){
            Object tmp = firstElement() ;
            removeElementAt(0) ;
            return tmp ;
         }else{
            try{ wait( timeout ) ; }
            catch( InterruptedException e ){}
         }
      }
   } 
   public Object spy(){
      return firstElement() ;
   }
   protected void finalize() throws Throwable {
     super.finalize() ;
//     System.out.println( " FIFO finalizer called " ) ;
   }
 }
