package  jpnfs.graphics ;

import java.awt.* ;
import java.awt.event.* ;
import java.util.* ;
   
public  class      CardPanel 
        extends    Panel       {
                               
    Panel           _cards ;
    CardLayout      _cardsLayout ;
    SelectCanvas    _canvas ;
    String     []   _string ;
    Component  []   _panel ;

   public CardPanel( Component [] panel , String [] string ){
      _panel  = new Component[panel.length] ;
      _string = new String[panel.length] ;
      for( int i = 0 ; i < panel.length ; i++ ){
         _panel[i]  = panel[i] ;
         _string[i] = string[i] ;
      }
      setLayout( new BorderLayout() ) ;  
      super.add( _cards  = new Panel()  , "Center" ) ;
      _cards.setLayout( _cardsLayout = new CardLayout() ) ;
      for( int i = 0 ; i < _panel.length ; i++ )
         _cards.add( _panel[i] , _string[i] ) ;
         
      super.add( _canvas = new SelectCanvas(_string , _cards ) , "North" ) ;
   
   }
   public CardPanel(){
      _panel  = new Component[0] ;
      _string = new String[0] ;
      setLayout( new BorderLayout() ) ;  
      super.add( _cards  = new Panel()  , "Center" ) ;
      _cards.setLayout( _cardsLayout = new CardLayout() ) ;
         
      super.add( _canvas = new SelectCanvas(_string , _cards ) , "North" ) ;
   
   }
   public synchronized void add( Component comp , Object obj ){
       _cards.add( comp , obj ) ;
       _canvas.addString( (String) obj ) ;
   }

}
