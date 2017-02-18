package jpnfs.cells ;

import jpnfs.dbfs.* ;
import jpnfs.dbserver.* ;
import jpnfs.shmcom.* ;

import   dmg.cells.nucleus.* ;
import   dmg.cells.network.* ;
import   dmg.util.* ;
import   dmg.protocols.ssh.* ;

import java.util.* ;
import java.io.* ;
import java.net.* ;


/**
  *  
  *
  * @author Patrick Fuhrmann
  * @version 0.1, 15 Feb 1998
  */
public class      ShmLoginCell 
       extends    CellAdapter
       implements Runnable  {

  private StreamEngine   _engine ;
  private BufferedReader _in ;
  private PrintWriter    _out ;
  private InetAddress    _host ;
  private String         _user ;
  private Thread         _workerThread ;
  private CellShell      _shell ; 
  private JmdId          _currentId   = new JmdId("1000") ;
  private boolean        _syncMode    = true ;
  private Gate           _readyGate   = new Gate(false) ;
  private int            _syncTimeout = 10 ;
  private int            _commandCounter = 0 ;
  private String         _lastCommand    = "<init>" ;
  private Reader         _reader        = null ;
  private String         _shmClient     = null ;
  private CellPath       _shmClientPath = null ;
  private JmdDirItem []  _itemList      = new JmdDirItem[64] ;
  private int            _nextItemPos   = 0 ;
  private int            _currentLevel  = 0 ;
  
  public ShmLoginCell( String name , StreamEngine engine ){
     super( name ) ;
     _engine  = engine ;
     
     _reader = engine.getReader() ;
     _in   = new BufferedReader( _reader ) ;
     _out  = new PrintWriter( engine.getWriter() ) ;
     _user = engine.getUserName() ;
     _host = engine.getInetAddress() ;
      
     _workerThread = new Thread( this ) ;         
     
     _workerThread.start() ;

     setPrintoutLevel( 0xff ) ;
     useInterpreter(false) ;
    //
     //
     //
     Dictionary dict = getDomainContext() ;
     _shmClient = (String) dict.get( "shmClientName" ) ;
     if( _shmClient == null ){
        _shmClient = "shmClient" ;
     }
     _shmClientPath = new CellPath( _shmClient ) ;
  }
  public void run(){
    if( Thread.currentThread() == _workerThread ){
        print( prompt() ) ;
        while( true ){
           try{
               if( ( _lastCommand = _in.readLine() ) == null )break ;
               _commandCounter++ ;
               if( execute( _lastCommand ) > 0 ){
                  //
                  // we need to close the socket AND
                  // have to go back to readLine to
                  // finish the ssh protocol gracefully.
                  //
                  try{ _out.close() ; }catch(Exception ee){} 
               }else{
                  print( prompt() ) ;
               }       
           }catch( IOException e ){
              esay("EOF Exception in read line : "+e ) ;
              break ;
           }catch( Exception e ){
              esay("I/O Error in read line : "+e ) ;
              break ;
           }
        
        }
        say( "EOS encountered" ) ;
        _readyGate.open() ;
        kill() ;
    
    }
  }
  //
  //   the database
  //
  public String hh_lookup = "<pnfsId> <name>" ;
  public String ac_lookup_$_2( Args args )throws Exception {
     JmdId id    = new JmdId( args.argv(0) ) ;
     String name = args.argv(1) ;
     ReqLookup reqLookup =  new ReqLookup( id , name  ) ;
     CellMessage msg = new CellMessage( new CellPath(_shmClient),reqLookup);
     try{
         msg = sendAndWait( msg , _syncTimeout*1000 ) ;
     }catch( Exception ioe ){
        return ioe.toString() ;
     }
     if( msg == null ){
        return "Request Timed out " ;
     }else{
        Object result = msg.getMessageObject() ;
        if( result instanceof ReqLookup ){
           reqLookup = (ReqLookup)msg.getMessageObject() ;
           return reqLookup.getResultId().toString() ;
        }else{
           return result.toString() ;
        }
     }
  }
  public String hh_readdir = "<dirPnfsId> [<cookie>]" ;
  public String ac_readdir_$_1_2( Args args )throws Exception {
     JmdId id    = new JmdId( args.argv(0) ) ;
     long cookie = 0 ;
     if( args.argc() > 1 )cookie = Long.parseLong( args.argv(1) ) ;
     ReqReadDir reqReadDir =  new ReqReadDir( id , cookie ) ;
     
     CellMessage msg = new CellMessage( new CellPath(_shmClient),reqReadDir);
     try{
         msg = sendAndWait( msg , _syncTimeout*1000 ) ;
     }catch( Exception ioe ){
        return ioe.toString() ;
     }
     if( msg == null ){
        return "Request Timed out " ;
     }else{
        Object result = msg.getMessageObject() ;
        if( result instanceof ReqReadDir ){
           reqReadDir = (ReqReadDir)msg.getMessageObject() ;
           JmdDirItem [] items = reqReadDir.getDirItems() ;
           StringBuffer sb = new StringBuffer() ;
           for( int i = 0 ; i < items.length ; i++ ){
              sb.append( items[i].getId().toString() ).append( "   " ) ;
              sb.append( items[i].getName()).append( "   " ) ;
              sb.append( items[i].getCookie()).append( "\n" ) ;
           }
           return sb.toString() ;
        }else{
           return result.toString() ;
        }
     }
     
  }
  public String hh_cd = "[<dirName>]" ;
  public String ac_cd_$_0_1( Args args )throws Exception {
     if( args.argc() == 0 ){
         _currentId   = new JmdId("1000") ;
         _nextItemPos = 0 ;
         return "" ;
     }
     JmdId id    = _currentId ;
     String name = args.argv(0) ;
     if( name.equals( ".." )  ){
        _nextItemPos = _nextItemPos > 0 ? _nextItemPos - 1 : 0 ;
        if( _nextItemPos > 0 ){
           _currentId = _itemList[_nextItemPos-1].getId() ;
        }else{
           _currentId = new JmdId( "1000" ) ;
        }
        return "" ;
     }
     ReqLookup reqLookup =  new ReqLookup( id , name  ) ;
     CellMessage msg = new CellMessage( _shmClientPath,reqLookup);
     try{
         msg = sendAndWait( msg , _syncTimeout*1000 ) ;
     }catch( Exception ioe ){
        return ioe.toString() ;
     }
     if( msg == null ){
        return "Request Timed out " ;
     }else{
        Object result = msg.getMessageObject() ;
        if( result instanceof ReqLookup ){
           reqLookup    = (ReqLookup)result ;
           _currentId   = reqLookup.getResultId() ;
           _itemList[_nextItemPos++] = reqLookup.getDirItem() ;
           return ""  ;
        }else{
           return result.toString() ;
        }
     }
  }
  public String ac_ls_$_0_1( Args args )throws Exception {
     JmdId        id   = _currentId  ;
     if( args.argc() > 0 )id = new JmdId( args.argv(0) ) ;
     long         cookie = 0 ;
     CellPath     path   = new CellPath( _shmClient ) ;
     ReqReadDir   reqReadDir ;
     CellMessage  msg ;
     StringBuffer sb     = new StringBuffer() ;
     Object       result = null ;
     while( true ){
        msg = new CellMessage( path , new ReqReadDir( id , cookie ) );
        try{
           msg = sendAndWait( msg , _syncTimeout*1000 ) ;
        }catch( Exception ioe ){
           return ioe.toString() ;
        }
        if( msg == null ){
           return "Request Timed out " ;
        }else{
           result = msg.getMessageObject() ;
           if( result instanceof ReqReadDir ){
              reqReadDir = (ReqReadDir)msg.getMessageObject() ;
              JmdDirItem [] items = reqReadDir.getDirItems() ;
              if( items.length <= 0 )break ;
              for( int i = 0 ; i < items.length ; i++ ){
                 if( items[i].getName().equals("..") ||
                     items[i].getName().equals("." )    )continue ;
                 sb.append( items[i].getId().toString() ).append( "   " ) ;
                 sb.append( items[i].getName()).append( "\n" ) ;
              }
              cookie = items[items.length-1].getCookie() ;
           }else{
              return result.toString() ;
           }
        }
     }
     return sb.toString() ;
     
  }
  //
  //    this and that
  //
   public void   cleanUp(){
   
     say( "Clean up called" ) ;
     println("");
     try{ _out.close() ; }catch(Exception ee){} 
     _readyGate.check() ;
     say( "finished" ) ;

   }
  public void println( String str ){ 
     _out.print( str ) ;
     if( ( str.length() > 0 ) &&
         ( str.charAt(str.length()-1) != '\n' ) )_out.print("\n") ;
     _out.flush() ;
  }
  public void print( String str ){
     _out.print( str ) ;
     _out.flush() ;
  }
   public String prompt(){ 
      StringBuffer sb = new StringBuffer() ;
      sb.append(_currentId.toString() ) ;
      if( _currentLevel > 0 )
             sb.append("[").append(_currentLevel).append("]") ;
      sb.append("-" ) ;
      if( _nextItemPos == 0 ){
         sb.append( "/" ) ;
      }else{
         for( int i = 0 ;i < _nextItemPos ; i++ )
            sb.append( "/" ).append( _itemList[i].getName() ) ;
      }
      sb.append(" > " ) ;   
      return sb.toString()  ; 
   }
   public int execute( String command ) throws Exception {
      if( command.equals("") )return 0 ;
      
         try{
             println( command( command ) ) ;
             return 0 ;
         }catch( CommandExitException cee ){
             return 1 ;
         }
   
   }
   private void printObject( Object obj ){
      if( obj == null ){
         println( "Received 'null' Object" ) ;
         return ;
      }  
      String output = null ;    
      if( obj instanceof Object [] ){
         Object [] ar = (Object []) obj ;
         for( int i = 0 ; i < ar.length ; i++ ){
            if( ar[i] == null )continue ;
             
            print( output = ar[i].toString() ) ;
            if(  ( output.length() > 0 ) &&
                 ( output.charAt(output.length()-1) != '\n' ) 

               )print("\n") ;
         }
      }else{
         print( output =  obj.toString() ) ;
         if( ( output.length() > 0 ) &&
             ( output.charAt(output.length()-1) != '\n' ) )print("\n") ;
      }
   
   }
  //
  // the cell implemetation 
  //
   public String toString(){ return _user+"@"+_host ; }
   public void getInfo( PrintWriter pw ){
     pw.println( "            Stream LoginCell" ) ;
     pw.println( "         User  : "+_user ) ;
     pw.println( "         Host  : "+_host ) ;
     pw.println( " Last Command  : "+_lastCommand ) ;
     pw.println( " Command Count : "+_commandCounter ) ;
   }
   public void   messageArrived( CellMessage msg ){
   
        Object obj = msg.getMessageObject() ;
        println("");
        println( " CellMessage From   : "+msg.getSourceAddress() ) ; 
        println( " CellMessage To     : "+msg.getDestinationAddress() ) ; 
        println( " CellMessage Object : "+obj.getClass().getName() ) ;
        printObject( obj ) ;
     
   }
   ///////////////////////////////////////////////////////////////////////////
   //                                                                       //
   // the interpreter stuff                                                 //
   //                                                                       //
   public String ac_set_timeout_$_1( Args args ) throws Exception {
      _syncTimeout = new Integer( args.argv(0) ).intValue() ;
      return "" ;
   }
   public String hh_set_level = "<fsLevel>" ;
   public String ac_set_level_$_1( Args args ) throws Exception {
      int newLevel = new Integer( args.argv(0) ).intValue() ;
      if( ( newLevel > 7 ) || ( newLevel < 0 ) )
        return "Illegal Level "+newLevel ;
      _currentLevel = newLevel ;
      return "" ;
   }
   public String ac_exit( Args args ) throws CommandExitException {
      throw new CommandExitException( "" , 0 ) ;
   }
      


}
