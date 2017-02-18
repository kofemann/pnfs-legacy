package jpnfs.pnfs ;
import  java.io.* ;

public class PnfsExportsTest {

  public static void main( String [] args ){
     if( args.length < 3 ){
        System.out.println( "Usage : ... <exportsDir> <group> <vmp>" ) ;
        System.exit(43) ;
     }
     String group = args[1] ;
     String vmp   = args[2] ;
     PnfsMountpoint mp ;
     PnfsExportsDatabase pnfs = null ;
     try{
        pnfs = new PnfsExports( args[0] ) ;
     }catch( IOException ioe ){
        System.out.println( " IOExeption in "+args[0] ) ;
        System.exit(4);
     }
     while( true ){
     
         mp = pnfs.getMountpoint( group , vmp ) ; 
         if( mp == null ){
            System.out.println( "Chain broken" ) ;
            break ;
         }
         if( mp.isLink() ){
            System.out.println( " Link : "+mp ) ;
            String newGroup = mp.getMountgroup() ;
            group = newGroup == null ? group : newGroup  ;
            String newVmp   = mp.getNewVirtualMountpoint() ;
            vmp = newVmp == null ? vmp : newVmp ;
         }else{
            System.out.println( " Rmp  : "+mp.getRealMountpoint() ) ;
            break ;
         }
     }
        
  
  }

}
