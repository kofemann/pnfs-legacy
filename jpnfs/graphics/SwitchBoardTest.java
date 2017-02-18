package jpnfs.graphics ;

public class SwitchBoardTest implements SwitchBoardListener {
   private String _name ;
   private SwitchBoard  _switchboard ;
   public SwitchBoardTest( String name ){
      _name       = name ;
      _switchboard = new SwitchBoard( _name  , this ) ; 
   }
   public void eventArrived( SwitchBoardEvent event ){
      System.out.println( " "+_name+" : "+event ) ;
   }
   public void send( String dest , String message ){
      _switchboard.sendEvent( dest , new SwitchBoardEvent( message ) ) ;
   }
   public static void main( String [] args ){
      SwitchBoardTest a = new SwitchBoardTest("first" ) ;
      SwitchBoardTest b = new SwitchBoardTest("second" ) ;
      a.send( "first" , "first to first hallo" ) ;
      b.send( "first" , "second to first hallo" ) ;
      a.send( "second" , "first to second hallo" ) ;
      b.send( "second" , "second to second hallo" ) ;
   }

}
