package jpnfs.pnfs ;

import  java.util.*;
import  java.io.* ;

/**
  *  pnfsMountgroup is a simple interface to the pnfs export facility.
  *  It contains a number of names mountgroups which themselfs contain
  *  a number of pnfsMountpoints.
  */
public class      PnfsMountgroupSet 
       extends    Hashtable 
       implements PnfsModifiable {

   private boolean _wasChanged = false ;
   private File    _dirBase    = null ;
   private Hashtable _removed  = new Hashtable() ;
   /**
     *   Constructs an empty mountgroupset.
     */
   public PnfsMountgroupSet(){}
   /**
     *   Constructs a mountgroup set and initializes it with
     *   the contents of the database pointed to by 'path'.
     */
   public PnfsMountgroupSet( File dir ) throws IOException {
      _dirBase = dir ;
      loadMountgroups( dir ) ;
   }
   /**
     *  Clears the current mountgroup set.
     */
   public synchronized void removeAll(){
     Enumeration e = keys() ;
     while( e.hasMoreElements() ){
        String groupName = (String)e.nextElement() ;
       _removed.put( groupName , groupName ) ;
     }
     removeAll() ; 
   }
   /**
     *   Adds the contents of the database 'path' to the
     *   content of the current mountgroup set.
     */
   public void loadMountgroups( File dir ) throws IOException {
      if( ! dir.isDirectory() )
        throw new IOException( "Not a directory" ) ;
        
      String [] groupNames = dir.list() ;

      for( int i = 0 ; i < groupNames.length ; i++ ){
      
         File mgFile = new File( dir , groupNames[i] ) ;
         
         if( ! mgFile.isFile() )continue ;
         try{
             put( groupNames[i] , new PnfsMountgroup( dir , groupNames[i] ) ) ;
         }catch( Exception ee ){ continue ; }
      
      }
   
   }
   /**
     *   adds the mountpoint 'mp' to the mountgroup 'group'.
     *   If 'group' doesn't exist, it will be created first.
     */
   public void addMountpoint( String groupName  , PnfsMountpoint mp ){
   
      PnfsMountgroup group = (PnfsMountgroup)get( groupName ) ;
      
      if( group == null )
         put( groupName , group = new PnfsMountgroup(groupName) ) ;
         
      group.addMountpoint( mp ) ;
      
   }
   /**
     *   adds an empty mountgroup to the current mountgroup set.
     */
   public void addMountgroup( String groupName ) throws IllegalArgumentException {
       
       if( get( groupName ) != null )
         throw new IllegalArgumentException( "Duplicated name : "+groupName ) ;
       
       _removed.remove( groupName ) ;
       put( groupName , new PnfsMountgroup( groupName ) ) ;
   }
   public PnfsMountgroup getMountgroup( String groupName ){
      return (PnfsMountgroup)get( groupName ) ;
   }
   public Enumeration getMountpoints( String groupName ){
      return ((PnfsMountgroup)get( groupName )).elements() ;
   }
   /**
     *   Tries to get the mountpoint with the virtual mountpoint 'vmp'
     *   from the group 'group'. null is returned it 'group' doesn't 
     *   contain 'vmp'
     */
   public PnfsMountpoint getMountpoint( String groupName , String vmp ){ 
       PnfsMountgroup group ;
       if( ( group = (PnfsMountgroup)get( groupName ) ) == null )return null ;
       return group.getMountpoint( vmp ) ;
   }
   /**
     * Tries to get the mountpoint with the virtual mountpoint 'vmp'
     * searching all groups. null is returned if the specified mountpoint
     * doesn't exists.
     */
   public PnfsMountpoint getMountpoint( String vmp ){
       PnfsMountpoint mp ;
       for( Enumeration e = elements() ; e.hasMoreElements() ; ){
          PnfsMountgroup group  = (PnfsMountgroup)e.nextElement() ;
          if( ( mp = group.getMountpoint( vmp ) ) != null )return mp ;
       }
       return null ;
   } 
   public void save() throws IOException {
      if( _dirBase == null )
        throw new IOException( "Not base directory" ) ;
        
      save( _dirBase ) ;
   }
   public void save( File dir ) throws IOException {
       for( Enumeration e = elements() ; e.hasMoreElements() ; ){
          PnfsMountgroup group  = (PnfsMountgroup)e.nextElement() ;
          group.save( new File (dir , group.getName() ) );
       }
       for( Enumeration e = _removed.keys() ; e.hasMoreElements() ; ){
          (new File( dir , (String)e.nextElement() ) ).delete() ;
       }
   } 
   /**
     * Tries to get the groupname of the first appearance of 
     * the mountpoint with the virtual mountpoint 'vmp'.
     */
   public PnfsMountgroup getGroupOfMountpoint( String vmp ){
       PnfsMountpoint mp ;
       for( Enumeration e = elements() ; e.hasMoreElements() ; ){
          PnfsMountgroup group  = (PnfsMountgroup)e.nextElement() ;
          if( group.getMountpoint( vmp ) != null )return group ;
       }
       return null ;
   }
   /**
     * Gets an enumeration of all group names.
     */
   public Enumeration groupNames(){ return keys() ; } 
   /**
     *  Tries to find the first appearance of the real mountpoint
     *  in all groups. The full mountpoint is returned or null if
     *  not found.
     
   public PnfsMountpoint find( String rmp ){
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
   */
   /**
     * Removes the mountgroup 'group' from the mount group set.
     */
   public synchronized void removeMountgroup( String groupName ){
      if( remove( groupName ) != null ){
         _removed.put( groupName , groupName ) ;
         wasChanged(true) ;
      }
      return ;
   }
   public String toString(){
      StringBuffer sb = new StringBuffer() ;
      String       keyName ;
      Hashtable    mountHash ;
      pnfsMountpoint mp ;
      for( Enumeration e = elements() ; e.hasMoreElements() ; ){
          PnfsMountgroup group = (PnfsMountgroup)e.nextElement() ;
          sb.append( "------------ "+group.getName()+" ---------------------\n" ) ;      
          sb.append( group.toString()+"\n" ) ;      
      }
      return sb.toString();
   
   }
   public void wasChanged( boolean t ){ _wasChanged = t ; }
   public boolean wasChanged(){ return _wasChanged ; }
   public static void main( String [] args ){
      if( args.length < 1 ){
        System.err.println( " Usage : PnfsMountgroupSet <groupDir>" ) ;
        System.exit(4);
      }
      try{
         PnfsMountgroupSet mgs = new PnfsMountgroupSet( new File( args[0] ) ) ;
         System.out.println( mgs.toString() ) ;
         mgs.addMountgroup( "h1" ) ;
         PnfsMountgroup mg = mgs.getMountgroup( "h1" ) ;
         mg.addMountpoint( new PnfsMountpoint( "/hallo" , "/0/hallo" ,true , 0 ) ) ;
         mgs.removeMountgroup( "Zeus" ) ;
         System.out.println( mgs.toString() ) ;
         mgs.save() ;
      }catch( Exception e ){
         System.err.println( "Exception "+e ) ;
         e.printStackTrace() ;
      }
   }
}
