
import java.awt.* ;
import java.awt.event.*;

public class      LogoCanvas 
       extends    Canvas 
       implements MouseListener , Runnable {
       
   private String     _string = "PnfsSpy" , _result ;
   private Font       _font   , _smallFont  ;
   private Toolkit    _toolkit ;
   private ActionListener _actionListener = null ;
   //
   //  all the choise stuff
   //
   private boolean    _makeChoiseMode = false ;
   private String []  _choises ;
   private Object     _choiseLock = new Object() ;
   private int    []  _offsets ;
   //
   //  all the animation
   //
   private Thread     _worker = null ;
   private int        _animationMode  = 0 ;
   private int        _animationState = 0 , _edges = 3 ;
   private Image      _offImage ;
   private Object     _animationLock = new Object() ;
   //
   // the animation modes
   //
   public static final int INFINIT    = 1 ;
   public static final int GROWING    = 2 ;
   public static final int SHRINKING  = 3 ;
   
   public LogoCanvas(){  this(null) ; }
   public LogoCanvas( String title ){
      super() ;
      _string = title == null ? "Pnfs" : title ;
      setSize( 300 , 200  );      

      _toolkit = getToolkit() ;
      
      setBackground( Color.blue ) ;
      addMouseListener( this ) ;
      _font      = new Font( "TimesRoman" , Font.BOLD , 40 ) ;
      _smallFont = new Font( "TimesRoman" , Font.ITALIC , 20 ) ;
      
      _makeChoiseMode = false ;
      _animationMode  = 0 ;
      
   }
   public void setActionListener( ActionListener l ){ 
        _actionListener  = l ;
        return ;
   }
   public void setString( String str ){
     _makeChoiseMode  = false ;
     _stopAnimation() ;
     _string = str ;
     repaint() ;
     return ;
   } 
   public void animation( int mode ){
     _makeChoiseMode  = false ;
     _animationMode   = mode ;
     if( ( _animationMode == INFINIT ) ||
         ( _animationMode == GROWING )     ){
        _animationState = 0 ;
     }else{
        _animationState = 1000 ;
     }
     synchronized( _animationLock ){
        if( _worker != null ){ _worker.stop() ; }
        _worker = new Thread( this ) ;
        _worker.start();
     }
   }
   public String makeChoise( String title , String [] choises ){
      _stopAnimation() ;
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
      _string = "" ;
      
      return _result ;
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
      }else if( _animationMode > 0 ) {
         _offImage = createImage( d.width , d.height ) ;         
         _makeFun( g ) ;
      }else {
         if( _string == null )return ;
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
   public void update( Graphics g ){
      if( _animationMode > 0 ) {
         _makeFun( g ) ;
      }
   }   
   private void _stopAnimation(){
     _animationMode = 0 ;
     synchronized( _animationLock ){
        if( _worker != null ){ _worker.stop() ; }
        _worker = null ;
     }
       
   }
   public void run(){
     if( Thread.currentThread() == _worker ){
        if( _animationMode == INFINIT ){
           while(true){
              _runUp() ;
              _runDown() ;
             _edges ++ ;
             if( _edges > 5 )_edges = 3 ;
           }
        }else if( _animationMode == GROWING ){
           _edges = 3 ;
           _runUp();
           if( _actionListener != null )
             _actionListener.actionPerformed( 
                  new ActionEvent( this , 0 , "finished" ) ) ;
        }else if( _animationMode == SHRINKING ){
           _edges = 3 ;
           _runDown();
           if( _actionListener != null )
             _actionListener.actionPerformed( 
                  new ActionEvent( this , 0 , "finished" ) ) ;
        }
     }
   }
   private void _runDown(){
       for(  _animationState = 1000 ;
             _animationState >= 0 ; 
             _animationState -= 10 ){
             
           repaint() ;
           _toolkit.sync() ;
           try{ Thread.currentThread().sleep(100) ; }
           catch( InterruptedException ie ){}
       }
   }
   private void _runUp(){
       for( _animationState = 0 ;
            _animationState < 1001 ; 
            _animationState += 10 ){
             
            repaint() ;
            _toolkit.sync() ;
            try{ Thread.currentThread().sleep(100) ; }
            catch( InterruptedException ie ){}
       }
   
   }
   private void _makeFun( Graphics g ){
      
      double fraction  = (double) (_animationState) / 1000. ;
      Dimension     d  = getSize() ;
      if( _offImage == null )return ;
      Graphics offGraphics = _offImage.getGraphics() ;
      offGraphics.setColor( Color.blue ) ;
      offGraphics.fillRect( 0 , 0 , d.width , d.height ) ;
//      offGraphics.setColor( new Color( 
//                      Color.HSBtoRGB( (float)0.5 ,
//                                      (float)0.5 ,
//                                      (float)0.9  ) ) );
      offGraphics.setColor( Color.red ) ;
      _drawPolygon( offGraphics , fraction ) ; 
      offGraphics.setColor( Color.blue ) ;
      _drawPolygon( offGraphics , fraction*0.5 ) ; 
      offGraphics.setColor( Color.yellow ) ;
      _drawPolygon( offGraphics , fraction*0.25 ) ; 
      g.drawImage( _offImage , 0 , 0 , null ) ;
   }
   public void _drawPolygon( Graphics g , double fraction ){
      Dimension   d  = getSize() ;
      double y0 = (double)d.height / 2. ;
      double x0 = (double)d.width  / 2. ;
      double r  = Math.min( x0 , y0 ) * fraction ;
      double a  = fraction * 2. *  Math.PI ;
      int    n  = _edges ;
      double diff =  2. *  Math.PI / (double) n;
      int [] x  = new int [n] ;
      int [] y  = new int [n] ;
      
      for( int i = 0 ; i < n ; i ++ , a+= diff  ){
         x[i]  = (int)( x0 + r * Math.sin( a ) );
         y[i]  = (int)( y0 - r * Math.cos( a ) );
      }
      g.fillPolygon( x , y , n ) ; 
   }
   public void mouseClicked( MouseEvent e ){
     if( _makeChoiseMode ){
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
     }else if( _animationMode > 0 ){
       _stopAnimation() ;
       if( _actionListener != null )
          _actionListener.actionPerformed( 
              new ActionEvent( this , 0 , "clicked" ) ) ;
     }else{
       if( _actionListener != null )
          _actionListener.actionPerformed( 
              new ActionEvent( this , 0 , "clicked" ) ) ;
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
