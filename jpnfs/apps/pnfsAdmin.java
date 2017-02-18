package  jpnfs.apps ;

import jpnfs.pnfs.* ;
import jpnfs.graphics.* ;

import java.awt.* ;
import java.awt.event.* ;
import java.util.* ;
   
import java.io.* ;
   
 public class      pnfsAdmin 
        extends    Frame 
        implements ActionListener {
                               
    MenuBar         _menuBar ;
    Menu            _editMenu , _fileMenu , _widthMenu ;
    MenuItem        _editEditItem ;
    MenuItem        _fileExitItem ;
    HelloCanvas     _helloCanvas ;
    Label           _textField ;
    Panel           _cards ;
    CardLayout      _cardsLayout ;
    mountgroupPanel _mountgroupPanel ;
    hostPanel       _hostPanel ;
    String          _exportPath = null ;
    
    public pnfsAdmin( String [] args ){
      
      setLayout( new BorderLayout() ) ;      
             
      add( _textField    = new Label() , "North" ) ;
      add( _cards        = new Panel() , "Center" ) ;
      
      _textField.setText( "Pnfs Mountpoint Administration Tool" ) ;
      
      _helloCanvas     = new HelloCanvas("pnfsAdmin") ;
      setTitle( "pnfsAdmin" ) ;
      pnfsMountgroup group ; 
      if( args.length > 0 ){
         _exportPath = args[0] ;
         group = new pnfsMountgroup(_exportPath+"/mountpoints")  ;
      }else{
         _exportPath = "." ;
         group = simMountgroup() ;
      }
      _mountgroupPanel = new mountgroupPanel( group ) ;
      _mountgroupPanel.setActionListener( this ) ;
      
      _hostPanel = new hostPanel( _exportPath , group ) ;
      
      _cards.setLayout( _cardsLayout = new CardLayout() ) ;
      _cards.add( _helloCanvas      , "hello" ) ;
      _cards.add( _mountgroupPanel  , "mountGroup" ) ;
      _cards.add( _hostPanel        , "hosts" ) ;

//      _cardsLayout.show( _cards , "mountGroup" ) ;
      _cardsLayout.show( _cards , "hello" ) ;
       
      installMenu();
      
      
//      setSize( 600 , 400 );      
      pack();
//      setSize( 600 , 400 );
      show();
       
 }
 private pnfsMountgroup simMountgroup(){
    pnfsMountgroup mg = new pnfsMountgroup() ;
    mg.add( "Generic", new pnfsMountpoint( "/desy" , "/0/root/usr/desy" , false ) );
    mg.add( "Generic", new pnfsMountpoint( "/desy" , "/0/root/usr/desy/mc" , false ) );
    mg.add( "Zeus"   , new pnfsMountpoint( "/zeus" , "/0/root/usr/zeus" , false ) );
    mg.add( "Zeus"   , new pnfsMountpoint( "/zeus1" , "/0/root/usr/zeus1" , false ) );
    mg.add( "Zeus"   , new pnfsMountpoint( "/zeus2" , "/0/root/usr/zeus2" , false ) );
    mg.add( "Zeus"   , new pnfsMountpoint( "/zeus3" , "/0/root/usr/zeus3" , false ) );
    mg.add( "H1"     , new pnfsMountpoint( "/h1" , "/0/root/usr/h1" , false ) );
    mg.add( "Hermes" , new pnfsMountpoint( "/hermes" , "/0/root/usr/hermes" , false ) );
    return mg ;
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
            
      _widthMenu.add( item = new MenuItem( "Hello Menu" ) );
      item.addActionListener( this ) ;
      item.setActionCommand( "hello" ) ;
      _widthMenu.add( item = new MenuItem( "Mountpoint Menu" ) );
      item.addActionListener( this ) ;
      item.setActionCommand( "mountpoints" ) ;
      _widthMenu.add( item = new MenuItem( "Host Menu" ) );
      item.addActionListener( this ) ;
      item.setActionCommand( "hosts" ) ;
      
      _menuBar.add( _fileMenu ) ;
//      _menuBar.add( _editMenu ) ;
      _menuBar.add( _widthMenu ) ;
      setMenuBar( _menuBar ) ;
    
 }
 public void actionPerformed( ActionEvent event ){
 //     System.out.println( " Action with comment "+event.getActionCommand());
      String c = event.getActionCommand() ;
      if( c.equals("exit") )System.exit(0);
      
      if(  c.equals("mountpoints") ){        
            _cardsLayout.show( _cards , "mountGroup" ) ;
      }else if( c.equals( "hello" ) ){
            _cardsLayout.show( _cards , "hello" ) ;
      }else if( c.equals( "hosts" ) ){
            _cardsLayout.show( _cards , "hosts" ) ;
      }
   
      Object source = event.getSource() ;
      if(  source == _mountgroupPanel ){
//         mountpointPanel mpp = (mountpointPanel)source ;
//         System.out.println( " Virtual : "+mpp.getVirtualMountpoint() ) ;
//         System.out.println( " Real    : "+mpp.getRealMountpoint() ) ;
//         System.out.println( " I/O En  : "+mpp.isIoEnabled() ) ;
      }
 }
 private void say( String text ){
       System.out.println( text ) ;
       _textField.setText( text ) ;
 }
 public static void main( String [] args ){

      new pnfsAdmin( args ) ;
    
 }
 
 
}
