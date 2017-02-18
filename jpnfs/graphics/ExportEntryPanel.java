package  jpnfs.graphics ;

import   jpnfs.pnfs.* ;

import java.awt.* ;
import java.awt.event.* ;
import java.util.* ;
import java.io.* ;

public class      ExportEntryPanel 
       extends    Panel 
       implements PnfsListener, ActionListener, ItemListener {   
 
  private   PnfsExportEntry  _exportEntry  = null ;
  private   PnfsListener     _pnfsListener = null ;
  private List        _mountpointList ;
  private List        _hostnameList ;
  private TextField   _hostIpText ;
  private TextField   _hostNameText ;
  private MountpointPanel _mountpointPanel ;
  
  public ExportEntryPanel(){

  }
  public void setPnfsListener( PnfsListener al ){
    _pnfsListener = al ;
    return ;
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
