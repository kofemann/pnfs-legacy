package jpnfs.dbserver ;

import  jpnfs.dbfs.* ;

import java.io.* ;

public class ReqLookup extends ReqObject {
   private  JmdAuth       _auth = null ;
   private  JmdId         _id   = null ;
   private  JmdPermission _permission = null ;
   private  String        _name = null ;
   private  JmdDirItem    _item = null ;
   private  JmdUnix       _attr = null ;

   public ReqLookup( JmdId id , String name ){
       super(ReqObject.LOOKUP) ;
       _id   = id ;
       _name = name ;
       _permission = new JmdPermission() ;
       _item = new JmdDirItem() ;
       _attr = new JmdUnix() ;
       _auth = new JmdAuth() ;
   }
   public JmdId getResultId(){ return _item.getId() ; }
   public JmdDirItem getDirItem(){ return _item ; }
   public String toString(){
   
      return "(r="+super.toString()+
             ";id="+_id+
             ";p="+_permission+
             ";name="+_name+
             ";dir="+_item+
             ";a="+_attr+")" ;
   }
  
}   
