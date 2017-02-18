package jpnfs.pnfs ;
import  java.util.*;
import  java.io.*;

public class Pnfs {

 public Pnfs(){
 
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
}
