package  jpnfs.graphics ;


import java.util.* ;
import java.awt.* ;
import java.awt.event.* ;
 
 
public class SelectCanvas extends Canvas implements MouseListener {
   String    []  _string = new String[0] ;
   CardLayout    _cardLayout ;
   Panel         _cards ;
   Vector        _rects   = new Vector() ;
   int           _current = 0 ;
   Font          _boldFont , _italicFont ;
   
   public SelectCanvas(  String [] str , Panel panel ){ 
      _string     = str ;
      _SelectCanvas( panel ) ;
      
   }
   
   public SelectCanvas( Panel panel ){ _SelectCanvas( panel ) ; }
   
   public void _SelectCanvas( Panel panel ){
      setSize( 100 , 40 ) ;
      _cards      = panel ;
      _cardLayout = (CardLayout) panel.getLayout()  ;
      addMouseListener( this ) ;
      _boldFont   = new Font( "TimesRoman" , Font.BOLD   , 20 ) ;
      _italicFont = new Font( "TimesRoman" , Font.ITALIC , 20 ) ;
      
   }
   
   public synchronized void addString( String str ){
      String [] s = new String[_string.length+1] ;
      int i ; 
      for( i = 0 ; i < _string.length ; i++ )s[i] = _string[i] ;
      s[i] = str ;
      _string = s ;
   }
   public void paint( Graphics g ){
      Dimension   d  = getSize() ;
      FontMetrics fm  ;
      g.setColor( Color.white ) ;
      g.fillRect( 0 , 0 , d.width , d.height ) ;
      _rects.removeAllElements() ;
//      g.setColor( Color.blue ) ;
      g.setColor( Color.white ) ;
      int xDelta = 12 ;
      int yDelta = 6 ;
      int n      = _string.length ; 
      int diff   = ( d.width - ( n + 1 )*xDelta ) / n ;
      int height = d.height - yDelta ;
      int x      = xDelta ;
      int y      = yDelta ;
      int fHeight ;
      int fWidth  ;
      for( int i = 0 ; i < n ; i++ ){
          Rectangle r = new Rectangle( x , y , diff , height  ) ;
         _rects.addElement( r ) ;
         x += ( diff + xDelta ) ;
      }
      int [] px = new int[4] ;
      int [] py = new int[4] ;
      for( int i = 0 ; i < n ; i++ ){
         if( i == _current )continue ;
         g.setColor( Color.blue ) ;
         g.setFont( _italicFont ) ;         
         fm = g.getFontMetrics();
         fHeight = fm.getHeight() ;
         fWidth  = fm.stringWidth(_string[i]) ;
         Rectangle r = (Rectangle)_rects.elementAt(i) ;
         px[0] = r.x - xDelta ;
         px[1] = r.x  ;
         px[2] = r.x + diff ;
         px[3] = r.x + diff + xDelta ;
         py[1] = py[2] = r.y ;
         py[0] = py[3] = r.y + r.height ;        
         g.fillPolygon( px , py , px.length  ) ;
         g.setColor( Color.white ) ;
         g.drawString(  _string[i] , 
                       r.x + (diff-fWidth)/2 ,
                       r.y + (height+fHeight)/2 ) ;
      }
      int i = _current ;
      g.setColor( Color.lightGray ) ;
      g.setFont( _boldFont ) ;
      fm = g.getFontMetrics();
      fHeight = fm.getHeight() ;
      fWidth  = fm.stringWidth(_string[i]) ;
      Rectangle r = (Rectangle)_rects.elementAt(i) ;
      px[0] = r.x - xDelta ;
      px[1] = r.x  ;
      px[2] = r.x + diff ;
      px[3] = r.x + diff + xDelta ;
      py[1] = py[2] = r.y ;
      py[0] = py[3] = r.y + r.height ;        
      g.fillPolygon( px , py , px.length  ) ;
      g.setColor( Color.black ) ;
      g.drawString(  _string[i] , 
                    r.x + (diff-fWidth)/2 ,
                    r.y + (height+fHeight)/2 ) ;
      
   }
   public void mouseClicked( MouseEvent e ){
      int x = e.getX() ;
      int y = e.getY() ;
      for( int i = 0  ; i < _rects.size() ; i++ ){
         if( ((Rectangle)_rects.elementAt(i)).contains( x , y ) ){
            _cardLayout.show( _cards , _string[i] ) ;
            _current = i ;
            repaint() ;
            break ;
         }
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
