package jpnfs.shmcom ;

import java.io.* ;
import jpnfs.dbfs.* ;
import jpnfs.dbserver.* ;

public class ShmException extends Exception  {
   private ReqObject _request = null ;
   
   public ShmException( ReqObject request , String message ){
        super( message ) ;
        _request = request ;
   }
   public ShmException( ReqObject request ){
        super( request.toString() ) ;
   }
   public int getStatus(){ return _request.getStatus() ; }
   public int getAnswer(){ return _request.getAnswer() ; }
   public ReqObject getReqObject(){  return _request ; }

}
