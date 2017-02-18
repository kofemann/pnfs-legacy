package jpnfs.pnfs ;

public class pnfsDbInfo {
   private long _sumRead  = 0 ;
   private long _sumWrite = 0 ;
   private long _sum      = 0 ;
   private long _time     = 0 ;
   private String _dbName = null ;
   private int  _dbId     = 0 ;
   static final private int P_READ   = 1 ;
   static final private int P_WRITE  = 2 ;
   
   static final private int [] _flags = {
      0 , 0, P_READ ,P_READ ,P_READ ,P_READ ,
      P_WRITE ,P_WRITE ,P_WRITE ,
      P_READ ,
      P_WRITE ,P_WRITE ,P_WRITE ,P_WRITE ,
      P_READ , P_READ , 
      P_WRITE ,P_WRITE ,P_WRITE ,P_WRITE ,P_WRITE ,
      P_WRITE ,P_WRITE ,P_WRITE ,P_WRITE ,
      0 ,
      P_READ ,
      P_READ , P_READ , P_READ   } ;
       
   public pnfsDbInfo( String dbName , long [] array , int len )
          throws IllegalArgumentException {
          
        if( len < 30 )throw new IllegalArgumentException( "len < 30") ;
        _dbId   = (int) array[0] ;
        _time   = array[1] ;
        _dbName = dbName ;
        for( int i = 0 ; i < 30 ; i++ ){
        
           if( _flags[i] > 0 ){
             switch( _flags[i] ){
               case P_READ  : _sumRead  += array[i] ; break ;
               case P_WRITE : _sumWrite += array[i] ; break ;            
             }
             _sum += array[i] ;
           }
        }
   
   }
   public int    getDbId()  { return _dbId ; }
   public String getDbName(){ return _dbName ; }
   public long   getTime()  { return _time ; } 
   public long   getSum()   { return _sum ;  }

}
