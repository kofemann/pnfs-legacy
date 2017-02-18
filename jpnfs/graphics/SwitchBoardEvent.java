package jpnfs.graphics ;


public class SwitchBoardEvent extends java.util.EventObject {

   private String _message ;
   private Object _object = null ;
   private String _dest = "unknown" ;
   
   public SwitchBoardEvent( String message , Object obj ){
     
      super( message ) ; 
      _message = message ;
      _object  = obj ;
   }
   public SwitchBoardEvent( String message ){
      super( message ) ; 
      _message = message ;
      _object  = null ;
   }
   public String getMessage(){ return _message ; }
   public Object getObject(){ return _object ; }
   public String toString(){
      return "dest="+_dest+";Msg="+_message ;
   }
   void setSource( Object obj ){ super.source = obj ; }
   void setDestination( String dest ){ _dest = dest ; }
   String getDestination(){ return _dest ; }
}
