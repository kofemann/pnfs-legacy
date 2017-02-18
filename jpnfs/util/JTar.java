
package jpnfs.util ;

import java.io.* ;
import java.util.* ;

 public class JTar {

    private static Base64 _base64 = new Base64() ;


    public JTar( String [] args ) throws IOException {
        if( args.length < 2 )usage() ;
        
        String mode     = args[0] ;
        String textFile = args[1] ; 
        if( mode.equals("cf") ){

            if( args.length < 3 )usage() ;

            PrintStream ps =  
                 new PrintStream( textFile.equals("-")  ?
                                  System.out :
                                  new PrintStream( new FileOutputStream(new File(textFile))) ) ;

            try{
               scanDirectory( new File( args[2] ) , ps ) ;
            }finally{
               try{ ps.flush() ; }catch(Exception eee ){}
               try{ ps.close() ; }catch(Exception eee ){}
            }

        }else if( mode.equals("xf") || mode.equals("xvf") ){

            InputStream in = textFile.equals("-") ? 
                             System.in :
                             new FileInputStream( new File(textFile) ) ;

            BufferedReader br = new BufferedReader( new InputStreamReader( in ) ) ; 

            try{
               expandFile( br , mode.equals("xvf") ) ;
            }catch(Exception e ){
               try{ br.close() ; }catch(Exception eee ){}
            }

        }else usage() ; 
    }
    public static void usage(){
       System.err.println("Usage : ... cf <filename> <directory>");
       System.err.println("Usage : ... xf <filename>" ) ;
       System.exit(4);
    }
    private void expandFile( BufferedReader br , boolean verbose ) throws IOException {
       String line = null ;
       while( ( line = br.readLine() ) != null ){

          StringTokenizer st = new StringTokenizer(line) ;

          String  pnfsId     = st.nextToken() ;
          String  type       = st.nextToken() ;
          boolean directory  = type.equals("d");
          File    file       = new File( st.nextToken() ) ;
          long    size       = Long.parseLong( st.nextToken() ) ;
          String  attr       = st.nextToken() ;
          StringTokenizer sx = new StringTokenizer(attr,"/") ;
          ArrayList list     = new ArrayList() ;
          while( sx.hasMoreTokens() )list.add(sx.nextToken());

          try{
             if( directory ){
                file.mkdir() ;
             }else{
                file.createNewFile() ;
             }
          }catch(Exception ee ){
             System.err.println("Problem creating "+file+" : "+ee.getMessage() ) ;
             continue ;
          }
          String newPnfsId = getId(file) ;
          if( verbose )System.out.println(pnfsId+" "+newPnfsId+" "+file);
          File   parent    = file.getParentFile() ;
          String filename  = file.getName() ;
          for( int i = 0 , n = list.size() ; i < n ; i++ ){
             File attrfile = new File( parent , ".(pset)("+newPnfsId+")(attr)("+i+")("+list.get(i)+")" ) ;
             try{
                attrfile.createNewFile();
             }catch(Exception ee ){
                //
                // will return an error for directories, but will work anyway.
                //
                if(!directory)System.err.println("Problem creating attr level "+i+" of newPnfsId "+ee.getMessage()); 
             }
          }
 
          if( ! directory ){

              new File( parent , ".(fset)("+filename+")(size)("+size+")" ).createNewFile();
              
              for( int i = 1 ; ( i < 8 ) && st.hasMoreTokens() ; i++ ){
                 String content = st.nextToken();
                 if( content.equals("-") )continue ; 
                 byte [] data = _base64.base64ToByteArray(content);
                 File datafile = new File( parent , ".(use)("+i+")("+filename+")" ) ;
                 try{
                    FileOutputStream out = new FileOutputStream( datafile ) ;
                    try{
                       out.write( data , 0 , data.length ) ;
                    }finally{
                       try{ out.close() ; }catch(Exception ee ){}
                    }
                 }catch(Exception eee ){
                    System.err.println("Problem storing level "+i+" of "+file+" : "+eee.getMessage());
                    continue ;
                 }
              }
          }
       }

    }
    private void scanDirectory( File dir , PrintStream out ) throws IOException {

        String [] list = dir.list() ;
        if( list == null )return ;
        for( int i = 0 , n = list.length ; i < n ; i++ ){
           
 
           File    f         = new File( dir , list[i] ) ;
           try{
           
              boolean directory = f.isDirectory() ;
              String  pnfsId    = getId(f) ;
              StringBuffer sb   = new StringBuffer() ;
              sb.append(pnfsId).append(" ").
                 append(directory?"d ":"f ").
                 append(f.toString()).append(" ").
                 append(f.length()).append(" ").
                 append(getAttributes(f,pnfsId)).
                 append(" ") ;
                 
                 if( ! directory ){
                    for( int j = 1 ; j < 8 ; j++ ){
                       byte [] x = readLevel( f , j ) ;
                       if( x == null ) sb.append("-") ;
                       else sb.append( _base64.byteArrayToBase64(x) ) ;
                       sb.append(" ");
                    }
                 }

               out.println(sb.toString()) ;
            }catch(Exception  linee){
               System.err.println("Problem scanning : "+f+" ("+linee.getMessage()+")");
            }

            if( f.isDirectory() )scanDirectory( f , out ) ;

        }
    }
    private byte [] readLevel( File f , int level )throws IOException {

       File parent = f.getParentFile() ;
       String base = f.getName() ;

       File file = new File( parent , ".(use)("+level+")("+base+")" ) ;

       int len = (int)file.length() ;
       if( len == 0 )return null ;
       
       byte [] result = new byte[len] ;

       FileInputStream in = new FileInputStream(file) ;

       try{
          in.read( result , 0 , result.length ) ;
          return result ;
       }catch(IOException ee ){
          try{  in.close() ; }catch(Exception eee){}
       }
       return null ;
    }
    private String getAttributes( File file , String pnfsId ) throws IOException {

       File parent = file.getParentFile() ;
       String base = file.getName() ;
       File idFile = new File( parent , ".(getattr)("+pnfsId+")" ) ;

       BufferedReader br = new BufferedReader(
                             new FileReader( idFile ) ) ;
       String line = null ;
       StringBuffer sb = new StringBuffer() ;
       try{
          sb.append(br.readLine());
          while( ( line = br.readLine() ) != null ){
            sb.append("/").append(line);
          }
             
       }finally{
          try{ br.close() ; }catch(Exception ce ){}
       }

       return sb.toString() ;
    }
    private String getId( File file ) throws IOException {

       File parent = file.getParentFile() ;
       String base = file.getName() ;
       File idFile = new File( parent , ".(id)("+base+")" ) ;

       BufferedReader br = new BufferedReader(
                             new FileReader( idFile ) ) ;
       String line = null ;
       try{
          line = br.readLine() ;
       }finally{
          try{ br.close() ; }catch(Exception ce ){}
       }
       if( line == null )
         throw new
         IOException("Couldn't get pnfsId of "+file) ;

       return line ;
    }
    public static void main( String [] args ) throws Exception {

        new JTar( args ) ;
        System.exit(0);
    }

 }
