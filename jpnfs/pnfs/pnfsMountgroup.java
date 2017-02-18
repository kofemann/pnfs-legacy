package jpnfs.pnfs ;

import  java.util.Hashtable ;
import  java.util.Enumeration ;
import  java.util.StringTokenizer;
import  java.io.* ;

/**
  *  pnfsMountgroup is a simple interface to the pnfs export facility.
  *  It contains a number of names mountgroups which themselfs contain
  *  a number of pnfsMountpoints.
  */
public class pnfsMountgroup {

   private Hashtable _groups  = new Hashtable() ;
   private Hashtable _changed = new Hashtable() ;
   /**
     *   Constructs an empty mountgroupset.
     */
   public pnfsMountgroup(){}
   /**
     *   Constructs a mountgroup set and initializes it with
     *   the contents of the database pointed to by 'path'.
     */
   public pnfsMountgroup( String path ){
      load( path ) ;
   }
   /**
     *  Clears the current mountgroup set.
     */
   public void removeAll(){ _groups = new Hashtable() ; }
   /**
     *   Adds the contents of the database 'path' to the
     *   content of the current mountgroup set.
     */
   public void load( String path ){
      load( new File( path ) ) ;
   }
   public void load( File dir ){
      if( ! dir.isDirectory() )return  ;
      String [] groupNames = dir.list() ;
      BufferedReader       in ;
      StringTokenizer      st ;
      pnfsMountpoint       mp = null ;
      Hashtable            groupHash = null;
      String               line ;
      for( int i = 0 ; i < groupNames.length ; i++ ){
         File mgFile = new File( dir , groupNames[i] ) ;
         if( ! mgFile.isFile() )continue ;
         try{
            in = new BufferedReader( new FileReader( mgFile ) ) ;
                 
            for( int j = 0 ; ( line = in.readLine() ) != null ; ){
               if( ( line.length() == 0 ) || ( line.charAt(0) == '#' ) )
                  continue ;
                  
               try { mp  = new pnfsMountpoint( line ) ; }
               catch( IllegalArgumentException iae ){ continue ; }
               
               if( j == 0 ){
                  groupHash = new Hashtable() ;
                  _groups.put( groupNames[i] , groupHash ) ;
               }
               groupHash.put( mp.getVirtualMountpoint() , mp ) ;
               j++ ;
            }
         }catch( Exception ioe ){ continue ; }
      
      }
   
   }
   /**
     *   adds the mountpoint 'mp' to the mountgroup 'group'.
     *   If 'group' doesn't exist, it will be created first.
     */
   public void add( String group , pnfsMountpoint mp ){
       Hashtable      groupHash ;
       
       if( ( groupHash = (Hashtable)_groups.get( group ) ) == null ){
          groupHash = new Hashtable() ;
          _groups.put( group , groupHash ) ;
       }
       groupHash.put( mp.getVirtualMountpoint() , mp ) ;
       _changed.put( group , group ) ;
       return ;
   }
   /**
     *   adds the a mountgroup to the current mountgroup set.
     */
   public void add( String group ) throws IllegalArgumentException {
       
       if( _groups.get( group ) != null )
         throw new IllegalArgumentException( "Duplicated name "+group ) ;
       _groups.put( group , new Hashtable() ) ;
       _changed.put( group , group ) ;
       return ;
   }
   /**
     *   Tries to get the mountpoint with the virtual mountpoint 'vmp'
     *   from the group 'group'. null is returned it 'group' doesn't 
     *   contain 'vmp'
     */
   public pnfsMountpoint get( String group , String vmp ){ 
       Hashtable      groupHash ;
       pnfsMountpoint mp ;
       
       if( ( groupHash = (Hashtable)_groups.get( group ) ) == null )
          return null ;
       
       
       if( ( mp = (pnfsMountpoint)groupHash.get( vmp ) )== null )
          return null ;
                 
       return mp ;
   }
   /**
     * Tries to get the mountpoint with the virtual mountpoint 'vmp'
     * from all groups. null is returned if the specified mountpoint
     * doesn't exists.
     */
   public pnfsMountpoint get( String vmp ){
       Hashtable h ;
       pnfsMountpoint mp ;
       for( Enumeration e = _groups.elements() ; e.hasMoreElements() ; ){
          h  = (Hashtable)e.nextElement() ;
          if( ( mp = (pnfsMountpoint)h.get( vmp ) ) != null )return mp ;
       }
       return null ;
   } 
   /**
     * Tries to get the groupname of the first appearance of 
     * the mountpoint with the virtual mountpoint 'vmp'.
     */
   public String getGroup( String vmp ){
       Hashtable      h ;
       pnfsMountpoint mp ;
       String         groupName ;
       
       for( Enumeration e = _groups.keys() ; e.hasMoreElements() ; ){
          groupName  = (String)e.nextElement() ;
          h = (Hashtable) _groups.get( groupName ) ;
          if( ( mp = (pnfsMountpoint)h.get( vmp ) ) != null )
             return groupName ;
       }
       return null ;
   }
   /**
     * Gets an enumeration of all group names.
     */
   public Enumeration groups(){ return _groups.keys() ; } 
   /**
     *  Tries to find the first appearance of the real mountpoint
     *  in all groups. The full mountpoint is returned or null if
     *  not found.
     */
   public pnfsMountpoint find( String rmp ){
      Hashtable    mountHash ;
      pnfsMountpoint mp ;
      for( Enumeration e = _groups.elements() ; e.hasMoreElements() ; ){
         mountHash = (Hashtable)e.nextElement() ;
         for( Enumeration l = mountHash.elements() ; l.hasMoreElements() ; ){
             mp = (pnfsMountpoint)l.nextElement() ;
             if( mp.getRealMountpoint().equals( rmp ) )return mp ;
          }
      
      }
      return null ;
   }
   /**
     * Removes the mountgroup 'group' from the mount group set.
     */
   public void removeMountgroup( String group ){
      _groups.remove( group ) ;
      _changed.put( group , group ) ;
      return ;
   }
   /**
     * Removes the mountpoint 'vmp' from the mountgroup 'group'
     */
   public void removeMountpoint( String group , String vmp ){
      Hashtable h = (Hashtable)_groups.get( group ) ;
      if( h == null )return ;
      h.remove( vmp ) ;
      _changed.put( group , group ) ;
      return ;
   }
   /**
     * Removes the first appearance of mountpoint 'vmp' from 
     * the mountgroup set.
     */
   public void removeMountpoint( String vmp ){
       Hashtable      h     = null ;
       pnfsMountpoint mp    = null ;
       String         group = null ;
       for( Enumeration e = _groups.keys() ; e.hasMoreElements() ; ){
          group = (String)e.nextElement() ;
          h     = (Hashtable)_groups.get( group ) ;
          if( ( mp = (pnfsMountpoint)h.get( vmp ) ) != null )break ;
       }
       if( ( h == null ) || ( mp == null ) )return ;
       
       h.remove( vmp ) ;
       _changed.put( group , group ) ;
       return ;
   }
   /**
     * Returns an enumeration of all mountpoints in the specified
     * mountgroup.
     */
   public Enumeration mountpoints( String group ){
       Hashtable      groupHash ;
       
       if( ( groupHash = (Hashtable)_groups.get( group ) ) == null )
          return null ;
       
       return groupHash.elements() ;
   }      
   /**
     * Returns an enumeration of all mountgroup which have been
     * changed after loading.
     */
   public Enumeration changes(){  return _changed.elements() ; }      
   /**
     * Determines wheter a group has beeb changed or not after
     * the initial load operation.
     */
   public boolean isChanged( String group ){
     return _changed.get( group ) != null ;
   }      
   
   public String toString(){
      StringBuffer sb = new StringBuffer() ;
      String       keyName ;
      Hashtable    mountHash ;
      pnfsMountpoint mp ;
      for( Enumeration e = _groups.keys() ; e.hasMoreElements() ; ){
          keyName = (String)e.nextElement() ;
          sb.append( " --- "+keyName+" ----\n" ) ;
          mountHash = (Hashtable)_groups.get( keyName ) ;
          for( Enumeration l = mountHash.elements() ; l.hasMoreElements() ; ){
             mp = (pnfsMountpoint)l.nextElement() ;
             sb.append( "   "+mp.getVirtualMountpoint() +
                        "   "+mp.getRealMountpoint() + 
                        "   "+( mp.isIoEnabled()?"IOEnabled":"IODisabled")+"\n" ) ;   
          
          }
      
      }
      return sb.toString();
   
   }
   public static void main( String [] args ){
      if( args.length < 1 ){
        System.err.println( " Usage : pnfsMountgroup <groupDir>" ) ;
        System.exit(4);
      }
      pnfsMountgroup mp = new pnfsMountgroup() ;
      mp.load( args[0] ) ;
      System.out.println( " Mountgroup :\n" + mp ) ;
   }
}
