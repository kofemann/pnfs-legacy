package jpnfs.util ;

import java.util.Vector ;


/**
  *  
  *
  * @author Patrick Fuhrmann
  * @version 0.1, 15 Feb 1998
  */
public class Args {

final private int  caNull        = 0 ;
final private int  caStringInit  = 1 ;
final private int  caString      = 2 ;
final private int  caString2Init = 3 ;
final private int  caString2     = 4 ;
final private int  caToken       = 5 ;

private Vector _argv = new Vector() ;
private Vector _optv = new Vector() ;

public int argc(){  return _argv.size() ; }
public int optc(){  return _optv.size() ; }

public String argv( int i ){ 
 
 try{
   return ((StringBuffer)_argv.elementAt(i)).toString() ;
 }catch( ArrayIndexOutOfBoundsException e ){
   return null ;
 }
   
}
public String optv( int i ){ 
 
 try{
   return ((StringBuffer)_optv.elementAt(i)).toString() ;
 }catch( ArrayIndexOutOfBoundsException e ){
   return null ;
 }
   
}
public void shift(){

   try{
     _argv.removeElementAt(0);
   }catch( ArrayIndexOutOfBoundsException e ){} 
   return ;

}
public String [] getArray() {

   String [] strings = new String[_argv.size()] ;
   for( int i = 0 ; i < _argv.size() ; i++ )
       strings[i] = argv(i) ;
       
   return strings ;

}
public  Args( String line ){
   _Args( line ) ;
   int l = _argv.size() ;
   StringBuffer str ;
   for( int i = 0 ; i < l ; i++ ){
      str = (StringBuffer)_argv.elementAt(i) ;
      if( str.charAt(0) == '-' ){
         _optv.addElement( str ) ;
         _argv.removeElementAt(i) ;
         i-- ; l-- ;
      }
   
   }
}
public void  _Args( String line ){
  int i , s = caNull  ;
  int l = line.length() ;
  StringBuffer current ;
  char c ;
  
  if( l == 0 )return ;
  
  current = null ;
  
  for( i = 0  ; i <= l ; i++ ){
     c = i == l ? '\0' : line.charAt(i) ;
   
     switch( s ){
       
       case 0 :  // caNull
       
           
         if( ( c == ' ' ) || ( c == '\0' ) ){
            continue ;
         }else if( c == '"' ){
            s = caStringInit ;
         }else if( c == '\'' ){
            s = caString2Init ;
         }else{
            current = new StringBuffer(64) ;
            current.append( c ) ;
            _argv.addElement( current ) ;
            s = caToken ; 
         }
        
       break ;
      
       case 5 :
         if( ( c == ' ' ) || ( c == '\0' ) ){
            s = caNull ;
         }else{
            current.append( c ) ;
         }
       break ;
       case 1 :  // caStringInit
         if( ( c == '"' ) || ( c == '\0' ) ){
            _argv.addElement( current = new StringBuffer(4) ) ;
            s = caNull ;
         }else{
            _argv.addElement( current = new StringBuffer(64) ) ;
            current.append( c ) ;          
            s = caString ; 
         }
       break ;
       case 2 : //  caString
         if( c == '"' ){
            s = caNull ;
         }else{
            current.append( c ) ;
         }
       break ;
       case 3 :  // caString2Init
         if( ( c == '\'' ) || ( c == '\0' ) ){
            _argv.addElement( current = new StringBuffer(4) ) ;
            s = caNull ;
         }else{
            _argv.addElement( current = new StringBuffer(64) ) ;
            current.append( c ) ;          
            s = caString2 ; 
         }
       break ;
       case 4 :  // caString2
         if( c == '\'' ){
            s = caNull ;
         }else{
            current.append( c ) ;
         }
       break ;
       
     }
     
  }

  return ;
  
}

//
//  the test main routine
//
 
     static public void main( String argv[] ){
       Args args ;
       String [] strings ;
        for( int i = 0 ; i < argv.length ; i++ ){
        
           args = new Args( argv[i] ) ;
           System.out.println( " ->"+argv[i]+"<- Argc : " + args.argc() ) ;
           for( int j = 0 ; j < args.argc() ; j++ ){
           
              System.out.println( j+" ->"+args.argv(j)+"<-");
           }
           for( int j = 0 ; j < args.optc() ; j++ ){
           
              System.out.println( j+" ->"+args.optv(j)+"<-");
           }
           
//           strings = args.getArray() ;
//           for( int j = 0 ; j < strings.length ; j++ )
//              System.out.println( j+" ->"+strings[j]+"<-");
        }
     }


}
