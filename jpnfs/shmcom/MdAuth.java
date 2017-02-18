package jpnfs.shmcom ;

/*
typedef struct {

        md_uid_t   uid ;        //      4   0
        md_gid_t   gid ;        //      4   4
        md_gid_t   gids[16] ;   // 16 * 4   8
           short   gidsLen ;    //      2  72
   unsigned long   host ;       //      4  74
            long   priv ;       //      4  78
   struct timeval  timestamp ;  //  2 * 4  82
         md_id_t   mountID ;    //  3 * 4  90
	 
   
} md_auth ;                     //    102



*/
public class MdAuth {
   private byte [] _data = new byte[104] ;
   public MdAuth(){
   
       jpnfs.shmcom.ShmCom.set4Data( _data , 78 , 15 ) ;
   }
   public byte [] getBytes(){ return _data ; }

}
