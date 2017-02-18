package  jpnfs.graphics ;

import   jpnfs.pnfs.* ;

import java.awt.* ;
import java.awt.event.* ;
import java.util.* ;
import java.io.* ;

public class      pnfsMountPanel 
       extends    Panel 
       implements ActionListener {   
 
  private Button _rmGroup , _addGroup , _rmMountpoint , _addMountpoint ;
  private Choice _groupChoice ;
  private List   _mountpointList ;
  private TextField _virtualMp , _realMp , _newGroup ;
  private Label     _messages ;
  private Checkbox  _ioEnabled ;
  private Hashtable _mountHash = new Hashtable() ;
  private Font      _font ;
  public pnfsMountPanel( String mountpoint ){
//     setLayout( new GridLayout( 0 , 1 ) ) ;      
     setLayout( new BorderLayout( ) ) ;      
     setBackground( Color.white ) ;
     _rmGroup = new Button("Remove Group" ) ;
     _rmGroup.addActionListener( this ) ;
     _rmGroup.setActionCommand( "removegroup" ) ;
     
     _addGroup = new Button( "Add Group" ) ;
     _addGroup.addActionListener( this ) ;
     _addGroup.setActionCommand( "addgroup" ) ;
     
     _rmMountpoint = new Button( "Remove Mountpoint" ) ;
     _rmMountpoint.addActionListener( this ) ;
     _rmMountpoint.setActionCommand( "removemp" ) ;
     
     _addMountpoint = new Button( "Add Mountpoint" ) ;
     _addMountpoint.addActionListener( this ) ;
     _addMountpoint.setActionCommand( "addmp" ) ;
     
     _mountpointList = new List() ;
     _mountpointList.addActionListener( this ) ;
     _groupChoice = new Choice() ;
     _virtualMp = new TextField(40) ;
     _realMp    = new TextField(40) ;
     _newGroup  = new TextField("New Group") ;

     _messages  = new Label("Messages" ) ;
     _messages.setForeground( Color.red ) ;
     _ioEnabled  = new Checkbox( "I/O Enabled" ) ;
     
    
     Panel mpPanel = new Panel() ;
     mpPanel.setLayout( new GridBagLayout( ) ) ;
     mpPanel.setBackground( Color.orange ) ;
     
     addComponent( mpPanel , new Label( "Virtual Mountpoint" ) ,
                   0 , 0 , 1 , 1   ) ;
     addComponent( mpPanel , _virtualMp ,
                   1 , 0 , 2 , 1   ) ;
     _virtualMp.addActionListener( this ) ;
     
     addComponent( mpPanel , new Label( "Real Mountpoint" )  ,
                   0 , 1 , 1 , 1   ) ;
     addComponent( mpPanel , _realMp ,
                   1 , 1 , 2 , 1   ) ;

     addComponent( mpPanel , _ioEnabled ,
                   0 , 2 , 1 , 1   ) ;

     addComponent( mpPanel , _messages ,
                   0 , 3 , 3 , 1   ) ;

     addComponent( mpPanel , _addMountpoint ,
                   1 , 2 , 1 , 1   ) ;

     addComponent( mpPanel , _rmMountpoint ,
                   2 , 2 , 1 , 1   ) ;
     
     addComponent( mpPanel , _mountpointList ,
                   3 , 0 , 1 , 4   ) ;

     add( mpPanel , "Center" ) ;
     
     
     Panel groupPanel = new Panel( new GridLayout(1,0) ) ;
     
     groupPanel.add( new Label( "Current Group" ) ) ;
     groupPanel.add( _groupChoice ) ;
     groupPanel.add( new Label( "New Group" ) ) ;
     groupPanel.add( _newGroup ) ;
     groupPanel.add( _rmGroup ) ;
     groupPanel.add( _addGroup ) ;
     
     _font = groupPanel.getFont() ;
     
     
//     groupPanel.setBackground( Color.blue ) ;
//     groupPanel.setForeground(Color.white ) ;
     add( groupPanel , "North" ) ;

     _groupChoice.setFont( _font ) ;
     _virtualMp.setFont( _font ) ;
     _groupChoice.add( "Generic" ) ;

  }
  public Insets getInsets(){ return new Insets( 10 , 10 , 10 , 10 ) ; }
  public void actionPerformed( ActionEvent event ){
      System.out.println( " action : "+event.getActionCommand() ) ;
      Object source = event.getSource() ;
      _messages.setText("");
      if( source instanceof Button ){
         String command = event.getActionCommand() ;
         if( command.equals( "removegroup" ) ){
//            String text = _groupChoice.getSelectedItem();
//	    System.out.println( " Removing selected item : "+text ) ;
//            _groupChoice.remove( text ) ;
              int i = _groupChoice.getSelectedIndex() ;
              System.out.println( " Removing selected item : "+i ) ;
             _groupChoice.remove( i ) ;
         }else if( command.equals( "addgroup" ) ){
            String text = _newGroup.getText() ;
            if( text.equals("") ){
              _messages.setText("Specify Group Name in 'New Group Field'");
            }else{
              _groupChoice.setFont( _font ) ;
              _groupChoice.add( text ) ;
              _groupChoice.select( text ) ;
              _newGroup.setText("");
            }
         }else if( command.equals( "addmp" ) ){
            String vmp = _virtualMp.getText() ;
	    String rmp = _realMp.getText() ;
	    if( vmp.equals("") || rmp.equals("") ){
               _messages.setText( "No Real Mountpoint specified" ) ;
               return ;
            }
            if( _mountHash.get( vmp ) != null ){
               _messages.setText( "Mountpoint already exists" ) ;
               return ;
              
            }
            pnfsMountpoint mp = new pnfsMountpoint( vmp , rmp , 0 ) ;
            mp.setIoEnabled( _ioEnabled.getState() ) ;
	    _mountHash.put( vmp , mp ) ;
	    _mountpointList.add( vmp ) ;
            
	    
         }else if( command.equals( "removemp" ) ){
            String vmp = _virtualMp.getText() ;
	    if( vmp.equals("") ){
               _messages.setText( "No Mountpoint specified" ) ;
               return ;
            }
            if( _mountHash.get( vmp ) == null ){
               _messages.setText( "Mountpoint doesn't exists" ) ;
               return ;              
            }
            _mountpointList.remove( vmp ) ;
            _mountHash.remove( vmp ) ;
         }else if( command.equals( "vmpready" ) ){
         }
      
      }else if( source instanceof TextField ){
         if( source == _virtualMp ){
            pnfsMountpoint vmp = 
	          (pnfsMountpoint)_mountHash.get( _virtualMp.getText() ) ;
	    if( vmp == null ){
	       _realMp.setText( "") ;
	    }else{
	       _realMp.setText( vmp.getRealMountpoint() ) ;
               _ioEnabled.setState( vmp.isIoEnabled() ) ;
	    }
	 }
      
      }else if( source == _mountpointList ){
         String item = _mountpointList.getSelectedItem();
         if( item == null )return ;
            pnfsMountpoint vmp = 
	          (pnfsMountpoint)_mountHash.get( item ) ;
	    if( vmp == null ){
	       _realMp.setText( "") ;
	    }else{
	       _virtualMp.setText( vmp.getVirtualMountpoint() ) ;
	       _realMp.setText( vmp.getRealMountpoint() ) ;
               _ioEnabled.setState( vmp.isIoEnabled() ) ;
	    }
      }
  
   }
   private void addComponent( Container container , Component component ,
                              int gridx , int gridy ,
			      int gridwidth , int gridheight ) {
       LayoutManager lm = container.getLayout() ;
       if( ! ( lm instanceof GridBagLayout  ) ){
          System.out.println( " Not a GridBagLayout " ) ;
          return ;
       }
       GridBagConstraints gbc = new GridBagConstraints() ;
       gbc.gridx      = gridx ;
       gbc.gridy      = gridy ;
       gbc.gridwidth  = gridwidth ;
       gbc.gridheight = gridheight ;
       gbc.fill       = GridBagConstraints.BOTH ;
       gbc.anchor     = GridBagConstraints.CENTER ;
       ((GridBagLayout)lm).setConstraints( component , gbc ) ;
       container.add( component ) ;
       		  
   }
}
