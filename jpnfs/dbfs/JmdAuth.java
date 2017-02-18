package jpnfs.dbfs ;

import  jpnfs.shmcom.* ;

import java.io.* ;

public class JmdAuth implements Serializable {
    private int _priv  = 15 ;
    
    public JmdAuth( int priv ){ _priv = priv ; }
    public JmdAuth(){}
    
    public String toString(){ return "(priv="+_priv+")" ; }
} 
