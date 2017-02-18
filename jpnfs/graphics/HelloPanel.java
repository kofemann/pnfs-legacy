package  jpnfs.graphics ;

import jpnfs.pnfs.* ;
import jpnfs.util.* ;

import java.awt.* ;
import java.awt.event.* ;
import java.util.* ;


public class HelloPanel 
       extends TextField 
       implements SwitchBoardListener, ActionListener {

   private SwitchBoard _switchboard ;
   
   public HelloPanel( String name , int size  ){ 
      super( size ) ;
      _switchboard = new SwitchBoard( name , this ) ; 
      addActionListener( this ) ;
   }
   public HelloPanel( String name ){ 
      super(  ) ;
      _switchboard = new SwitchBoard( name , this ) ; 
      addActionListener( this ) ;
   }
   public void eventArrived( SwitchBoardEvent event ){
      setText( event.getMessage() ) ;
   }
   public void actionPerformed( ActionEvent event ){
      Args args = new Args( getText() ) ; 
      if( args.argc() < 2 )return ;
      _switchboard.sendMessage( args.argv(0) , args.argv(1) ) ;
   }


} 
