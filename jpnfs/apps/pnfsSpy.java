package jpnfs.apps ;

import jpnfs.pnfs.* ;
import jpnfs.graphics.* ;

import java.awt.* ;
import java.awt.event.* ;
import java.util.* ;
import java.io.* ;
   
 public class      pnfsSpy 
        extends    Frame 
        implements ActionListener , Runnable  {
                               
    MenuBar        _menuBar ;
    Menu           _editMenu , _fileMenu , _widthMenu ;
    Menu           _updateMenu ;
    MenuItem       _editEditItem ;
    MenuItem       _fileExitItem ;
    statCanvas []  _stats ;
    Thread         _worker , _init ;
    pnfsCounters[] _counters ;
    int            _maxCounters ,  _counter = 0 ;
    long []        _startSums  = null ;
    long []        _startTimes = null ;
    long []        _lastSums  = null ;
    long []        _lastTimes = null ;
    long []        _sums      = null ;
    long []        _times     = null ;     
    double []      _fractions = null ;  
    HelloCanvas    _helloCanvas ;
    Panel          _barPanel ;
    Label          _textField ;
    Panel          _cards ;
    CardLayout     _cardsLayout ;
    int            _intervall = 4000  ;
    double         _maxWidth = 0.0 , _maxFrac = 1.0 ;
    String         _pnfsMountpoint ; 
    boolean        _doSum     = false ;   
    boolean        _fromStart = false ;   
        
    public pnfsSpy( String [] args ){
      
     if( args.length < 1 ){
//       System.err.println( " USAGE : spyTest <pnfsPath>" ) ;
//       System.exit(3);
         _pnfsMountpoint = null ;
         
     }else{
        _pnfsMountpoint = args[0] ;
     }
      setLayout( new BorderLayout() ) ;      
             
//      add( _textField    = new TextField() , "North" ) ;
      add( _textField    = new Label() , "North" ) ;
      add( _cards        = new Panel() , "Center" ) ;
      
      _textField.setText( "Hi there ..... " ) ;
      
      _helloCanvas = new HelloCanvas() ;
      _barPanel    = new Panel( new GridLayout( 0 , 1 ) ) ;
      
      _cards.setLayout( _cardsLayout = new CardLayout() ) ;
      _cards.add( _helloCanvas  , "hello" ) ;
      _cards.add( _barPanel     , "bars"  ) ;

      _cardsLayout.show( _cards , "hello" ) ;
       
      installMenu();
      
      setTitle( "pnfsSpy" ) ;
      
      
      setSize( 600 , 400 );      
      pack();
      setSize( 600 , 400 );
      show();
      
      _init = new Thread( this ) ;
      _init.start();
            
      
       
 }
 public void initPnfs( String mountpoint , int [] ids  ){

    _counters = new pnfsCounters[ids.length] ;
   
    String virtualFile = null ;
    try{
       for( int i = 0 ; i < ids.length ; i++ ){
          virtualFile   = mountpoint+"/.(get)(counters)("+ids[i]+")" ;
          _counters[i]  = new pnfsCounters( virtualFile ) ;
          say( " Open : "+virtualFile ) ;
       }
       _maxCounters = _counters.length ; 
    }catch( IOException ioe ){
       System.err.println( " pnfsCounters says : "+ioe ) ;   
       System.exit(4);    
    }
    _startSums  = new long[ _maxCounters ] ;
    _startTimes = new long[ _maxCounters ] ;
    _lastSums   = new long[ _maxCounters ] ;
    _lastTimes  = new long[ _maxCounters ] ;
    _sums       = new long[ _maxCounters ] ;
    _times      = new long[ _maxCounters ] ;   
    _fractions  = new double[ _maxCounters ] ;    
    return ;
 }     
 public void run(){
       
       if( Thread.currentThread() == _worker ){
          runWorker() ;       
       }else if( Thread.currentThread() == _init ){
          runInit() ;
       }
 }
 private int [] findActiveDatabases( String mp ){
    int []          ids = new int[128] ;
    String          virtualFile ;
    int             i , l , j ;
    BufferedReader  in ;
    String          line ;
    StringTokenizer st ;
    String []       tok = new String[4] ;
    
    for( i = 0 , l = 0 ; i < ids.length ; i++ ){
        virtualFile = mp+"/.(get)(database)("+i+")" ;
        try {
           in = new BufferedReader(new FileReader(virtualFile) );   
           if( ( line = in.readLine() ) == null )break ;
           st = new StringTokenizer( line , ":" ) ; 
           for( j = 0 ; ( j < tok.length ) && st.hasMoreTokens() ; j ++ ){
              tok[j] = st.nextToken() ;
           }  
           if( j < tok.length )continue ;
           if( tok[2].equals( "r" ) && tok[3].equals("enabled") ){
              ids[l++] = new Integer( tok[1] ).intValue() ;
              _helloCanvas.setString( tok[0]+" ["+tok[1]+"]    Active" ) ;
           }else{
              _helloCanvas.setString( tok[0]+" ["+tok[1]+"]    Inactive" ) ;
           }
           _sleep(1) ;
        }catch( Exception ioe ){
            break ;
        }
    }
    int [] rids = new int[l] ;
    for( i = 0 ; i < l ; i++ )rids[i] = ids[i] ;
    return rids ;
 
 }
 private String [] getPnfsMountpoints(){
    String [] mountpoints ;
    
    mountpoints = getPnfsMountpointsFrom( "/etc/mtab" ) ;
    if( mountpoints == null ){
        mountpoints = getPnfsMountpointsFrom( "/etc/mnttab" ) ;    
    }
    return mountpoints ;
 }  
 private String [] getPnfsMountpointsFrom( String mtab ){
 
    BufferedReader  in ;
    FileInputStream pnfs ;
    int             i , l ;
    String          line , raw , mp , type ;
    StringTokenizer st ;
    String []       mountpoints = new String[16] ;
    //
    //
    try{
      in = new BufferedReader(new FileReader( mtab ) );
      for( i = 0 , l = 0 ; 
           ( l < mountpoints.length           ) &&
           ( ( line = in.readLine() ) != null ) ; i++ ){
         st = new StringTokenizer( line ) ;
         if(  st.countTokens() < 4 )return new String[0] ;
         raw  = st.nextToken() ;
         mp   = st.nextToken() ;
         type = st.nextToken() ;
         if( ! type.equals( "nfs" ) )continue ;
         try{
            pnfs = new FileInputStream( mp+"/.(const)(x)" ) ;
            pnfs.close() ;
         }catch( Exception ie3 ){
            continue ;
         }
         mountpoints[l++] = mp ;
      }
      in.close() ;
//  }catch( FileNotFoundException fne ){
//      return new String[0] ;
    }catch( Exception e ){
      return null ;
    }
    String [] out = new String[l] ;
    for( i = 0 ; i < l ; i++ )out[i] = mountpoints[i] ;
    return out ;
 
 }
 private void _sleep( int n ){
    try{ Thread.currentThread().sleep( n*1000 ) ; }
    catch( InterruptedException ie ){} ;
    return ;
 }
 private void runInit(){ 
     _sleep(2) ;
     
     if( _pnfsMountpoint == null ){
         _helloCanvas.setString( "Searching pnfs mountpoints" ) ;     
         _sleep(4);
         String [] mountpoints = getPnfsMountpoints() ;
         if( ( mountpoints == null ) || ( mountpoints.length == 0 ) ){
            System.err.println( " No pnfs mountpoints found " ) ;
            _helloCanvas.setString( "No pnfs mountpoints found" ) ;     
            _sleep( 20 ) ;
            System.exit(4) ;
         }
         say( " Mountpoints : " ) ;
         if( mountpoints.length == 1 ){
            _pnfsMountpoint = mountpoints[0] ;
         }else{           
            _pnfsMountpoint = _helloCanvas.makeChoise( "Which mountpoint ?", 
                                                        mountpoints ) ;
         }
     }
     _helloCanvas.setString( " Using Mountpoint "+_pnfsMountpoint ) ;     
     _sleep(2) ;
     _helloCanvas.setString( "Searching for Active databases" ) ;     
     
     int [] dbIDs = findActiveDatabases( _pnfsMountpoint ) ;
     
     initPnfs( _pnfsMountpoint , dbIDs ) ;
     
     _sleep(2) ;
       
     _stats = new statCanvas[_maxCounters] ;
 
     for( int i = 0 ; i < _stats.length ; i ++ ){
        _stats[i] = new statCanvas(_counters[i].getInfo().getDbName()) ;
        _stats[i].setBackground( Color.green ) ;
        _stats[i].setColor( Color.yellow ) ;
        _stats[i].setSize( 50 , 10 ) ;
        _barPanel.add( _stats[i] ) ;
     } 
         
      _helloCanvas.setString( "Switching to bars ..." ) ;
      
      _worker = new Thread( this ) ;
      _worker.start();
      _sleep(2)  ;
      
      _cardsLayout.show( _cards , "bars" ) ;
 }
 private void runWorker(){
    int   v = 0 ;
     
    boolean tuning = true ;
    
    while( true ){
            
            _updateDisplay() ;
            
            try{ Thread.currentThread().sleep(_intervall) ; }
            catch( InterruptedException ie ){} ;
            
            if( tuning && (( v = _counters[0].getIntervall() ) > 0 )){
//               say( " setting autointervall to "+v+" seconds" ) ;
                 _cardsLayout.show( _cards , "hello" ) ;
                 _helloCanvas.setString( "Setting Refresh to "+v+
                                         " Seconds" ) ;
                 _sleep(2);
                 _cardsLayout.show( _cards , "bars" ) ;
                 _intervall = 1000 * v ; 
                 tuning = false ;              
            }
    }
   
 }
 private synchronized void _updateDisplay(boolean force ){
//    double width = _maxWidth < 1.0 ? _maxFrac : _maxWidth ;
//    say( " Intervall : "+_intervall+ " ; max Width = "+width+" hits/second" ) ;
//    for( int i = 0 ; i < _maxCounters ; i++ )
//       _stats[i].setFraction( _fractions[i] / width ) ;
     _updateDisplay();
 }
 private static final double _choice[] = { 
    1. , 10. ,50. , 
    100.   , 200.    ,
    1000.  , 2000.   , 5000. ,
    10000. , 20000.  , 50000.,
    100000., 200000. , 500000. ,
    1000000., 2000000., 5000000. ,
    10000000. , 20000000., 50000000.} ;
 
 private synchronized void _updateDisplay(){
     pnfsDbInfo info ;
     long  []   diffTimes = new long[_maxCounters] ;
     long  []   diffSums  = new long[_maxCounters] ;
     double width ;
     
     _maxFrac = 0.0 ;
     for( int i = 0 ; i < _maxCounters ; i++ ){
        info      = _counters[i].getInfo() ;
        _sums[i]  = info.getSum()  ;
        _times[i] = info.getTime() ;
        if( _counter == 0 ){
           _startTimes[i] = _lastTimes[i] = _times[i] ;
           _startSums[i]  = _lastSums[i]  = _sums[i]  ;
        }else{
           if( _fromStart ){
              diffTimes[i] = _times[i] - _startTimes[i] ;
              diffSums[i]  = _sums[i]  - _startSums[i] ;
           }else{
              diffTimes[i] = _times[i] - _lastTimes[i] ;
              diffSums[i]  = _sums[i]  - _lastSums[i] ;
           }
           if( diffTimes[i] > 0 ){
              
              if( _doSum ){
                 _fractions[i] = (double)diffSums[i] ;
              }else{
                 _fractions[i] = ((double)diffSums[i])/((double)diffTimes[i] ) ;
              }
              _maxFrac = _maxFrac > _fractions[i] ? _maxFrac : _fractions[i] ;
           }
        }
     }
     if( _counter++ == 0 )return ;
     int l ;
     for(  l = 0 ; ( l < _choice.length ) && ( _maxFrac > _choice[l] ) ; l++ ) ;
     if( l ==  _choice.length )_maxFrac = _choice[_choice.length] ;
     else _maxFrac = _choice[l] ;
     
     width = _maxWidth < 1.0 ? _maxFrac : _maxWidth ;
     
     say( " Intervall = "+
          (_fromStart?" From Start ; ":(""+(_intervall/1000)+" seconds ; ")) +
          " Right Edge = "+width+(_doSum ?" hits ;":" hits/second ;")+
          (_maxWidth<1.0?" scale=auto ;":" scale=fixed ;")  ) ;
     
     for( int i = 0 ; i < _maxCounters ; i++ ){
//         if( diffTimes[i] > 0  ){
             _stats[i].setFraction( _fractions[i] / width ) ;
                _lastTimes[i] = _times[i] ;
                _lastSums[i]  = _sums[i]  ;
//          }
       } 
 
 }
    private void installMenu(){
      _menuBar      = new MenuBar() ;
      
      _fileMenu     = new Menu( "File" ) ;
      
      _fileMenu.add( _fileExitItem = new MenuItem( "Exit" ) );
      _fileExitItem.addActionListener( this ) ;
      _fileExitItem.setActionCommand( "exit" ) ;
      
      _editMenu     = new Menu( "Edit" ) ;
      
      _editMenu.add( _editEditItem = new MenuItem( "Edit Topology" ) );
      _editEditItem.addActionListener( this ) ;
      _editEditItem.setActionCommand( "edit" ) ;
      
      _widthMenu     = new Menu( "Options" ) ;
      
      MenuItem item ;
            
      _widthMenu.add( item = new MenuItem( "AutoScale" ) );
      item.addActionListener( this ) ;
      item.setActionCommand( "auto" ) ;
      _widthMenu.add( item = new MenuItem( "100 hits/sec" ) );
      item.addActionListener( this ) ;
      item.setActionCommand( "set100" ) ;
      _widthMenu.add( item = new MenuItem( "50 hits/sec" ) );
      item.addActionListener( this ) ;
      item.setActionCommand( "set50" ) ;
      _widthMenu.add( item = new MenuItem( "10 hits/sec" ) );
      item.addActionListener( this ) ;
      item.setActionCommand( "set10" ) ;
      _widthMenu.add( item = new MenuItem( "1 hit/sec" ) );
      item.addActionListener( this ) ;
      item.setActionCommand( "set1" ) ;
      _widthMenu.add( item = new MenuItem( "Average From Start" ) );
      item.addActionListener( this ) ;
      item.setActionCommand( "sum" ) ;
      _widthMenu.add( item = new MenuItem( "Last Intervall Only" ) );
      item.addActionListener( this ) ;
      item.setActionCommand( "diff" ) ;
      _widthMenu.add( item = new MenuItem( "Total Count" ) );
      item.addActionListener( this ) ;
      item.setActionCommand( "total" ) ;
      _widthMenu.add( item = new MenuItem( "Counts per Second" ) );
      item.addActionListener( this ) ;
      item.setActionCommand( "ratio" ) ;
      _widthMenu.add( item = new MenuItem( "Reset Counters" ) );
      item.addActionListener( this ) ;
      item.setActionCommand( "reset" ) ;
      
      _menuBar.add( _fileMenu ) ;
//      _menuBar.add( _editMenu ) ;
      _menuBar.add( _widthMenu ) ;
      setMenuBar( _menuBar ) ;
    
    }
    public void actionPerformed( ActionEvent event ){
//      System.out.println( " Action with comment "+event.getActionCommand());
      if( event.getActionCommand().equals("exit") )System.exit(0);
      
      if( event.getActionCommand().equals("set10") ){ 
          _maxWidth = 10.; 
          _updateDisplay(true) ;
      }else if( event.getActionCommand().equals("set1") ){ 
          _maxWidth = 1.; 
          _updateDisplay(true) ;
      }else if( event.getActionCommand().equals("set50") ){ 
          _maxWidth = 50.; 
          _updateDisplay(true) ;
      }else if( event.getActionCommand().equals("set100") ){ 
          _maxWidth = 100.; 
          _updateDisplay(true) ;
      }else if( event.getActionCommand().equals("auto") ){ 
          _maxWidth = 0.0 ; 
          _updateDisplay(true) ;
      }else if( event.getActionCommand().equals("sum") ){ 
          _fromStart = true ;
          _updateDisplay(true) ;
      }else if( event.getActionCommand().equals("diff") ){ 
          _fromStart = false ;
          _updateDisplay(true) ;
      }else if( event.getActionCommand().equals("ratio") ){ 
          _doSum = false ; 
          _updateDisplay(true) ;
      }else if( event.getActionCommand().equals("total") ){ 
           _doSum = true ; 
          _updateDisplay(true) ;
      }else if( event.getActionCommand().equals("reset") ){ 
          resetCounter() ; 
          _updateDisplay(true) ;
      }
      
      Object source = event.getSource() ;
      if(  source == _editEditItem ){
         if( event.getActionCommand().equals("edit") ){
            _editEditItem.setActionCommand( "unedit" ) ;
            _editEditItem.setLabel( "Normal" ) ;
         }else{
            _editEditItem.setActionCommand( "edit" ) ;
            _editEditItem.setLabel( "Edit Mode" ) ;
         }
      }
    }
    private synchronized void resetCounter(){ _counter = 0 ; }
    
    private void say( String text ){
//       System.out.println( text ) ;
       _textField.setText( text ) ;
    }
    public static void main( String [] args ){

      new pnfsSpy( args ) ;
    
    }
 
 
 }
