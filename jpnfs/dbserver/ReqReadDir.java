package jpnfs.dbserver ;

import  jpnfs.dbfs.* ;

import java.io.* ;

public class ReqReadDir extends ReqObject {
   private  JmdAuth       _auth = new JmdAuth() ;
   private  JmdId         _id   = null ;
   private  JmdPermission _permission = new JmdPermission() ;
   private  JmdDirItem [] _items  = new JmdDirItem[4] ;
   private  int           _count  = _items.length ;
   private  long          _cookie = 0 ;

   public ReqReadDir( JmdId id , long cookie ){
       super( ReqObject.READDIR ) ;
      _id     = id ;
      _cookie = cookie ;
      for( int i = 0 ; i < _count ; i++ )_items[i] = new JmdDirItem() ;
   }
   public JmdDirItem [] getDirItems(){ 
      JmdDirItem [] res = new JmdDirItem[_count] ;
      for( int i = 0 ; i < _count  ; i++ )res[i] = _items[i] ; 
      return res ; 
   }
   public String toString(){
      StringBuffer sb = new StringBuffer() ;
      sb.append( "("+super.toString() ) ;
      sb.append( "c="+_cookie+";[" ) ;
      for( int i = 0 ; i < _count ; i++ )
         sb.append( _items[i].toString() ) ;
      sb.append( "])" ) ;
      return sb.toString() ;
   }  
}
