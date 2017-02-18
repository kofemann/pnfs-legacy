/*
 * $Id: JGdbm.java,v 1.1 2006-01-26 16:59:00 tigran Exp $
 */

/*
 * gdbm to SQL conversion utility. reads gdbm files, recovers entries
 * and stores them into SQL database.
 */
import java.io.* ;
import java.nio.*;
import java.nio.channels.*;
import java.util.* ;
import java.sql.*;


public class JGdbm {

    private BORandomAccessFile _random     = null ;
    private FileHeader         _fileHeader = null ;
    private Bucket         _currentBucket      = null ;
    private int            _currentBucketIndex = -1 ;
    private int            _currentElementLocation = -1 ;
    private BucketElement  _currentElement         = null ;
    
    public class Entry {
       private int _keySize  = 0 ;
       private int _dataSize = 0 ;
       private byte [] _key  = null ;
       private byte [] _data = null ;
       public byte [] getData(){ return _data ; }
       public byte [] getKey(){ return _key ; }
       public String toString(){
          StringBuffer sb = new StringBuffer() ;
          if( _key == null )return "NULL" ;
          //sb.append("Key=").append(new String(_key)) ;
          sb.append("Key=").append(_key.length) ;
          sb.append(";Value=");
          if( _data == null )sb.append("NULL") ;
          //else sb.append(new String(_data));
          else sb.append(_data.length);
          return sb.toString() ;
       }
    }
    //
    //  file access class (gdbm compatible mode)
    //
    private class BORandomAccessFile {
    
       private RandomAccessFile _file = null ;
       
       MappedByteBuffer _mappedByteBuffer = null;
       
       public BORandomAccessFile( RandomAccessFile file )throws IOException {
          _file = file ;
          FileChannel fileChannel = file.getChannel();
          _mappedByteBuffer = fileChannel.map(FileChannel.MapMode.READ_ONLY, 0, file.length());          
       }
       
       public int readInt() throws IOException {
          // int i = _file.readInt() ;
          int i = _mappedByteBuffer.getInt();
          int x = 
          ( (i >> 24 ) & 0xFF     ) |
          ( (i >>  8 ) & 0xFF00   ) |
          ( (i <<  8 ) & 0xFF0000 ) |
          ( (i << 24 ) & 0xFF000000 )  ;
       
          return i ;
       }
       
       public void seek( long position ) throws IOException {
          //_file.seek(position);
          _mappedByteBuffer.position((int)position);
       }
       
       public int read( byte [] data , int offset , int size ) throws IOException {
          // return _file.read( data , offset , size ) ;
           int i;
           int realSize = _mappedByteBuffer.remaining() > size ? size : _mappedByteBuffer.remaining() ;
                                                
            _mappedByteBuffer.get(data, offset, realSize);
       
           return realSize;
       }
       
       
    }
    private static final int BUCKET_AVAILABLE = 6 ;
    private static final int SMALL_KEY        = 4 ;
    private class BucketElement {
       private int  _hashValue   = 0 ;
       private byte [] _keyStart = new byte[SMALL_KEY] ;
       private long _dataPointer = 0L ;
       private int  _keySize     = 0 ;
       private int  _dataSize    = 0 ;
       private BucketElement( BORandomAccessFile file )throws IOException {
          _hashValue   = file.readInt() ;
          file.read( _keyStart , 0 , _keyStart.length ) ;
          _dataPointer = (long)file.readInt() ;
          _keySize     = file.readInt() ;
          _dataSize    = file.readInt() ;
       }
       public String toString(){
          StringBuffer sb = new StringBuffer() ;
          sb.append("BE={h=").append(Integer.toString(_hashValue,16)).
             append(";");
          for(int i = 0 ; i < SMALL_KEY ; i++ ){
             sb.append(Integer.toString((int)_keyStart[i],16)).
                append(";");
          }
          sb.append("p=").append(Long.toString(_dataPointer,16)).
             append(";ks=").append(Integer.toString(_keySize)).
             append(";ds=").append(Integer.toString(_dataSize)).
             append(";}");
             
          return sb.toString();
       }
       public void dump(){
          System.out.println(""+_dataPointer+" "+_keySize+" " + _dataSize );
       }
       public void dump( DataOutputStream out )throws IOException {
            int rc  = 0 ;
            if( _dataPointer == 0L )return ;
            byte [] data = new byte[_keySize+_dataSize];
            try{
               _random.seek( _dataPointer );  
               rc = _random.read(data,0,data.length);
            }catch(IOException ioe ){
               System.err.println("Can't read data at ("+_dataPointer+","+_keySize+"," + _dataSize+") : "+ioe);
               return;
            }
            if( rc  <  data.length ){
               System.err.println("Can't read data at ("+_dataPointer+","+_keySize+"," + _dataSize+") short read");
               return;
            }
            out.writeInt(_keySize+_dataSize);
            out.writeInt(_keySize);
            out.write( data , 0 , _keySize ) ;
            out.writeInt(_dataSize);
            out.write( data , _keySize , _dataSize ) ;
       }       


        public void dump( java.sql.Connection con, String tableName )throws IOException {
            int rc  = 0 ;
            if( _dataPointer == 0L )return ;
            byte [] key = new byte[_keySize];
            byte [] data = new byte[_dataSize];
            
            String insertSQL = "INSERT INTO " + tableName + " VALUES( ?, ? , NOW() )";
            // String insertSQL = "INSERT INTO pnfs VALUES( ?, ? , NOW() )";
            
            try{
               _random.seek( _dataPointer );  
               rc = _random.read(key,0,key.length);
               rc = _random.read(data,0,data.length);
            }catch(IOException ioe ){
               System.err.println("Can't read data at ("+_dataPointer+","+_keySize+"," + _dataSize+") : "+ioe);
               return;
            }
            if( rc  <  data.length ){
               System.err.println("Can't read data at ("+_dataPointer+","+_keySize+"," + _dataSize+") short read");
               return;
            }
		
            
            try {			                

                PreparedStatement ps = con.prepareStatement(insertSQL);                
                
                ps.setBytes(1, key);
                ps.setBytes(2, data);                
                
                
            //    for( int i = 0; i < key.length; i++) {
            //        System.out.print( toHex( key[i] ).toUpperCase() );
            //    }
            //    
            //    System.out.println("");
                ps.executeUpdate();
            
            }catch( SQLException e) {
                //throw new IOException( e.getMessage() );
             //   System.out.println( e.getMessage() );
            }

        }
                
        
        private String toHex(byte b) {
            Integer I = new Integer((((int)b) << 24) >>> 24);
            int i = I.intValue();

            if ( i < (byte)16 ) {
                return "0"+Integer.toString(i, 16);
            } else {
               return     Integer.toString(i, 16);
            }        
        }
    
    }   


    private class Bucket {
       private int _availableCount = 0 ;
       private int _bucketBits     = 0 ;
       private int _count          = 0 ;
       private BucketElement    [] _elements           = null ;
       private AvailableElement [] _availableElements  = new AvailableElement[BUCKET_AVAILABLE] ;
       public Bucket( BORandomAccessFile file ) throws IOException{
          _availableCount = file.readInt() ;
          for( int i = 0 ; i < BUCKET_AVAILABLE ; i++ ){
          
             int  size = file.readInt() ;
             long addr = (long)file.readInt() ;
             _availableElements[i] = new AvailableElement(addr,size) ;
             
          }
          _bucketBits = file.readInt() ;
          _count      = file.readInt() ;
          _elements   = new BucketElement[_fileHeader._bucketElements];
          for( int i = 0 ; i < _fileHeader._bucketElements ; i++ ){
             _elements[i] = new BucketElement(file) ;
          }
       }
       public void dump( DataOutputStream out ) throws IOException {

          for( int i = 0 ; i < _fileHeader._bucketElements ; i++ ){
             if( _elements[i]._hashValue == -1 )continue ;
             _elements[i].dump( out );
          }
       }
      
       public void dump( ) throws IOException {

          for( int i = 0 ; i < _fileHeader._bucketElements ; i++ ){
             if( _elements[i]._hashValue == -1 )continue ;
             _elements[i].dump( );
          }
       }

 
       public void dump( java.sql.Connection con , String tableName) throws IOException {

          for( int i = 0 ; i < _fileHeader._bucketElements ; i++ ){
             if( _elements[i]._hashValue == -1 )continue ;
             _elements[i].dump( con , tableName);
          }
       }
       
       
       public void dumpBucket(){

          for( int i = 0 ; i < _fileHeader._bucketElements ; i++ ){
             if( _elements[i]._hashValue == -1 )continue ;
             System.out.print("   be "+i+" ");
             _elements[i].dump();
          }
       }
       public String toString(){
          StringBuffer sb = new StringBuffer() ;
          sb.append("Bucket\n").
             append("   Bits    ").append(_bucketBits).append("\n").
             append("   Count   ").append(_count).append("/").
             append(_fileHeader._bucketElements).append("\n").
             append("   AvCount ").append(_availableCount).
             append("/").append(BUCKET_AVAILABLE).append("\n");
          for( int i = 0 ; i < _fileHeader._bucketElements ; i++ ){
             if( _elements[i]._hashValue == -1 )continue ;
             sb.append("     B=(").
                append(i).append(") ").append(_elements[i].toString()).append("\n");
          }
          for( int i = 0 ; i < _availableCount ; i++ ){
             sb.append("     A= ").append(_availableElements[i].toString()).append("\n");
          }
          return sb.toString();
       }
    }
    private class FileHeader {
       //
       // in file structure
       //
       private int  _magic     = 0 ;
       private int  _blockSize = 0 ;
       private long _directoryPosition = 0L ;
       private int  _directorySize     = 0 ;
       private int  _directoryBits     = 0 ;
       private int  _bucketSize        = 0 ;
       private int  _bucketElements    = 0 ;
       private long _nextBlock         = 0 ;
       //
       // in memeory steering
       //
       private BORandomAccessFile _file = null ;
       private long [] _hashDirectory   = null ;
       
       private AvailableBlock _headAvailableBlock = null ;
         
       public FileHeader( BORandomAccessFile file )throws IOException {
           _file = file ;
           //
           // file header structure
           //
           _magic             = file.readInt() ;
           _blockSize         = file.readInt() ;
           _directoryPosition = (long)file.readInt() ;
           _directorySize     = file.readInt() ;
           _directoryBits     = file.readInt() ;
           _bucketSize        = file.readInt() ;
           _bucketElements    = file.readInt() ;
           _nextBlock         = (long)file.readInt() ;
           //
           // more
           //

           readHeadAvailableBlock() ;
           _random.seek(_directoryPosition);
           readHashDirectory(  ) ;
       }
       private void readHashDirectory() throws IOException{
           _hashDirectory = new long[_directorySize/4] ;
           _file.seek(_directoryPosition) ;
           for( int i = 0 ; i < _hashDirectory.length ; i++ ){
              _hashDirectory[i] = (long)_file.readInt() ;
           }
       }
       private void readHeadAvailableBlock() throws IOException {
           _headAvailableBlock = new AvailableBlock( _file ) ;
       }
       public String toString(){
          StringBuffer sb = new StringBuffer() ;
          sb.append("File Header\n");
          sb.append("  Magic             = ").
             append(Integer.toString(_magic,16)).
             append("\n") ;
          sb.append("  BlockSize         = ").
             append(_blockSize).append("\n");
          sb.append("  DirectoryPosition = 0x").
             append(Long.toString(_directoryPosition,16)).append("\n");
          sb.append("  DirectorySize     = ").
             append(_directorySize).append("\n");
          sb.append("  DirectoryBits     = ").
             append(_directoryBits).append("\n");
          sb.append("  BucketSize        = ").
             append(_bucketSize).append("\n");
          sb.append("  BucketElements    = ").
             append(_bucketElements).append("\n");
          sb.append("  nextBlock         = 0x").
             append(Long.toString(_nextBlock,16)).append("\n");
          sb.append(_headAvailableBlock.toString()) ;
          /*
          try{
              _random.seek(_hashDirectory[0]);
             Bucket bucket = new Bucket(_random);
             System.out.println("bucket ok");
             sb.append(bucket.toString()).
                append("\n");
          }catch(Exception ee){
             sb.append("Exception in reading bucket @ ").
                append(_hashDirectory[0]).
                append("\n");
          }
          */
          return sb.toString() ;
       }
       public void dump( DataOutputStream out ) throws IOException {
          for( int i = 0 ; i < _hashDirectory.length ; i++ ){
             long pos = _hashDirectory[i] ;
             System.out.print(" bucket "+i+" ");    
             if( pos == 0L ){
                System.err.println(" bucket "+i+" NULL");    
                continue ;
             }
             Bucket bucket = null ;
             try{
                _random.seek(pos);
                bucket = new Bucket(_random);
             }catch(Exception ee){
                System.err.println(" bucket "+i+" exception "+ee);    
                continue ;
             }
             bucket.dump( out );
          }
       }

       public void dump( ) throws IOException {
          for( int i = 0 ; i < _hashDirectory.length ; i++ ){
             long pos = _hashDirectory[i] ;
             System.out.print(" bucket "+i+" ");
             if( pos == 0L ){
                System.err.println(" bucket "+i+" NULL");
                continue ;
             }
             Bucket bucket = null ;
             try{
                _random.seek(pos);
                bucket = new Bucket(_random);
             }catch(Exception ee){
                System.err.println(" bucket "+i+" exception "+ee);
                continue ;
             }
             bucket.dump();
          }
       }
       
       
       public void dump( java.sql.Connection con , String tableName) throws IOException {
          for( int i = 0 ; i < _hashDirectory.length ; i++ ){
             long pos = _hashDirectory[i] ;
             System.out.println(" bucket "+i+" ");    
             if( pos == 0L ){
                System.err.println(" bucket "+i+" NULL");    
                continue ;
             }
             Bucket bucket = null ;
             try{
                _random.seek(pos);
                bucket = new Bucket(_random);
             }catch(Exception ee){
                System.err.println(" bucket "+i+" exception "+ee);    
                continue ;
             }
             bucket.dump( con , tableName);
             try {
                con.commit();
             }catch(SQLException e) {
                 throw new IOException( e.getMessage() );
             }
          }
       }
       
       
       public void dumpBuckets(){
          System.out.println(toString()) ;
          for( int i = 0 ; i < _hashDirectory.length ; i++ ){
             long pos = _hashDirectory[i] ;
             System.out.print(" bucket "+i+" ");    
             if( pos == 0L ){
                System.out.println("NULL");    
                continue ;
             }
             try{
                _random.seek(pos);
                Bucket bucket = new Bucket(_random);
                System.out.println(pos);
                bucket.dumpBucket();
             }catch(Exception ee){
                System.out.println("exception : "+ee);
             }
          }
       }
    }
    private class AvailableElement {
       private int _size     = 0 ;
       private long _address = 0 ;
       public AvailableElement( long address , int size ){
          _address = address ;
          _size    = size ;
       }
       public String toString(){
          return "[AE:0x"+Long.toString(_address,16)+":"+_size+"]";
       }
    }
    private class AvailableBlock {
       private int _size  = 0 ;
       private int _count = 0 ;
       private long _nextBlock = 0L ;
       private AvailableElement [] _elements = null ;
       public AvailableBlock( BORandomAccessFile file ) throws IOException{
       
          _size      = file.readInt() ;
          _count     = file.readInt() ;
          _nextBlock = (long)file.readInt() ;
          _elements  = new AvailableElement[_size] ;
          
          for( int i = 0 ; i < _count ; i++ ){
             int  size = file.readInt() ;
             long addr = (long)file.readInt() ;
             _elements[i] = new AvailableElement(addr,size) ;
          }
       }
       public String toString(){
          StringBuffer sb = new StringBuffer() ;
          sb.append("Available Buffer\n") ;
          sb.append("   Size/Count = ").append(_size).
             append("/").append(_count).append("\n");
          sb.append("   Next Block = 0x").append(Long.toString(_nextBlock)).
             append("\n") ;
             
          for( int i = 0 ; i < _count ; i++ ){
             sb.append("   ").append(_elements[i]).append("\n");
          }
          return sb.toString() ;
       }
    }
    private int getHash(byte [] key ){
//       System.out.println("Key len : "+key.length);
       long value = (int)((0x238F13AFL * (long)key.length)&0x7FFFFFFF) ;
//       System.out.println("H "+Long.toString(value,16));
       for( int i = 0 , n = key.length ; i < n ; i ++ ){
           value = (value + (key[i] << (i *5 % 24) ) ) & 0x7FFFFFFF;
//           System.out.println("H "+Long.toString(value,16));
       }
       value = (1103515243L * value + 12345L) & 0x7FFFFFFFL;
//       System.out.println("H "+Long.toString(value,16));

       return (int) value ;
    }
    private boolean isEqual( byte [] a , byte [] b , int size ){
       int i = 0 ;
       for( i = 0 ; ( i < size ) && ( a[i] == b[i] ) ; i ++ ) ;
       return i == size ;
    }
    private Bucket getNextBucket() throws IOException {

       long pos = _fileHeader._hashDirectory[_currentBucketIndex] ;
       int  index = 0 ;
       for( index = _currentBucketIndex + 1 ;
            ( index < _fileHeader._hashDirectory.length ) &&
            ( pos == _fileHeader._hashDirectory[index]  ) ;
            index++ ) ;
            
       if( index == _fileHeader._hashDirectory.length )return null ;
       
       return getBucketByDirectoryIndex( index ) ;
    }
    private Bucket getBucketByDirectoryIndex( int index ) throws IOException {
    
    
       _currentElementLocation = -1 ;
       _currentElement         = null ;
       
       if( index == _currentBucketIndex )return _currentBucket ;
       
       long previousPosition = _currentBucketIndex < 0 ?
                               -1 :
                               _fileHeader._hashDirectory[_currentBucketIndex] ;
       
       long pos = _fileHeader._hashDirectory[_currentBucketIndex=index] ;
       
       if( previousPosition == pos )return _currentBucket ;
       
       _random.seek(pos) ;
       
       _currentBucket = new Bucket(_random) ;
       
       System.out.println("New Bucket loaded : index="+_currentBucketIndex+
                          " pos="+Long.toString(pos,1));
//       System.out.println(_currentBucket.toString());
    
       return _currentBucket ;
    }
    private Entry getNextEntry() throws IOException {
    
       boolean found = false ;
       
       while( ! found ){
       
          _currentElementLocation++ ;
          if( _currentElementLocation == _fileHeader._bucketElements ){
             
             if( getNextBucket() == null ) {
                 return null ;
             }
             _currentElementLocation++ ;
             
          }
//          System.out.println("Searching bucket element : "+_currentElementLocation+" : "+
//             _currentBucket._elements[_currentElementLocation]._hashValue);
          found = ( _currentElement = _currentBucket._elements[_currentElementLocation] )
                ._hashValue != -1 ;
       }
       
       return readCurrentEntry() ;
    }
    public Entry first() throws IOException {

       Bucket bucket = getBucketByDirectoryIndex(0) ;
    
       return getNextEntry() ;
    }
    public Entry next( Entry entry ) throws IOException {
    
       if( ( entry       == null ) || 
           ( entry._key  == null ) || 
           ( entry._data == null ) ) {
               return null ;
       }
           
       get( entry._key ) ;
       return getNextEntry() ;
    }
    public Entry get( byte [] key ) throws IOException {
    
       int hashValue = getHash( key ) ;
       
//       System.out.println("HashValue of : "+(new String(key))+" : "+
//                          Integer.toString(hashValue,16));
                          
       int directoryHashLookup = hashValue >> ( 31 - _fileHeader._directoryBits ) ;
       
       Bucket bucket = getBucketByDirectoryIndex(directoryHashLookup) ;

       _currentElementLocation = hashValue % _fileHeader._bucketElements ;
       int homeLocation        = _currentElementLocation ;
       _currentElement         = bucket._elements[_currentElementLocation] ;
       
       int bucketHashValue     = _currentElement._hashValue;
       
//       System.out.println("Initial bucketHashValue : 0x"+
//                           Long.toString(bucketHashValue));
                           
       while( bucketHashValue != -1 ){
       
//          System.out.println("ElementLocation : "+
//                             elementLocation+" / "+
//                             Long.toString(bucketHashValue,16)) ;
                             
          int keySize = _currentElement._keySize ;
          
//          System.out.println("Key Size : "+keySize+" - "+key.length);
          
          if( ( bucketHashValue != hashValue    ) ||
              ( keySize         != key.length   ) ||
              ( ! isEqual( key , 
                         _currentElement._keyStart ,
                         keySize > SMALL_KEY ? SMALL_KEY : keySize ) ) ){
             
              _currentElementLocation = 
                   ( _currentElementLocation + 1 ) % _fileHeader._bucketElements ;       

              if( _currentElementLocation == homeLocation )return null ;
              
              bucketHashValue =
                  (_currentElement = bucket._elements[_currentElementLocation])._hashValue ;
          }else{
              
              Entry entry = readCurrentEntry() ;
              
              if( isEqual( entry._key , key , keySize ) ){
                  
                  return entry ;
                  
              }else{
                  _currentElementLocation = 
                       ( _currentElementLocation + 1 ) % _fileHeader._bucketElements ;
                  if( _currentElementLocation == homeLocation ) {
                      return null ;
                  }

                  bucketHashValue = 
                      (_currentElement = bucket._elements[_currentElementLocation])._hashValue ;
             }
          }
       }
       return null ;
    }
    private Entry readCurrentEntry() throws IOException {
    
       _random.seek(_currentElement._dataPointer) ;
       
       Entry entry = new Entry() ;
       
       int size = _currentElement._keySize ;
       entry._key = new byte[size] ;
       _random.read( entry._key , 0 , size ) ;
       size = _currentElement._dataSize ;
       entry._data = new byte[size] ;
       _random.read( entry._data , 0 , size ) ;
       
       return entry ;
    
    }
    public JGdbm( File file , boolean readWrite ) throws Exception {
    
        if( ! file.exists() ) {
          throw new
          IllegalArgumentException("File not found : "+file) ;
        }
          
        _random = new BORandomAccessFile( new RandomAccessFile(file,"r") ) ;
        
        _fileHeader = new FileHeader( _random ) ;
        System.out.println(_fileHeader.toString() );
        //_fileHeader.dumpBuckets();

    }
    public void dump( DataOutputStream out ) throws IOException {
       _fileHeader.dump(out);
    }


    public void dump() throws IOException {
       _fileHeader.dump();
    }

    public void dump( java.sql.Connection con , String tableName) throws IOException {
       _fileHeader.dump(con, tableName);
    }
    
    public static void main( String [] args ) throws Exception {
    
        if( args.length < 1 ){
           System.err.println("Usage : ... <filename> [<binfile>]");
           System.exit(4);
        }
        File file = new File(args[0]);
        JGdbm j = new JGdbm( file , false ) ;
        if( args.length > 2 ) {
            System.exit(3);
        }
/*
        if( args.length > 1 ){
           byte [] input = args[1].getBytes() ;
           byte [] array = new byte[input.length+1] ;
           System.arraycopy(input,0,array,0,input.length);
           JGdbm.Entry result = j.get( array ) ;
           
           if( result == null )System.out.println("Result is 'null'");
           else System.out.println( new String(result.getData()) ) ;
        }
*/
        DataOutputStream out = null ; 
        if( args.length > 1 ){

            out =  new DataOutputStream( new BufferedOutputStream( new FileOutputStream( new File( args[1] ) ) ) );
            j.dump( out ) ;        
        } else {
            j.dump();
        }
       
/*
        System.out.println("LOOP");
        for( Entry e = j.first() ; e != null ; e = j.next(e) ){
           System.out.println(e.toString());
           if( out != null ){
                byte [] key = e.getKey() ;
                byte [] data = e.getData() ;
                out.writeInt( key.length + data.length ) ;
                out.writeInt( key.length ) ;
                out.write( key , 0 , key.length) ;
                out.writeInt( data.length ) ;
                out.write( data , 0 , data.length ) ;
            
           }
        }
*/
        
//        System.out.println(j.toString());
        
       if( out != null ){
           out.close() ;
           out.flush() ;
       }
       System.exit(0);
    }
}
