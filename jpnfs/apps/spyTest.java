package jpnfs.apps ;

import jpnfs.pnfs.* ;
import java.io.* ;
public class spyTest {
  
  public static  void main( String [] args ){
    if( args.length < 2 ){
       System.err.println( " USAGE : spyTest <pnfsPath> <dbId> ...." ) ;
       System.exit(3);
    }
    pnfsCounters [] counters = new pnfsCounters[32] ;
    int counterMax = 0 , i ;
    String virtualFile = null ;
    try{
       for( i = 1 ; i < args.length ; i++ ){
          virtualFile   = args[0]+"/.(get)(counters)("+args[i]+")" ;
          counters[i-1] = new pnfsCounters( virtualFile ) ;
          System.out.println( " Open : "+virtualFile ) ;
       }
       counterMax = i - 1 ; 
    }catch( IOException ioe ){
       System.err.println( " pnfsCounters says : "+ioe ) ;   
       System.exit(4);    
    }
    while( true ){
      long [] ar ;
      long    sum ;
      for( i = 0 ; i < counterMax ; i++ ){
         ar  = counters[i].getValues() ;
         sum = 0 ;
         for( int j = 1 ; j < ar.length ; j ++ )sum += ar[j] ;       
      }
      try{ Thread.currentThread().sleep(10000) ; }
      catch( InterruptedException ioe ){} ;
      System.out.println( "" ) ;
    }

  }  
  
  
  
  
}
