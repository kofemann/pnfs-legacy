package jpnfs.dbserver ;

import java.io.* ;
import jpnfs.dbfs.* ;

public class ReqObject implements Serializable {
   private int _command = 0 ;
   private int _timeout = 5 ;
   private int _answer  = 0 ;
   private int _status  = 0 ;
   public static final int DUMMY        = (1);
   public static final int FIRST        = (0x10);
   public static final int GETROOT      = (0x10);
   public static final int GET_RECORD   = (0x11);
   public static final int GET_ATTR     = (0x12);
   public static final int LOOKUP       = (0x13);
   public static final int MKDIR        = (0x14);
   public static final int SET_ATTR     = (0x15);
   public static final int RMDIR        = (0x16);
   public static final int READDIR      = (0x17);
   public static final int MKFILE       = (0x18);
   public static final int RMFILE       = (0x19);
   public static final int RENAME       = (0x1A);
   public static final int MAKELINK     = (0x1B);
   public static final int READLINK     = (0x1C);
   public static final int READDATA     = (0x1D);
   public static final int WRITEDATA    = (0x1E);
   public static final int SETSIZE      = (0x1F);
   public static final int SETPERM      = (0x20);
   public static final int TRUNCATE     = (0x21);
   public static final int RM_FROM_DIR  = (0x22);
   public static final int ADD_TO_DIR   = (0x23);
   public static final int CH_PARENT    = (0x24);
   public static final int DEL_OBJECT   = (0x25);
   public static final int FORCE_SIZE   = (0x26);
   public static final int BACKUP       = (0x27);
   public static final int LOOKUP_ONLY  = (0x28);
   public static final int I_COMMAND    = (0x29);
   public static final int GET_CHAIN    = (0x2a);
   public static final int FIND_ID      = (0x2b);
   public static final int MOD_LINK     = (0x2c);
   public static final int SET_ATTRS    = (0x2d);
   public static final int LAST         = (0x2d);
   
   public ReqObject(){}
   public ReqObject( int command ){ 
      _command = command ;
   }
   public ReqObject( int command , int timeout ){
      _command = command ;
      _timeout = timeout ;
   }
   public int getStatus(){ return _status ; }
   public int getAnswer(){ return _answer ; }
   public int getCommand(){ return _command ; }
   public void setTimeout( int seconds ){ _timeout = seconds ; }
   public void setCommand( int command ){ _command = command ; }
   public String toString(){
      return "(c="+_command+
             ";t="+_timeout+
             ";s="+_status+
             ";a="+_answer+")" ;
   }  
}


