package  jpnfs.apps ;

import jpnfs.pnfs.* ;
import jpnfs.graphics.* ;

import java.awt.* ;
import java.awt.event.* ;
import java.util.* ;
   
import java.io.* ;
   
 public class      Admin 
        extends    Frame 
        implements PnfsListener {
                               
    private CardPanel _cards  ;
    private     Label     _textField ;
    private     Canvas    _helloCanvas ;
    private     TreePanel _treePanel ;
    private     PnfsExportsDatabase _pnfsDatabase = null ;
    private     HelloPanel _helloPanel ;
    private     MountgroupDisplayPanel _mountgroupPanel ;
    public Admin( String [] args ){
      
      if( args.length < 1 ){
        System.out.println( " USAGE : ... <exportDir>" ) ;
        System.exit(0);
      }
      setTitle( "pnfsAdmin" ) ;
      setLayout( new BorderLayout() ) ;      
      
      _textField    = new Label("Pnfs Administration Tool",Label.CENTER) ;
      _cards        = new CardPanel() ;
      _helloCanvas  = new HelloCanvas("pnfsAdmin") ;
      _helloPanel   = new HelloPanel( "hello" ) ;
      add( _textField  , "North" ) ;
      add( _cards      , "Center" ) ;
      add( _helloPanel , "South" ) ;
      try{
//         MountgroupPanel mpPanel = new MountgroupPanel(
//                                   new PnfsMountgroup( 
//                                   new File(args[0]) , "Generic" ) ) ;

           _pnfsDatabase    = new PnfsExports( args[0] ) ;
           _treePanel       = new TreePanel( _pnfsDatabase ) ;
           _mountgroupPanel = new MountgroupDisplayPanel( _pnfsDatabase ) ;
           
//         mpPanel.setBackground( Color.red ) ;
//         mpPanel.setPnfsListener( this ) ;
//         _cards.add( mpPanel       , "MountPoint" ) ;

           _cards.add( _helloCanvas      , "Hello" );
           _cards.add( _treePanel        , "MountTree" ) ;
           _cards.add( _mountgroupPanel  , "Mountgroup" ) ;
      }catch( IOException ioe ){
         System.out.println( "Can't access "+args[0]+"/Generic" ) ;
         System.exit(4);
      }
      pack();
      show();
       
 }
 public void pnfsAction( PnfsEvent event ){
      Object source = event.getSource() ;
      if( source instanceof MountpointPanel ){
        // can only be a findMountpointEvent
        MountpointPanel mpp = (MountpointPanel)source;
        PnfsMountpoint mp   = mpp.getMountpoint() ;
        System.out.println( " find : "+mp ) ;
        String vmp = mp.getVirtualMountpoint() ;
        if( vmp.equals( "/admin" ) ){
          mp = new PnfsMountpoint( "/admin" , "/0/admin" , true , 1 );
        }else{
          mp = new PnfsMountpoint( vmp , "" , false , 0 );
        }
        mpp.setMountpoint( mp ) ;
        
      }
 }
 public static void main( String [] args ){

      new Admin( args ) ;
    
 }
 
 
}
