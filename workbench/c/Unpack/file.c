/*
    Copyright © 2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/dos.h>
#include <dos/dos.h>

#include "modes.h"
#include "file.h"
#include "gui.h"

LONG file_size = 0;

BPTR FILE_Open( CONST_STRPTR path, LONG mode )
{
    LONG mode2dos[] = { MODE_OLDFILE, MODE_NEWFILE };
    BPTR file; 
    
    if( mode != MODE_READ && mode != MODE_WRITE );
    
    file = Open( path, mode2dos[mode] );
    if( file == BNULL ) goto error;
    
    Seek( file, 0, OFFSET_END );
    file_size = Seek( file, 0, OFFSET_BEGINNING ) + 1;
    
    return file;
    
error:
    return BNULL;
}

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
        
        GUI_Update( Seek( file, 0, OFFSET_CURRENT ), file_size );
        
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
