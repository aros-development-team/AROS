#include <proto/dos.h>
#include <dos/dos.h>

#include "file.h"

LONG FILE_Read( BPTR file, APTR buffer, LONG length )
{
    LONG read = 0, 
         left = length;
    
    while( read < length )
    {
        LONG actual = Read( file, buffer, left );
        if( actual == -1 ) return -1;
        
        read   += actual;
        buffer += actual;
        left   -= actual;
        
        if( actual == 0 ) break;
    }
    
    return read;
}

LONG FILE_Write( BPTR file, CONST_APTR buffer, LONG length )
{
    LONG written = 0,
         left    = length;
         
    while( written < length )
    {
        LONG actual = Write( file, buffer, left );
        if( actual == -1 ) return -1;
        
        written += actual;
        buffer  += actual;
        left    -= actual;
    }
    
    return written;
}

LONG FILE_Position( BPTR file )
{
    return Seek( file, 0, OFFSET_CURRENT );
}
