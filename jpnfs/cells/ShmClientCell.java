package jpnfs.cells ;

import jpnfs.dbfs.* ;
import jpnfs.dbserver.* ;
import jpnfs.shmcom.* ;

import dmg.cells.nucleus.* ;
import dmg.util.* ;
import java.io.* ;
import java.util.* ;

public class      ShmClientCell 
       extends    CellAdapter 
       implements Runnable {

   private Args     _args ;
   private String   _cellName ;
   private ShmCom   _shmcom = null ;
   private Thread   _infoThread ;
   
   public ShmClientCell( String cellName , String args ){
      super( cellName , args , false ) ;
      _cellName = cellName ;
      _args     = getArgs() ;
      long shmKey ;
      if( _args.argc()  < 1 ){
         shmKey = 0x1122 ;
      }else{
         shmKey = Integer.parseInt( _args.argv(0) , 16 ) ;
      }
      try{
         
          _shmcom  = new ShmCom( shmKey , 12 * 1024 ) ;
          _infoThread = new Thread( this ) ;
          _infoThread.start() ;
          
      }catch(Exception ee ){
         start() ;
         kill() ;
         throw new IllegalArgumentException( ee.toString() ) ;
      }
      useInterpreter( true ) ;
      start() ;
      pin( "ShmCom started on "+Long.toHexString(shmKey) ) ;
   } 
   public void getInfo( PrintWriter pw ){
   }   
   public void messageArrived( CellMessage msg ){
       Object obj = msg.getMessageObject() ;
       if( obj instanceof ReqObject ){
           pin( "Request ("+msg.getSourcePath()+") : "+obj ) ;
           try{
              _shmcom.postAndWait( (ReqObject) obj ) ;
           }catch( Exception e ){
              obj = e ;
              pin("Result : "+e ) ;
           }
           msg.setMessageObject( obj ) ;
           msg.revertDirection() ;
           try{
              sendMessage( msg ) ;
           }catch(Exception ee ){
              pin( ee.toString() ) ;
           }
       }
   }
   public void cleanUp(){
       try{
          pin( "Closing shared memory" ) ;
          _shmcom.close() ;
       }catch( Exception e){
          pin( "Problem while closing : "+e ) ;
       }
   }
  public void run(){
    if( Thread.currentThread() == _infoThread ){
        Runtime rt = Runtime.getRuntime()  ;
        while( true ){
           try{ Thread.currentThread().sleep(1000) ; }
           catch(Exception ee){}
           System.out.println( "Total : "+rt.totalMemory() ) ;
           System.out.println( "Free  : "+rt.freeMemory() ) ;
           rt.gc();
        }
    }
  }
}
