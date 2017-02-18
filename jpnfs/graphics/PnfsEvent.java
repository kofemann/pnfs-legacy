package jpnfs.graphics ;

import java.util.* ;

public class PnfsEvent extends EventObject {
   String _command ;
   public PnfsEvent( Object source , String command ){
     super( source ) ;
     _command = command ;
   }
   public String getPnfsCommand(){ return _command ; }
}
