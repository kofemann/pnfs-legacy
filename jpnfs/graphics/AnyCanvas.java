package  jpnfs.graphics ;


import java.awt.* ;
import java.awt.event.* ;
 
 
public class AnyCanvas extends Canvas {
   String _string ;
   public AnyCanvas( int x , int y , String str ){ 
      setSize( x , y ) ;
      _string = str ;
   }
   public void paint( Graphics g ){
      Dimension   d  = getSize() ;
      FontMetrics fm = g.getFontMetrics() ;
      int height     = fm.getHeight() ;
      int width      = fm.stringWidth( _string ) ;
      g.setColor( Color.red ) ;
      g.drawString(  _string , (d.width-width)/2 , d.height/2 ) ;
   }
}
