package  jpnfs.graphics ;
import   jpnfs.pnfs.* ;

import java.awt.* ;
import java.awt.event.* ;
import java.util.* ;
import java.io.* ;

public class      MountpointPanel 
       extends    Panel 
       implements ActionListener {   

  private TextField      _virtualMp , _realMp , _level ;
  private Checkbox       _ioEnabled ;
  private PnfsListener   _pnfsListener = null ;
  
  public MountpointPanel(){
  
     setLayout( new GridBagLayout( ) ) ;
//     setBackground( Color.orange ) ;
     Label virtLabel  = new Label( "Virtual Mountpoint" ) ;
     Label readLabel  = new Label( "Real Mountpoint" ) ;
     Label levelLabel = new Label( "Level" , Label.RIGHT ) ;

     _virtualMp  = new TextField(30) ;
     _realMp     = new TextField(30) ;
     _level      = new TextField(4) ;
     _ioEnabled  = new Checkbox( "I/O Enabled" ) ;
     
     _virtualMp.addActionListener( this ) ;
     
     addComponent(  virtLabel  , 0 , 0 , 1 , 1   ) ;
     addComponent( _virtualMp  , 1 , 0 , 2 , 1   ) ;     
     addComponent( readLabel   , 0 , 1 , 1 , 1   ) ;
     addComponent( _realMp     , 1 , 1 , 2 , 1   ) ;
     addComponent( _ioEnabled  , 0 , 2 , 1 , 1   ) ;
     addComponent( levelLabel  , 1 , 2 , 1 , 1   ) ;
     addComponent( _level      , 2 , 2 , 1 , 1   ) ;

  
  }
  public String  getVirtualMountpoint(){ return _virtualMp.getText() ; }
  public String  getRealMountpoint(){ return _realMp.getText() ; }
  public boolean isIoEnabled(){ return _ioEnabled.getState() ; }
  public void    clear(){
       _virtualMp.setText( "" ) ;
       _realMp.setText( "" ) ;
       _realMp.setText( "" ) ;
       _ioEnabled.setState( false ) ;
  }
  public void    setMountpoint( PnfsMountpoint mp ){
       _virtualMp.setText( mp.getVirtualMountpoint() ) ;
       _realMp.setText( mp.getRealMountpoint() ) ;
       _ioEnabled.setState( mp.isIoEnabled() ) ;
       _level.setText( ""+mp.getLevel() ) ;
  }
  public PnfsMountpoint getMountpoint(){ 
    String l = _level.getText() ;
    int level = 0 ;
    try{
       level = new Integer( l ).intValue() ;
    }catch( Exception e ){
       level = 0 ;
    }
    return new PnfsMountpoint( _virtualMp.getText() ,
                               _realMp.getText() ,
                               _ioEnabled.getState() ,
                               level
                               ) ;
  } 
  public void actionPerformed( ActionEvent event ){
      Object source = event.getSource() ;
      if( source == _virtualMp ){
         if( _pnfsListener != null ){
            _pnfsListener.pnfsAction( 
                  new PnfsEvent( this , "findMountpoint" ) ) ;         
         }
      }

  }
  public void setPnfsListener( PnfsListener al ){
     _pnfsListener = al ;
     return ;
  }
  public Insets getInsets(){ return new Insets( 10 , 10 , 10 , 10 ) ; }
  
  private void addComponent( Component component ,
                              int gridx , int gridy ,
			      int gridwidth , int gridheight ) {
       LayoutManager lm = getLayout() ;
       if( ! ( lm instanceof GridBagLayout  ) ){
          System.out.println( " Not a GridBagLayout " ) ;
          return ;
       }
       GridBagConstraints gbc = new GridBagConstraints() ;
       gbc.gridx      = gridx ;
       gbc.gridy      = gridy ;
       gbc.gridwidth  = gridwidth ;
       gbc.gridheight = gridheight ;
//       gbc.fill       = GridBagConstraints.BOTH ;
       gbc.fill       = GridBagConstraints.NONE ;
       gbc.anchor     = GridBagConstraints.CENTER ;
       ((GridBagLayout)lm).setConstraints( component , gbc ) ;
       add( component ) ;
       		  
  }

}
