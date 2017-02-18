package  jpnfs.graphics ;
import   jpnfs.pnfs.* ;
import   jpnfs.util.* ;

import java.awt.* ;
import java.awt.event.* ;
import java.util.* ;
import java.io.* ;

public class      MountgroupDisplayPanel 
       extends    Panel 
       implements SwitchBoardListener, ItemListener {
       
   PnfsExportsDatabase  _pnfs ;   
   SwitchBoard          _switchboard ;
   Hashtable            _groups ;
   List                 _list ;
   Label                _label ;
   public MountgroupDisplayPanel( PnfsExportsDatabase database ){
      super( new BorderLayout() ) ;
      _pnfs        = database ;
      _switchboard = new SwitchBoard( "mountgroupPanel"  , this ) ;
      _list        = new List(5) ;
      _label       = new Label("Groups") ;
      _groups      = new Hashtable() ;
      add( _list , "Center" ) ;
      add( _label , "North" ) ;
      _list.addItemListener( this ) ;
   
   }    
   public void eventArrived( SwitchBoardEvent event ){
      Args args = new Args( event.getMessage() ) ; 
      if( args.argc() < 1 )return ;
      System.out.println( "MountgroupDispalyPanel : " + args.argv(0) ) ;
      if( args.argv(0).equals( "refresh" ) ){
          System.out.println( "MountgroupDispalyPanel : refresh" ) ;
          if( _list.getItemCount() > 0 )_list.removeAll() ;
          _groups.clear() ;
          Enumeration e = _pnfs.getGroupNames() ;
          for( int i = 0 ; e.hasMoreElements() ;i++ ){
             String str = (String)e.nextElement() ;
             _list.add( str ) ;
             _groups.put( str , new Integer(i) ) ;
          }
      }else if( args.argv(0).equals( "select" ) ){
         if( args.argc() < 2 )return ;
         Integer it = (Integer)_groups.get( args.argv(1) ) ;
         if( it == null ){
            _list.deselect( _list.getSelectedIndex() ) ;
         }else{
            _list.select( it.intValue() ) ;
         }
      }
   
   }
   public void itemStateChanged( ItemEvent event ){
      if( event.getStateChange() == ItemEvent.SELECTED ){
         ItemSelectable is = event.getItemSelectable() ;
         String [] strings = _list.getSelectedItems() ;
         if( strings.length < 1 )return ;
         System.out.println( "Selected : "+strings[0] ) ;
      }
   
   }
       
       
}   
 
 
