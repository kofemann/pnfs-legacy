package  jpnfs.graphics ;

import   jpnfs.pnfs.* ;

import java.awt.* ;
import java.awt.event.* ;
import java.util.* ;
import java.io.* ;

public class      mountpointPanel 
       extends    Panel 
       implements ActionListener {   

  private TextField      _virtualMp , _realMp ;
  private Checkbox       _ioEnabled ;
  private ActionListener _actionListener = null ;
  
  public mountpointPanel(){
  
     setLayout( new GridBagLayout( ) ) ;
     setBackground( Color.orange ) ;
     Label virtLabel = new Label( "Virtual Mountpoint" ) ;
     Label readLabel = new Label( "Real Mountpoint" ) ;

     _virtualMp  = new TextField(30) ;
     _realMp     = new TextField(30) ;
     _ioEnabled  = new Checkbox( "I/O Enabled" ) ;

     _virtualMp.addActionListener( this ) ;
     
     addComponent(  virtLabel  , 0 , 0 , 1 , 1   ) ;
     addComponent( _virtualMp  , 1 , 0 , 1 , 1   ) ;     
     addComponent( readLabel   , 0 , 1 , 1 , 1   ) ;
     addComponent( _realMp     , 1 , 1 , 1 , 1   ) ;
     addComponent( _ioEnabled  , 0 , 2 , 1 , 1   ) ;

  
  }
  public String  getVirtualMountpoint(){ return _virtualMp.getText() ; }
  public String  getRealMountpoint(){ return _realMp.getText() ; }
  public boolean isIoEnabled(){ return _ioEnabled.getState() ; }
  public void    clear(){
       _virtualMp.setText( "" ) ;
       _realMp.setText( "" ) ;
       _ioEnabled.setState( false ) ;
  }
  public void    setMountpoint( pnfsMountpoint mp ){
       _virtualMp.setText( mp.getVirtualMountpoint() ) ;
       _realMp.setText( mp.getRealMountpoint() ) ;
       _ioEnabled.setState( mp.isIoEnabled() ) ;
  }
  public pnfsMountpoint getMountpoint(){ 
    return new pnfsMountpoint( _virtualMp.getText() ,
                               _realMp.getText() ,
                               _ioEnabled.getState() ) ;
  } 
  public void actionPerformed( ActionEvent event ){
      Object source = event.getSource() ;
      if( source == _virtualMp ){
         if( _actionListener != null ){
            _actionListener.actionPerformed( 
                  new ActionEvent( this , 0 , "mountpointPanel" ) ) ;         
         }
      }

  }
  public void setActionListener( ActionListener al ){
     _actionListener = al ;
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
