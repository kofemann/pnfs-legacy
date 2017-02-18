package jpnfs.graphics ;

import java.awt.* ;
import java.awt.event.*;
public class HelloCanvas 
       extends Canvas 
       implements MouseListener {
       
   String     _string = "PnfsSpy" , _result ;
   Font       _font   , _smallFont  ;
   boolean    _makeChoiseMode = false ;
   String []  _choises ;
   Object     _choiseLock = new Object() ;
   int    []  _offsets ;
   
   public HelloCanvas(){  this(null) ; }
   public HelloCanvas( String title ){
      super() ;
      _string = title == null ? "Pnfs" : title ;
      setSize( 300 , 200  );      

      setBackground( Color.blue ) ;
      addMouseListener( this ) ;
      _font      = new Font( "TimesRoman" , Font.BOLD , 40 ) ;
      _smallFont = new Font( "TimesRoman" , Font.ITALIC , 20 ) ;
   }
   public void setString( String str ){
     _string = str ;
     repaint() ;
     return ;
   } 
   public void paint( Graphics g ){
      Dimension   d  = getSize() ;
      if( _makeChoiseMode ){
         g.setFont( _font ) ;
         FontMetrics fm = g.getFontMetrics() ;
         int y = 10 , x = 10  ;

         y  += fm.getHeight() ;
         g.drawString(  _string , x , y ) ;
         
         x += 10 ;
         g.setFont( _smallFont ) ;
         fm = g.getFontMetrics() ;
         int height     = fm.getHeight() ;
         int leading    = fm.getLeading() ;
         for( int i = 0 ; i < _choises.length ; i++ ){
            y += ( height + 2*leading );
            g.drawString(  _choises[i] , x , y ) ;
            _offsets[i] = y ;
         }
      }else{
         g.setFont( _font ) ;
         FontMetrics fm = g.getFontMetrics() ;
         int height     = fm.getHeight() ;
         int width      = fm.stringWidth( _string ) ;
         int y = ( d.height + height ) / 2 ;
         int x = ( d.width  - width  ) / 2 ;
         g.setColor( Color.red ) ;
         g.drawString(  _string , x , y ) ;
      }
   }
   public String makeChoise( String title , String [] choises ){
   
      _makeChoiseMode = true ;
      _string         = title ;
      _choises        = choises ;
      _offsets        = new int[choises.length] ;
      
      synchronized( _choiseLock ){
         repaint() ;
         try { _choiseLock.wait() ; }
         catch( InterruptedException ie ){}
      }
      _makeChoiseMode = false ;
      return _result ;
   }
   public void mouseClicked( MouseEvent e ){
       synchronized( _choiseLock ){
          int i ;
          _result = null ;
          if( _offsets == null )return ;
          for( i = 0 ; i < _offsets.length ; i++ ){
             if( _offsets[i] > e.getY() )break ;
          }
          if( i == _offsets.length )return ;
          _result = _choises[i] ;
          _choiseLock.notifyAll() ;
       }
   }
   public void mouseExited( MouseEvent e ){
   }
   public void mouseEntered( MouseEvent e ){
   }
   public void mousePressed( MouseEvent e ){
   }
   public void mouseReleased( MouseEvent e ){
   
   }

}
