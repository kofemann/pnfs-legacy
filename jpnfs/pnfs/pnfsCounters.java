package jpnfs.pnfs ;

import java.io.* ;

public class pnfsCounters implements Runnable {
   Thread            _worker ;
   RandomAccessFile  _io ;
   long        []    _array ;
   long              _lastTime = 0 ;
   int               _arLen ;
   int               _intervall     = 5 ;
   int               _autoIntervall = 0 ;
   String            _dbName        = "unknown" ;
   
   public pnfsCounters( String theFile ) throws IOException {
      _io     = new RandomAccessFile( theFile , "r" ) ;
      _array  = new long[128] ;
      _arLen  = 0 ;
//      _worker = new Thread( this ) ;
//      _worker.start() ;
   }
   public int  getIntervall(){ return _autoIntervall ; }
   public void setIntervall( int i ){ _intervall = i ; }
   public synchronized int getLength(){ return _arLen ; }
   public synchronized long [] getValuesAsync(){
      long [] ar = new long[ _arLen ] ;
      for( int i = 0 ; i < _arLen ; i++ )
         ar[i] = _array[i] ;
      return ar ;
   }
   public synchronized long getValueOf( int pos ) { 
      if( ( pos >= 0 ) && ( pos < _arLen ) )return _array[pos] ;
      return 0 ;
   }
   public void run(){
     String line , x ;
     long l ;
     int i;
     if( Thread.currentThread() == _worker ){
       while( true ){
          try{
             _readData() ;
             try{ Thread.currentThread().sleep(_intervall) ; }
             catch( InterruptedException ie ){} ;
          }catch( IOException ioe ){
            System.err.println( "IOException : "+ioe ) ;
            break ;
          }
       }
     }
   }
   private void _readData() throws IOException {
       String line , x ;
       int i ;
       _io.seek( 0L ) ; 
       for( i = 0 ; ( line = _io.readLine() ) != null ; i++ ){
           try{      
              x         = line.substring( line.indexOf('=')+1 ) ;
              _array[i] = new Long( x ).longValue() ;
              if( i == 0 ){
                 _dbName = line.substring( 0 , line.indexOf('=') ) ;
              }
           }catch( Exception e ){
              break ;
           }
       }
       _lastTime = i > 0 ? _array[1] : 0L ;
       _arLen = i ;
       return ;
   }
   public synchronized long [] getValues(){
      long last = _lastTime ;
      try { _readData() ; }
      catch( IOException ioe ){ return new long[0] ; }
      if( _arLen <= 0 )return new long[0] ;
      
      long [] ar = new long[ _arLen ] ;
      for( int i = 0 ; i < _arLen ; i++ )ar[i] = _array[i] ;
      if( ( last      != 0 ) && 
          ( _lastTime != 0 ) &&
          ( _autoIntervall == 0 ) &&
          ( last != _lastTime ) )_autoIntervall = (int)( _lastTime - last ) ;
      return ar ;
   }
   public synchronized  pnfsDbInfo getInfo(){
      long last = _lastTime ;
      try { _readData() ; }
      catch( IOException ioe ){ return null ; }
      if( _arLen <= 0 )return null ;
//      System.out.println( " last "+last+" <-> "+_lastTime) ;
      if( ( last      != 0 ) && 
          ( _lastTime != 0 ) &&
          ( _autoIntervall == 0 ) &&
          ( last != _lastTime ) )_autoIntervall = (int)( _lastTime - last ) ;
          
      return new pnfsDbInfo( _dbName , _array , _arLen )  ;
   }

}
