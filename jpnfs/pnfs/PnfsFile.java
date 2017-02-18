package jpnfs.pnfs ;
import  java.io.* ;

public class PnfsFile extends File {

  public PnfsFile( String str ){ super( str ) ; }
  public void isLink(){
     
  }
  public static void main( String [] args ){
     PnfsFile f = new PnfsFile( args[0] ) ;
     try{
     System.out.println( " get absolute path  : "+f.getAbsolutePath() ) ;
     System.out.println( " get canonical path : "+f.getCanonicalPath() ) ;
     System.out.println( " is file            : "+f.isFile() ) ;
     System.out.println( " get length         : "+f.length() ) ;
     System.out.println( " can read           : "+f.canRead() ) ;
     System.out.println( " can write          : "+f.canWrite() ) ;
     System.out.println( " get name           : "+f.getName() ) ;
     System.out.println( " get path           : "+f.getPath() ) ;
     System.out.println( " get parent         : "+f.getParent() ) ;
     }catch( IOException e ){
       System.err.println( " Exception : "+ e ) ;
     }
  }
}
