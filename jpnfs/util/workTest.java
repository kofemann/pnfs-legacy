
import java.awt.* ;
import java.awt.event.* ;
import java.util.* ;
   
import java.io.* ;
   
 public class      workTest 
        extends    Frame 
        implements ActionListener {
                               
    MenuBar         _menuBar ;
    Menu            _editMenu , _fileMenu , _widthMenu ;
    MenuItem        _editEditItem ;
    MenuItem        _fileExitItem ;
    Label           _textField ;
    jWorksheet      _textArea ;
    Panel           _cards ;
    CardLayout      _cardsLayout ;
    LogoCanvas      _logoCanvas ;
    
    public workTest( String [] args ){
      
      setLayout( new BorderLayout() ) ;      
             
      add( _textField    = new Label() , "North" ) ;
      add( _cards        = new Panel() , "Center" ) ;

      _cards.setLayout( _cardsLayout = new CardLayout() ) ;
      
      _cards.add( _textArea     = new jWorksheet() , "worksheet" ) ;
      _textArea.setActionListener( this ) ; 
      
      _cards.add( _logoCanvas     = new LogoCanvas() , "logo" ) ;
      _logoCanvas.setActionListener( this ) ;
      _cardsLayout.show( _cards , "logo" ) ;
      
      installMenu();
      
      
//      setSize( 600 , 400 );      
      pack();
//      setSize( 600 , 400 );
      show();
       
 }
 private void installMenu(){
      _menuBar      = new MenuBar() ;
      
      _fileMenu     = new Menu( "File" ) ;
      
      _fileMenu.add( _fileExitItem = new MenuItem( "Exit" ) );
      _fileExitItem.addActionListener( this ) ;
      _fileExitItem.setActionCommand( "exit" ) ;
      
      _editMenu     = new Menu( "Edit" ) ;
      
      _editMenu.add( _editEditItem = new MenuItem( "Edit Topology" ) );
      _editEditItem.addActionListener( this ) ;
      _editEditItem.setActionCommand( "edit" ) ;
      
      _widthMenu     = new Menu( "Options" ) ;
      
      MenuItem item ;
            
      _widthMenu.add( item = new MenuItem( "AnimationUp" ) );
      item.addActionListener( this ) ;
      item.setActionCommand( "animationUp" ) ;
      _widthMenu.add( item = new MenuItem( "AnimationDown" ) );
      item.addActionListener( this ) ;
      item.setActionCommand( "animationDown" ) ;
      _widthMenu.add( item = new MenuItem( "AnimationInfinit" ) );
      item.addActionListener( this ) ;
      item.setActionCommand( "animationInfinit" ) ;
      _widthMenu.add( item = new MenuItem( "Worksheet" ) );
      item.addActionListener( this ) ;
      item.setActionCommand( "worksheet" ) ;
      _widthMenu.add( item = new MenuItem( "Host Menu" ) );
      item.addActionListener( this ) ;
      item.setActionCommand( "hosts" ) ;
      
      _menuBar.add( _fileMenu ) ;
//      _menuBar.add( _editMenu ) ;
      _menuBar.add( _widthMenu ) ;
      setMenuBar( _menuBar ) ;
    
 }
 public void actionPerformed( ActionEvent event ){
      Object source = event.getSource() ;
      String s      = event.getActionCommand() ;
      
      if( source == _textArea ){
        say( s ) ;
        String y = _command( s ) ;
        _textArea.answer( y ) ;
        
      }
      if( s.equals("exit") )System.exit(0);
      else if( s.equals("animationUp" ) ){
         _logoCanvas.animation( LogoCanvas.GROWING ) ;
         _cardsLayout.show( _cards , "logo" ) ;
      }else if( s.equals("animationDown" ) ){
         _logoCanvas.animation( LogoCanvas.SHRINKING ) ;
         _cardsLayout.show( _cards , "logo" ) ;
      }else if( s.equals("animationInfinit" ) ){
         _logoCanvas.animation( LogoCanvas.INFINIT ) ;
         _cardsLayout.show( _cards , "logo" ) ;
      }else if( s.equals("worksheet" ) ){
         _cardsLayout.show( _cards , "worksheet" ) ;
      }else{
         say( " Unknown action Command : " + s ) ;
         _cardsLayout.show( _cards , "worksheet" ) ;
      }
      return ;
 }
 private String _command( String in ){
   return "Hallo this is line 1\nand this is line 2\n" ;
 }
 private void say( String text ){
       System.out.println( text ) ;
 }
 public static void main( String [] args ){

      new workTest( args ) ;
    
 }
 
 
}
