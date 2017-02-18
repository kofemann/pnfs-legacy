package  jpnfs.graphics ;

import java.awt.* ;
import java.awt.event.* ;
import java.util.* ;
   
import java.io.* ;
   
public class       CardTest 
        extends    Frame  {
                               
    Panel           _cards ;
    CardLayout      _cardsLayout ;
    Canvas          _canvas ;
    Canvas          _helloCanvas1 ;
    Canvas          _helloCanvas2 ;
    
 public CardTest( String [] args ){
      setLayout( new BorderLayout() ) ;  
/*          
      String [] _select = new String[3] ;
      _select[0] = "hello1" ;
      _select[1] = "hello2" ;
      _select[2] = "select3" ;
      
      add( _cards        = new Panel()  , "Center" ) ;
      _cards.setLayout( _cardsLayout = new CardLayout() ) ;
      _cards.add( _helloCanvas1 = new AnyCanvas(200,200,"First Canvas"), "hello1" ) ;
      _cards.add( _helloCanvas2 = new AnyCanvas(200,300,"Second Canvas"), "hello2" ) ;
      _cardsLayout.show( _cards , "mountGroup" ) ;
      add( _canvas = new SelectCanvas(_select , _cards ) , "North" ) ;

      String [] _select = new String[3] ;
      _select[0] = "hello1" ;
      _select[1] = "hello2" ;
      _select[2] = "hello3" ;
      Component [] comp = new Component[_select.length] ;
      for( int i = 0 ; i < _select.length ; i++ )
        comp[i] = new AnyCanvas(200,200,_select[i] ) ;
*/
      add( new Label("CardTest" ) ,"North" ) ;
      Panel cardPanel = new CardPanel() ;
      add( cardPanel ,"Center" ) ;
      
      String [] _select = new String[3] ;
      _select[0] = "hello1" ;
      _select[1] = "hello2" ;
      _select[2] = "hello3" ;
      
      for( int i = 0 ; i < _select.length ; i++ )
          cardPanel.add( new AnyCanvas(200,200, _select[i] ) , "I'm "+_select[i] ) ; 

      pack() ;
      show();
 }

 public static void main( String [] args ){

      new CardTest( args ) ;
    
 }

}
