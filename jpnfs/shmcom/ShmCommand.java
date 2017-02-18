package jpnfs.shmcom ;

import java.io.* ;
import jpnfs.dbfs.* ;
import jpnfs.dbserver.* ;

public class ShmCommand {

   public static void main( String[] args ){
      if( args.length < 1 ){
         System.err.println( "Usage : ... <command> [<args> ...]" ) ;
         System.err.println( "Usage : ... lookup <dirId> <entryName>" ) ;
         System.err.println( "Usage : ... readdir <dirId> <cookie>" ) ;
         System.exit(4) ;
      }
      ShmCom shmcom = null ;  
      try{
        shmcom  = new ShmCom(0x1122 , 12 * 1024 ) ;
      }catch( IOException ioe ){
        System.out.println( "Exception ie : "+ioe ) ;
        System.exit(4);
      }
      try{
         if( args[0].equals( "lookup" ) ){
            if( args.length < 3 )
               throw new IllegalArgumentException( "Not enough arguments" ) ;
            JmdId id    = new JmdId( args[1] ) ;
            String name = args[2] ;
            ReqLookup reqLookup =  new ReqLookup( id , name  ) ;
            int rc = shmcom.postAndWait(  reqLookup ) ;
//            System.out.println( reqLookup ) ;
            System.out.println( reqLookup.getResultId() ) ;
         }else if( args[0].equals( "readdir" ) ){
            if( args.length < 3 )
               throw new IllegalArgumentException( "Not enough arguments" ) ;
            JmdId id    = new JmdId( args[1] ) ;
            String name = args[2] ;
            long   cookie = Long.parseLong( args[2] ) ;
            ReqReadDir reqReadDir =  new ReqReadDir( id , cookie ) ;
            int rc = shmcom.postAndWait(  reqReadDir ) ;
//            System.out.println( reqReadDir ) ;
            JmdDirItem [] items = reqReadDir.getDirItems() ;
            for( int i = 0 ; i < items.length ; i++ )
               System.out.println( items[i].getId()+"  "+
                                   items[i].getName()+"   "+
                                   items[i].getCookie() ) ;
         }else{
            throw new IllegalArgumentException( "Unknonw command : "+args[0] ) ;
         }
      }catch( Exception e ){
         System.err.println( "Problem : "+e ) ;
      }  
      shmcom.close() ;
   }
   


} 
