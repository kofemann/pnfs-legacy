package jpnfs.shmcom ;

import java.io.* ;
import jpnfs.dbfs.* ;
import jpnfs.dbserver.* ;

public class ShmCom {

   public native long initShmCom( long key , int size ) ;
   public native void closeShmCom( long base ) ;
   public native long postAndWait( long base , long io , int slot , int wt ) ;
   public native long clientGetBuffer( long base ) ;

   public native int  postAndWait( long base , int slot , int command ,
                                   byte [] array , int offset , int length ,
                                   int  waitTime   ) ;


   public native int  postAndWait( long base , ReqObject request );
   public native static void set4Data( byte [] array , int offset , int value );
   public native static int  get4Data( byte [] array , int offset  );

   private long _base ;
   private int  _rc ;
   private int  _status ;
   private int  _answer ;
   private static final int POST_TIMEOUT = 5 ; 
  
   static {
       System.loadLibrary( "pnfs" ) ;   
   }
   public ShmCom( long key , int size ) throws IOException {
      _base = initShmCom( key , size ) ;
      if( _base == 0 )
         throw new 
         IOException( "Problem initializing shmcom : "+_rc ) ;
      
   }
   public int getStatus(){ return _status ; }
   public int getAnswer(){ return _answer ; }
   
   public int postAndWait( ReqObject request ) throws ShmException {
      if( postAndWait( _base , request ) != 0 ){
          throw new ShmException( request ) ;
      }
      return request.getAnswer() ;
   }
   
   public int postAndWait( int slot , int command , byte [] array , int length ){
   
      return postAndWait( _base , slot , command , 
                          array , 0 , length , POST_TIMEOUT);
       
   }
   public synchronized void close(){
      if( _base != 0 ){
         closeShmCom( _base ) ;
         _base = 0 ;
      }
   }
   public long getBase(){ return _base ; }
   
}
