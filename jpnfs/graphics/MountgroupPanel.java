package  jpnfs.graphics ;

import   jpnfs.pnfs.* ;

import java.awt.* ;
import java.awt.event.* ;
import java.util.* ;
import java.io.* ;

public class      MountgroupPanel 
       extends    Panel 
       implements PnfsListener, ActionListener, ItemListener {   

  private ActionListener  _actionListener = null ;
  private List            _mountpointList ;
  private MountpointPanel _mountPanel ;
  private PnfsMountgroup  _mountgroup ;
  private Button          _addMpButton ,  _rmMpButton , 
                          _loadButton  ,  _saveButton ;
     
                          
  public MountgroupPanel( PnfsMountgroup mg ){
     _mountgroup = mg ;
     
     setLayout( new GridBagLayout( ) ) ;
//     setBackground( Color.blue ) ;
     
     _addMpButton = new Button( "Add Mountpoint" ) ;
     _rmMpButton  = new Button( "Remove Mountpoint" ) ;
     _saveButton  = new Button( "Save" ) ;
     _loadButton  = new Button( "Load" ) ;
     
     _mountpointList = new List(5) ;
     
     _mountPanel  = new MountpointPanel();
     
     _mountpointList.addItemListener( this ) ;
     _addMpButton.addActionListener( this ) ;
     _rmMpButton.addActionListener( this ) ;
     _saveButton.addActionListener( this ) ;
     _loadButton.addActionListener( this ) ;
     _mountPanel.setPnfsListener( this ) ;
     _saveButton.setForeground( Color.red ) ;
     
     Label mpoints = new Label( "Mount Points" , Label.CENTER ) ;
     Label mpoint  = new Label( "Mount Point"  , Label.CENTER) ;
     Label actions = new Label( "Actions"      , Label.CENTER ) ;
     
     Panel buttons = new Panel( new GridLayout(0,1) ) ;
     buttons.add( _addMpButton ) ;
     buttons.add( _rmMpButton ) ;
     buttons.add( _saveButton ) ;
     buttons.add( _loadButton ) ;
     
     addComponent(  mpoints  , 0 , 0 , 1 , 1   ) ;
     addComponent(  mpoint   , 1 , 0 , 2 , 1   ) ;
     addComponent(  actions  , 3 , 0 , 1 , 1   ) ;
     
     addComponent(  _mountpointList  , 0 , 1 , 1 , 3   ) ;
     addComponent(  _mountPanel      , 1 , 1 , 2 , 3   ) ;     
     addComponent(  buttons          , 3 , 1 , 1 , 3   ) ;
     
     _showMountpoints() ;
     _deactivateSaveButton() ;
  
  }
//  public void paint( Graphics g ){
//     Dimension d = getSize() ;
//     g.setColor( Color.blue ) ;
//     g.fillRect( 0 , 0 , d.width , d.height ) ;
//  }
  public void itemStateChanged( ItemEvent event ){
      Object source = event.getSource() ;
      if( source == _mountpointList ){
         String vmp = _mountpointList.getSelectedItem() ;
         PnfsMountpoint mp = _mountgroup.getMountpoint( vmp ) ;
         _mountPanel.setMountpoint( mp ) ;             
      }
  }
  public void pnfsAction( PnfsEvent event ){
      Object source = event.getSource() ;
      if( source == _mountPanel ){
         PnfsMountpoint mp = _mountPanel.getMountpoint();
         String vmp = mp.getVirtualMountpoint() ;
         if( vmp.equals("")  ){
//             say("Mountpoint not fully specified" ) ;
             return ;
         }
         mp = _mountgroup.getMountpoint( vmp ) ;
         if( mp == null )return ;
         selectItem( mp.getVirtualMountpoint() ) ;
         _mountPanel.setMountpoint( mp ) ;
      }
  }
  public void actionPerformed( ActionEvent event ){
      Object source = event.getSource() ;
      
      if( source == _addMpButton ){
         
         PnfsMountpoint mp = _mountPanel.getMountpoint();
         
         if( mp.getVirtualMountpoint().equals("") ||
             mp.getRealMountpoint().equals("")       ){
             say("Mountpoint not fully specified" ) ;
             return ;
         }
         _mountgroup.removeMountpoint( mp.getVirtualMountpoint() ) ;
         _mountgroup.addMountpoint( mp ) ;
         _showMountpoints();
         selectItem( mp.getVirtualMountpoint() ) ;
         _mountPanel.setMountpoint( mp ) ;
         _activateSaveButton() ;
      }else if( source == _rmMpButton ){
         String vmp       = _mountpointList.getSelectedItem() ;
         if( ( vmp       == null )  || vmp.equals("") ){
            say( "Select group and mountpoint" ) ;
            return ;
         }
         _mountgroup.removeMountpoint( vmp ) ;
         _showMountpoints() ;
         _activateSaveButton() ;
      }else if( source == _loadButton ){
         try{
            _mountgroup.load() ;
            _showMountpoints();
            _deactivateSaveButton() ;
         }catch( Exception se ){
            say( "exception while saving : "+se ) ;        
         }
      }else if( source == _saveButton ){
         try{
            _mountgroup.save() ;
            _deactivateSaveButton() ;
         }catch( Exception se ){
            say( "exception while saving : "+se ) ;        
         }
      }
      

  }
  private void _activateSaveButton(){ 
     _saveButton.setForeground( Color.red ) ;
  }
  private void _deactivateSaveButton(){ 
     _saveButton.setForeground( Color.black ) ;
  }
  private int selectItem( String item ){
    
     String [] il = _mountpointList.getItems() ;
     for( int i = 0 ; i < il.length ; i++ ){
         if( item.equals( il[i] ) ){
             _mountpointList.select(i) ;
             _mountpointList.makeVisible(i);
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
  
  private void _showMountpoints(){
     Enumeration e = _mountgroup.mountpoints() ;
     if(_mountpointList.getItemCount()>0)_mountpointList.removeAll() ;
     int i ;
     PnfsMountpoint first = null ;
     for(i = 0 ; e.hasMoreElements() ; i ++ ){
        PnfsMountpoint mp = (PnfsMountpoint)e.nextElement() ;
        if( i == 0 )first = mp ;
        _mountpointList.add( mp.getVirtualMountpoint() ) ;
     }
     if( i > 0 ){
        _mountpointList.select(0) ;
        _mountpointList.makeVisible(0);
        _mountPanel.setMountpoint( first ) ;
     }else
        _mountPanel.clear() ;  
     
  }
  /*
  private void _clearList( List list ){
     int ind = list.getSelectedIndex() ;
     if( ind > -1 )list.deselect( ind ) ;
     return ;
  }
  */
  private void say( String str ){ System.out.println( str )  ; }
  
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
