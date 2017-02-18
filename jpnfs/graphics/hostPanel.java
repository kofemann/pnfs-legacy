package  jpnfs.graphics ;

import   jpnfs.pnfs.* ;

import java.awt.* ;
import java.awt.event.* ;
import java.util.* ;
import java.io.* ;
import java.net.* ;

public class      hostPanel 
       extends    Panel 
       implements ActionListener, ItemListener {   

  private ActionListener  _actionListener = null ;
  private List            _mountpointList ;
  private mountpointPanel _mountPanel ;
  private pnfsMountgroup  _mountgroup ;
  private TextField       _nameText , _ipText , _trustText ;
  private Label           _messages ;   
  private Button          _saveButton , _addButton , _rmButton ;   
  private String          _exportPath = null ;
  private Hashtable       _currentMountpoints = null ;
  private boolean         _mountpointsChanged = false ;
                
  public hostPanel( String exportPath , pnfsMountgroup mg ){
     _mountgroup = mg ;
     _exportPath = exportPath ;
     
     setLayout( new GridBagLayout( ) ) ;
     setBackground( Color.red ) ;
     
     _saveButton  = new Button( "Save Hostinfo" ) ;
     _addButton   = new Button( "Add Mountpoint" ) ;
     _rmButton    = new Button( "Remove Mountpoint" ) ;

     _nameText  = new TextField(30) ;
     _ipText    = new TextField(30) ;
     _trustText = new TextField(4) ;
     
     _ipText.setEditable( false ) ;
     
     _messages       = new Label("Messsages .... ") ;
     _messages.setBackground( Color.green ) ;
     _mountpointList = new List(5) ;
     
     _mountPanel     = new mountpointPanel();
     
     _mountpointList.addItemListener( this ) ;
     _saveButton.addActionListener( this ) ;
     _addButton.addActionListener( this ) ;
     _rmButton.addActionListener( this ) ;
     _nameText.addActionListener( this ) ;
     _mountPanel.setActionListener( this ) ;
     _trustText.addActionListener( this ) ;
     
     Panel button = new Panel( new FlowLayout() ) ;
     button.add( _addButton ) ;
     button.add( _rmButton) ;
     button.add( _saveButton );
     
     Label nameLabel  = new Label( "Host Name" ) ;
     Label ipLabel    = new Label( "Host IP Number" ) ;
     Label trustLabel = new Label( "Host Trustness" ) ;
     
//     Panel host = new Panel( new GridLayout( 0 , 2 ) ) ;
//     host.add( nameLabel ) ;
//     host.add( _nameText ) ;
//     host.add( ipLabel ) ;
//     host.add( _ipText ) ;
//     host.add( trustLabel ) ;
//     host.add( _trustText ) ;
     
     addComponent(   nameLabel   , 0 , 0 , 1 , 1   ) ;
     addComponent(  _nameText    , 1 , 0 , 2 , 1   ) ;
     addComponent(   ipLabel     , 0 , 1 , 1 , 1   ) ;
     addComponent(  _ipText      , 1 , 1 , 2 , 1   ) ;
     addComponent(   trustLabel  , 0 , 2 , 1 , 1   ) ;
     addComponent(  _trustText   , 1 , 2 , 2 , 1   ) ;
     
     addComponent(  _mountpointList   , 3 , 0 , 1 , 3  , true  ) ;    
     addComponent(  _mountPanel       , 4 , 0 , 2 , 3   ) ; 
         
     addComponent(  _messages        , 0 , 3 , 4 , 1  , true  ) ;
     addComponent(  button           , 4 , 3 , 2 , 1   ) ;
  
  }
  public void itemStateChanged( ItemEvent event ){
      say("");
      Object source = event.getSource() ;
      if( source == _mountpointList ){
        String         item = _mountpointList.getSelectedItem() ;
        pnfsMountpoint mp   = (pnfsMountpoint)_currentMountpoints.get( item ) ;
        _mountPanel.setMountpoint( mp ) ;
         
      }
  }
  public void actionPerformed( ActionEvent event ){
      say("");
      Object source = event.getSource() ;
      if( source == _nameText ){
         _hostKlicked() ;
      }else if( source == _mountPanel ){
         pnfsMountpoint mp  = _mountPanel.getMountpoint() ;
         String         vmp = mp.getVirtualMountpoint() ;
         if( ( _currentMountpoints            != null ) &&
             ( _currentMountpoints.get( vmp ) != null )    ){
             
               _displayMountpoints( vmp ) ;
               return ;
         }
         int s = _mountpointList.getSelectedIndex() ;
         if( s > -1 )_mountpointList.deselect(s) ;
         if( ( mp = _mountgroup.get( vmp ) ) == null ){
            if( ! vmp.equals(""))say( "Virtual Mountpoint >"+vmp+"< not found" ) ;
            _mountPanel.clear() ;
            return ;
         }
         _mountPanel.setMountpoint( mp ) ;
      }else if( source == _addButton ){
         if( _ipText.getText().equals("") ){
           say( "Insufficient Host informations (no ip number)" ) ;
           return ;
         }
         pnfsMountpoint mp = _mountPanel.getMountpoint() ;
         String vmp = mp.getVirtualMountpoint() ;
         if( vmp.equals("") ||
             mp.getRealMountpoint().equals("")        ){
            say("Insufficient Mountpoint Informations" ) ;
            return;   
         }
         if( _currentMountpoints == null ){
           _currentMountpoints = new Hashtable() ;
         }else if( _currentMountpoints.get(vmp) != null ){
            say( "Virtual Mountpoint "+vmp+" already in mountlist" ) ;
            return ;
         }
         _currentMountpoints.put( vmp , mp ) ;
         _displayMountpoints( vmp ) ;
         _mountpointsChanged = true ;
      }else if( source == _rmButton ){
         if( _ipText.getText().equals("") ){
           say( "Insufficient Host informations (no ip number)" ) ;
           return ;
         }
         pnfsMountpoint mp = _mountPanel.getMountpoint() ;
         String vmp = mp.getVirtualMountpoint() ;
         if( vmp.equals("") ){
            say("Insufficient Mountpoint Informations" ) ;
            return;   
         }
//         if(  ! mp.getRealMountpoint().equals("") ){
//            say("To much Mountpoint Information" ) ;
//            return;   
//         }
         if( _currentMountpoints == null ){
           return  ;
         }else if( _currentMountpoints.get(vmp) == null ){
            say( "Virtual Mountpoint "+vmp+" no found in mountlist" ) ;
            return ;
         }
         _currentMountpoints.remove( vmp ) ;
         _displayMountpoints( vmp ) ;
         _mountpointsChanged = true ;
      }else if( source == _saveButton ){
         if( ( _currentMountpoints == null ) || 
             ( ! _mountpointsChanged       )    ){
             
            say( "Nothing changed" ) ;
            return ;   
         }
         if( _ipText.getText().equals("") ||
             _nameText.getText().equals("")  ){
           say( "Insufficient Host informations" ) ;
           return ;
         }
         _store() ;
         _mountpointsChanged  = false ;
      }else if( source == _trustText ){
         _mountpointsChanged  = true ;
      }
      

  }
  private void _store(){
     String ip = _ipText.getText() ;
     
     try{
       File mounts = new File( _exportPath , ip ) ;
       PrintWriter out = new PrintWriter(
                          new BufferedWriter( 
                           new FileWriter( mounts ) ) );
        
        for( Enumeration e = _currentMountpoints.elements() ;
              e.hasMoreElements() ; ){
            out.println( e.nextElement().toString() ) ;
        }
        out.close() ;
        String trustDir = _exportPath +"/"+ "trusted" ;
        File trust = new File( trustDir , ip ) ;
        int tl    = _getTrustLevel2( trust ) ;
        int newTl = 0 ;
        try{
          newTl = new Integer( _trustText.getText() ).intValue() ;
        }catch( Exception ie ){
          newTl = 0 ;
        } 
        if(  ( ( newTl == 0 ) && ( tl <= 0 ) ) || 
             ( newTl == tl )  ){
            say( "Mountpoints Updated : "+ ip ) ;
            return ;
        }
        out = new PrintWriter(
                           new BufferedWriter( 
                            new FileWriter( trust ) ) );
        out.println( ""+newTl ) ;
        out.close();
        say( "Mountpoints and Trustness Updated : "+ ip  ) ;
     }catch( Exception e ){
       say( "Problem : "+e ) ;
       return ;
     }                  
  
  }
  private void _hostKlicked(){
         String hostname = _nameText.getText() ;
         if( hostname.equals("") ){
           say( "No Hostname specified" ) ;
           _clearMountinfos() ;
           return ;
         }
         InetAddress addr = null ;
         String hostAddress = null ;
         try{
           addr = InetAddress.getByName(hostname);
           hostAddress = addr.getHostAddress() ;
           say( " Hostname : "+addr.getHostName()+
                " ("+addr.getHostAddress()+")" ) ;
           _nameText.setText( addr.getHostName() ) ;
           _ipText.setText( hostAddress ) ;
         }catch( UnknownHostException uhe ){
            say( "Hostname "+hostname+" not found" ) ;
           _ipText.setText( "" ) ;
           _clearMountinfos() ;
            return ;
         }
         File name    = new File( _exportPath+"/"+hostAddress ) ;
         File trusted = new File( _exportPath+"/trusted/"+hostAddress ) ;
         if( ! name.exists() ){
            say( " Address "+hostAddress+" not found in export list" ) ;
           _clearMountinfos() ;
            return;
         }
         _currentMountpoints = _getMountpoints( name ) ;
         _displayMountpoints() ;
         _trustText.setText( ""+_getTrustLevel( trusted ));
         return ;  
  }
  private void _displayMountpoints(){ _displayMountpoints(null) ; }
  private void _displayMountpoints( String vmp ){
     if(_mountpointList.getItemCount()>0)_mountpointList.removeAll() ;
     _mountPanel.clear() ;
     if( _currentMountpoints == null )return ;
     String cvmp ;
     int i = 0 , j = 0 ;
     for( Enumeration e = _currentMountpoints.keys() ;
          e.hasMoreElements() ; i++ ){
          
         cvmp = (String)e.nextElement() ;
        _mountpointList.add( cvmp ) ;
        if( ( vmp != null ) && vmp.equals( cvmp ) )j=i;    
     }
     if( i > 0 ){
        _mountpointList.select(j) ;
        _mountpointList.makeVisible(j) ;
        String         item = _mountpointList.getSelectedItem() ;
        pnfsMountpoint mp   = (pnfsMountpoint)_currentMountpoints.get( item ) ;
        _mountPanel.setMountpoint( mp ) ;
     }
     
  }
  private void _clearMountinfos(){
     _trustText.setText("");
     if(_mountpointList.getItemCount()>0)_mountpointList.removeAll() ;
     _mountPanel.clear() ;
     _currentMountpoints = null ;
     _mountpointsChanged = false ;
  }
  private int _getTrustLevel( File f ){
    if( ! f.exists() )return 0 ;
    try{
       BufferedReader in = new BufferedReader( new FileReader( f ) ) ; 
       String line       = in.readLine() ;
       in.close() ;
       return new Integer( line ).intValue() ;
    }catch( Exception e ){
       return 0 ;
    }
     
  }
  private int _getTrustLevel2( File f ){
    if( ! f.exists() )return -1 ;
    try{
       BufferedReader in = new BufferedReader( new FileReader( f ) ) ; 
       String line       = in.readLine() ;
       in.close() ;
       return new Integer( line ).intValue() ;
    }catch( Exception e ){
       return -1 ;
    }
     
  }
  private Hashtable _getMountpoints( File f ){
    Hashtable       vec = new Hashtable() ;
    BufferedReader  in  = null ;
    StringTokenizer st  = null ;
    String          vmp , rmp , line ;
    pnfsMountpoint  mp ;
    pnfsMountpoint [] mps ;
    int             flags ;
    try{
       in = new BufferedReader( new FileReader( f ) ) ; 
            
       for( int j = 0 ; ( line = in.readLine() ) != null ; ){
               try { mp  = new pnfsMountpoint( line ) ; }
               catch( IllegalArgumentException iae ){ continue ; }
               vec.put( mp.getVirtualMountpoint() , mp ) ;
               j++ ;
       }
       in.close() ;
       return vec ;
    }catch( Exception e ){
       return vec ;
    }
  }
  public void setActionListener( ActionListener al ){
     _actionListener = al ;
     return ;
  }
  public Insets getInsets(){ return new Insets( 10 , 10 , 10 , 10 ) ; }
  private void say( String str ){
     _messages.setBackground( str.equals("") ? Color.red : Color.green ) ;
     _messages.setText( str ) ; 
//     System.out.println( str ) ;
  }
  private void addComponent( Component component ,
                              int gridx , int gridy ,
			      int gridwidth , int gridheight ) {
    addComponent(component,gridx,gridy,gridwidth,gridheight,false);
  }
  private void addComponent( Component component ,
                              int gridx , int gridy ,
			      int gridwidth , int gridheight , boolean fill ) {
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
       if( fill )
          gbc.fill       = GridBagConstraints.BOTH ;
       else
          gbc.fill       = GridBagConstraints.NONE ;
          
       gbc.anchor     = GridBagConstraints.CENTER ;
       ((GridBagLayout)lm).setConstraints( component , gbc ) ;
       add( component ) ;
       		  
  }

}
