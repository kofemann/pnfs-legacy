package  jpnfs.graphics ;
import   jpnfs.pnfs.* ;
import   jpnfs.util.* ;

import java.awt.* ;
import java.awt.event.* ;
import java.util.* ;
import java.io.* ;

public class      TreePanel 
       extends    Canvas 
       implements SwitchBoardListener {
       
   PnfsExportsDatabase  _pnfs ;   
   SwitchBoard          _switchboard ;
   String       []      _tree = null ;
   Object               _treeLock = new Object() ;
   
   public TreePanel( PnfsExportsDatabase database ){
      setSize( 200 , 200 ) ;
      _pnfs        = database ;
      _switchboard = new SwitchBoard( "TreePanel"  , this ) ;
      _tree  = new String[5] ;
      for( int i = 0 ; i < _tree.length ; i++ )
         _tree[i] = "Object "+i ;
   
   }    
   public void eventArrived( SwitchBoardEvent event ){
      Args args = new Args( event.getMessage() ) ; 
      String [] str = new String[args.argc()] ;
      for( int i = 0 ; i < str.length ; i++ )
         str[i] = args.argv(i) ;
      synchronized( _treeLock ){
         _tree = str ;
      }
      repaint() ;
   
   }
   public void paint( Graphics g ){
       Dimension   d  = getSize() ;
       FontMetrics fm = g.getFontMetrics() ;
       int x = 10 ;
       int y = 10 , l ;
       String [] stringArray ;
       int [] xes = new int[3] ;
       int [] yes = new int[3] ;
       synchronized( _treeLock ){
          stringArray = _tree ;
       }
       y += fm.getHeight() ;
       if( stringArray == null )return ;
       for( int i = 0 ; i < stringArray.length ; i ++ ){
           l  = fm.stringWidth( stringArray[i] ) ;
           g.drawString( stringArray[i] , x , y ) ;
           xes[0] = xes[1] = x + l/2 ;
           yes[0] = y + fm.getHeight() ;
           yes[2] = yes[1] = yes[0] + fm.getHeight() ;
           xes[2] = xes[1] + l/2 ;
           g.drawPolyline( xes , yes , 3 ) ;
           x = xes[2] + 10 ;
           y = yes[2] ;
          
       }
       Color bg = getBackground() ;
       g.setColor(bg.brighter()) ;
       xes[0] = 0 ; xes[1] = 0 ; xes[2] = d.width -1 ;
       yes[0] = d.height -1 ; yes[1] = 0 ; yes[2] = 0 ;
       g.drawPolyline( xes , yes , 3 ) ;
       xes[0] = 1 ; xes[1] = 1 ; xes[2] = d.width -2 ;
       yes[0] = d.height -2 ; yes[1] = 1 ; yes[2] = 1 ;
       g.drawPolyline( xes , yes , 3 ) ;
       g.setColor(bg.darker()) ;
       xes[0] = 1 ; xes[1] = d.width -1 ; xes[2] = d.width -1 ;
       yes[0] = d.height -1 ; yes[1] = d.height -1 ; yes[2] = 1 ;
       g.drawPolyline( xes , yes , 3 ) ;
       xes[0] = 2 ; xes[1] = d.width -2 ; xes[2] = d.width -2 ;
       yes[0] = d.height -2 ; yes[1] = d.height -2 ; yes[2] = 2 ;
       g.drawPolyline( xes , yes , 3 ) ;
       
   }
       
       
}   
 
