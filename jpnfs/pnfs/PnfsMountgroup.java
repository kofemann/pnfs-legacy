package jpnfs.pnfs ;

import  java.util.* ;
import  java.io.* ;

public  class      PnfsMountgroup 
        extends    Hashtable 
        implements PnfsModifiable, Cloneable {
        
   private String  _name ;
   private boolean _wasChanged = false ;
   private File    _baseFile   = null ;
   
   public PnfsMountgroup( String name ){
     super() ;
     _name = name ;
     wasChanged(true);
   }
   public PnfsMountgroup( String name , File in )
          throws IOException    {
     this( name ) ;
     loadMountgroupFile( _baseFile = in ) ;
     wasChanged(false);
   }
   public PnfsMountgroup( File dir , String name )
          throws IOException    {
     this( name ) ;
     _baseFile = new File( dir , name ) ;
     loadMountgroupFile( _baseFile ) ;
     wasChanged(false);
   }
   public void load()
          throws IOException    {
     if( _baseFile == null )
        throw new IOException( "No Basefile informations" ) ;
     loadMountgroupFile( _baseFile ) ;
     wasChanged(false);
   }
   public Object clone(){ return this.clone() ; }
   public int hashCode(){ return _name.hashCode() ; }
   public boolean equals( Object obj ){
     return ((PnfsMountgroup)obj)._name.equals( _name ) ;
   }
   public String getName(){ return _name ; }
   public void removeMountpoint( String vmp ){
      remove( vmp ) ;
      wasChanged(true);
   }
   public void addMountpoint( PnfsMountpoint mp ){
     put( mp.getVirtualMountpoint() , mp ) ;
     wasChanged(true);
   }
   public Enumeration mountpoints(){ return elements() ; }
   public PnfsMountpoint getMountpoint( String vmp ){
      return (PnfsMountpoint)get(vmp) ;
   }   
   public void wasChanged( boolean t ){ _wasChanged = t ; }
   public boolean wasChanged(){ return _wasChanged ; }
   public void save() throws IOException {
     if( ( ! wasChanged() ) || ( _baseFile == null ) )return ;
     save( _baseFile ) ;
   }
   public void save( File file ) throws IOException {
     _baseFile = file ;
     PrintWriter out = new PrintWriter( 
                       new BufferedWriter(
                       new FileWriter( file ) ) ) ;
     Enumeration e = elements() ;
     for( ; e.hasMoreElements() ; ){
        out.println( ((PnfsMountpoint)e.nextElement()).toString() ) ;
     }
     out.close() ;
     wasChanged(false);
   }
   private void loadMountgroupFile( File file ) throws IOException {
   
      if( ! file.isFile() )
         throw new IOException( "File not found" ) ;
         
      BufferedReader       in ;
      PnfsMountpoint       mp = null ;
      String               line ;
      try{
         in = new BufferedReader( new FileReader( file ) ) ;

         while( ( line = in.readLine() ) != null  ){
            if( ( line.length()   == 0   ) || 
                ( line.charAt(0)  == '#' )     ) continue ;

            try { mp  = new PnfsMountpoint( line ) ; }
            catch( IllegalArgumentException iae ){ continue ; }

            addMountpoint(  mp ) ;

         }
         in.close();
      }catch( IOException ioe ){ 
         throw ioe ;
      }
   }
   public String toString(){
     StringBuffer sb = new StringBuffer() ;
     Enumeration  e  = elements() ;
     for( ; e.hasMoreElements() ; ){
        sb.append( ((PnfsMountpoint)e.nextElement()).toString()+"\n" ) ;
     }
     return sb.toString() ;
     
   }
   public static void main( String [] args ){
      if( args.length < 1 ){
        System.err.println( " USAGE : ... GroupName " ) ;
        System.exit(4);
      }
      String filename = args[0] ;
      try{
         File dir = new File( "." ) ;
         PnfsMountgroup group = new PnfsMountgroup( dir , filename ) ;
         System.out.println( "------------------------------------- " ) ;
         System.out.println( group.toString() ) ;
         group.addMountpoint( 
               new PnfsMountpoint( "/admin" , "/0/admin" , true , 0 ) 
                             ) ;
         System.out.println( "------------------------------------- " ) ;
         System.out.println( group.toString() ) ;
         group.save() ;
      
      }catch( Exception e ){
         System.err.println( "IOError on "+args[0]+" : "+e ) ;
      }
   }
        
}
        
        
