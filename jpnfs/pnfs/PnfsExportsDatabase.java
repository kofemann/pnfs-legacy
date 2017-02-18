package jpnfs.pnfs ;
import  java.util.* ;

public interface PnfsExportsDatabase {

  public Enumeration getGroupNames() ;
  public Enumeration getMountpoints( String groupName ) ;
  public PnfsMountpoint getMountpoint( String groupName , String vmp ) ;

}
