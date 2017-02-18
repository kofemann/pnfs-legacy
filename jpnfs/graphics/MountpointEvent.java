package jpnfs.graphics ;
import  jpnfs.pnfs.PnfsMountpoint ;

public class MountpointEvent extends PnfsEvent {
  PnfsMountpoint _mountpoint ;
  public MountpointEvent( Object source , PnfsMountpoint mp ){
    super( source , "findMountpoint" ) ;
    _mountpoint = (PnfsMountpoint)mp.clone() ;
  }
  public PnfsMountpoint getMountpoint(){ return _mountpoint ; }

} 
