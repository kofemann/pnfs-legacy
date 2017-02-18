package jpnfs.graphics ;
 import java.awt.* ;
 public class statCanvas extends Canvas {
   double  _floatFraction  = 0.0 ;
   Color   _color          = Color.red ;
   String  _name           = null ;
   public statCanvas( String name ){
      super();  
      _name = name ;
   }
   public void setFraction( double f ){ 
     if( f < 0. )_floatFraction = 0. ;
     else if( f > 1. )_floatFraction = 1. ;
     else _floatFraction = f ;
     repaint();
     return ;
   }
   public void setColor( Color c ){ _color = c ; return ; }
   public void paint( Graphics g ){
      Dimension   d  = getSize() ;
      FontMetrics fm = g.getFontMetrics() ;
      int width  = (int)(  ((double)(d.width)) * _floatFraction ) ;
      int height = fm.getHeight() ;
      
      if( _floatFraction > 0.8 )g.setColor( Color.red ) ;
      else g.setColor( _color ) ;
      
      g.fillRect( 0 , 0 , width , d.height ) ;
      
      if( d.height > ( height + 6 ) ){
        g.setColor( Color.black ) ;
        g.drawRect( 0 , 0,  d.width - 1 , d.height - 1 ) ;
        //
        // frame
        //
        g.drawRect( 2 , 2,  d.width - 5 , d.height - 5 ) ;
        g.setColor( Color.white ) ;
        g.drawRect( 1 , 1,  d.width - 3 , d.height - 3 ) ;
        //
        // databasename
        //
        g.setColor( Color.black ) ;
        g.drawString( _name , 40 , (d.height + height)/2 ) ;
      }else if( d.height > 12 ){
        g.setColor( Color.white ) ;
        g.drawRect( 0 , 0,  d.width - 1 , d.height - 1 ) ;
      }
   }
 
 
 }
