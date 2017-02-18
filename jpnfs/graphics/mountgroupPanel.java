package  jpnfs.graphics ;

import   jpnfs.pnfs.* ;

import java.awt.* ;
import java.awt.event.* ;
import java.util.* ;
import java.io.* ;

public class      mountgroupPanel 
       extends    Panel 
       implements ActionListener, ItemListener {   

  private ActionListener  _actionListener = null ;
  private List            _mountgroupList  ;
  private List            _mountpointList ;
  private mountpointPanel _mountPanel ;
  private pnfsMountgroup  _mountgroup ;
  private Button          _addMpButton , _addGpButton , 
                          _rmMpButton  , _rmGpButton ;
  private TextField       _newGroupText ;
  private Label           _messages ;   
     
                          
  public mountgroupPanel( pnfsMountgroup mg ){
     _mountgroup = mg ;
     
     setLayout( new GridBagLayout( ) ) ;
     setBackground( Color.red ) ;
     
     _addMpButton = new Button( "Add Mountpoint" ) ;
     _addGpButton = new Button( "Add Mountgroup" ) ;
     _rmMpButton  = new Button( "Remove Mountpoint" ) ;
     _rmGpButton  = new Button( "Remove Mountgroup" ) ;

     _newGroupText  = new TextField(30) ;
     _messages      = new Label("Messsages .... ") ;

     _mountgroupList = new List(5) ;
     _mountpointList = new List(5) ;
     
     _mountPanel  = new mountpointPanel();
     
     _mountgroupList.addItemListener( this ) ;
     _mountpointList.addItemListener( this ) ;
     _addMpButton.addActionListener( this ) ;
     _addGpButton.addActionListener( this ) ;
     _rmMpButton.addActionListener( this ) ;
     _rmGpButton.addActionListener( this ) ;
     _newGroupText.addActionListener( this ) ;
     _mountPanel.setActionListener( this ) ;
     
     
     Label mgroups = new Label( "Mount Groups" ) ;
     Label mpoints = new Label( "Mount Points" ) ;
     Label mpoint  = new Label( "Mount Point" ) ;
     
     Panel buttons = new Panel( new FlowLayout() ) ;
     buttons.add( _addMpButton ) ;
     buttons.add( _addGpButton ) ;
     buttons.add( _rmMpButton ) ;
     buttons.add( _rmGpButton ) ;
     buttons.add( _newGroupText ) ;
     
     addComponent(  mgroups  , 0 , 0 , 1 , 1   ) ;
     addComponent(  mpoints  , 1 , 0 , 1 , 1   ) ;
     addComponent(  mpoint   , 2 , 0 , 1 , 1   ) ;
     
     addComponent(  _mountgroupList  , 0 , 1 , 1 , 3   ) ;
     addComponent(  _mountpointList  , 1 , 1 , 1 , 3   ) ;
     addComponent(  _mountPanel      , 2 , 1 , 1 , 3   ) ;
     
     addComponent(  buttons        , 0 , 4 , 3 , 1   ) ;
     addComponent(  _messages      , 0 , 5 , 3 , 1   ) ;
     
     _showMountgroups() ;
  
  }
  public void itemStateChanged( ItemEvent event ){
      say("");
      Object source = event.getSource() ;
      if( source == _mountgroupList ){
        _showGroup( _mountgroupList.getSelectedItem() ) ;
      }else if( source == _mountpointList ){
         String vmp = _mountpointList.getSelectedItem() ;
         pnfsMountpoint mp = 
           _mountgroup.get( _mountgroupList.getSelectedItem() , vmp ) ;
         _mountPanel.setMountpoint( mp ) ;  
                       
      }
  }
  public void actionPerformed( ActionEvent event ){
      say("");
      Object source = event.getSource() ;
      if( source == _addMpButton ){
         pnfsMountpoint mp = _mountPanel.getMountpoint();
         if( mp.getVirtualMountpoint().equals("") ||
             mp.getRealMountpoint().equals("")       ){
             say("Mountpoint not fully specified" ) ;
             return ;
         }
         String groupName = _mountgroupList.getSelectedItem() ;
         if( ( groupName == null ) || groupName.equals("") ){
            say( "Select group name" ) ;
            return ;
         }
         _mountgroup.add( groupName , mp ) ;
         _showGroup( groupName ) ;
         _mountPanel.setMountpoint( mp ) ;
      }else if( ( source == _addGpButton  ) ||
                ( source == _newGroupText )     ){
         String newGroup = _newGroupText.getText() ;
         if( newGroup.equals("") ){ 
            say( "No group name specified" ) ;
            return ;
         }
         try{
           _mountgroup.add( newGroup ) ;
           _mountgroupList.add( newGroup ) ;
           return ;
         }catch( IllegalArgumentException iae ){
            say( "Duplicated group name" ) ;
            return; 
         }
      }else if( source == _rmMpButton ){
         String groupName = _mountgroupList.getSelectedItem() ;
         String vmp       = _mountpointList.getSelectedItem() ;
         if( ( groupName == null ) ||
             ( vmp       == null ) ||
             groupName.equals("")  || 
             vmp.equals("")             ){
            say( "Select group and mountpoint" ) ;
            return ;
         }
         _mountgroup.removeMountpoint( groupName , vmp ) ;
         _showGroup( groupName ) ;
      }else if( source == _rmGpButton ){
         String groupName = _mountgroupList.getSelectedItem() ;
         if( ( groupName == null ) ||
             groupName.equals("")      ){
            say( "Select group" ) ;
            return ;
         }
         _mountgroup.removeMountgroup( groupName ) ;
         _showMountgroups() ;
      }else if( source == _mountPanel ){
         String vmp = _mountPanel.getMountpoint().getVirtualMountpoint() ;
         if( vmp.equals("") )return ;
         pnfsMountpoint mp = _mountgroup.get( vmp ) ;
         if( mp == null ){
            say( "Virtual Mountpoint "+vmp+" not found" ) ;
            return ;
         }
         String groupName = _mountgroup.getGroup( vmp ) ;
         say( " mountgroup  : "+groupName ) ;
         selectItem( groupName , _mountgroupList ) ;
         _showGroup( groupName ) ;
         selectItem( vmp , _mountpointList ) ;         
         _mountPanel.setMountpoint( mp ) ;
      }

  }
  private int selectItem( String item , List list ){
     String [] il = list.getItems() ;
     for( int i = 0 ; i < il.length ; i++ ){
         if( item.equals( il[i] ) ){
             list.select(i) ;
             list.makeVisible(i);
           return i ;
         }
     }
     return -1 ;
  }
  public void setActionListener( ActionListener al ){
     _actionListener = al ;
     return ;
  }
  public Insets getInsets(){ return new Insets( 10 , 10 , 10 , 10 ) ; }
  private void _showMountgroups(){
     String groupName ;
     Enumeration e = _mountgroup.groups() ;
     if(_mountgroupList.getItemCount()>0)_mountgroupList.removeAll() ;
     int i ;
     for(i = 0 ; e.hasMoreElements() ; i ++ ){
        groupName = (String)e.nextElement() ;
        _mountgroupList.add( groupName ) ;
     }
     if( i > 0 ){
        _mountgroupList.select(0) ;
        _mountgroupList.makeVisible(0);
        _showGroup( _mountgroupList.getSelectedItem() ) ; 
     }else{
        if(_mountpointList.getItemCount()>0)_mountpointList.removeAll() ;
        _mountPanel.setMountpoint(new pnfsMountpoint("","",false));
     }    
     
  }
  private void _showGroup( String group ){
     if(_mountpointList.getItemCount()>0)_mountpointList.removeAll() ;
     Enumeration e = _mountgroup.mountpoints( group ) ;
     pnfsMountpoint mp ;
     int i ;
     for( i = 0  ; e.hasMoreElements() ; i ++ ){
       mp = (pnfsMountpoint)e.nextElement() ;
       _mountpointList.add( mp.getVirtualMountpoint() ) ;
       if( i == 0 ){
          _mountPanel.setMountpoint( mp ) ; 
       }    
     }
     if( i > 0 ){
        _mountpointList.select(0) ;
        _mountpointList.makeVisible(0);
     }else{
        _mountPanel.setMountpoint(new pnfsMountpoint("","",false));
     }
  }
  private void _clearList( List list ){
     int ind = list.getSelectedIndex() ;
     if( ind > -1 )list.deselect( ind ) ;
     return ;
  }
  private void say( String str ){ _messages.setText( str ) ; }
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
       gbc.fill       = GridBagConstraints.BOTH ;
       gbc.anchor     = GridBagConstraints.CENTER ;
       ((GridBagLayout)lm).setConstraints( component , gbc ) ;
       add( component ) ;
       		  
  }

}
