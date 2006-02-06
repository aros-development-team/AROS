/*
    Copyright © 2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/memory.h>
#include <dos/dos.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <aros/macros.h>

#include "support.h"
#include "modes.h"
#include "bzip2.h"
#include "file.h"
#include "package.h"
#include "gui.h"

#define PKG_BUFFER_SIZE (32*1024) /* 32kiB */

/** Low-level functions *****************************************************/

APTR PKG_Open( CONST_STRPTR filename, LONG mode )
{
    if( mode != MODE_READ && mode != MODE_WRITE ) return NULL;

    return BZ2_Open( filename, mode );
}

void PKG_Close( APTR pkg )
{
    if( pkg != NULL ) BZ2_Close( pkg );
}

LONG PKG_Read( APTR pkg, APTR buffer, LONG length )
{
    return BZ2_Read( pkg, buffer, length );
}

/** High-level functions ****************************************************/

UBYTE /* version */ PKG_ReadHeader( APTR pkg )
{
    UBYTE data[4] = { 0, 0, 0, 0 };
    /*LONG  actual  = */ PKG_Read( pkg, data, 4 );
    
    /* FIXME: error handling? naaaah... :S */
    
    return data[3];
}

LONG /* error */ PKG_ExtractFile( APTR pkg )
{
    LONG   pathLength, dataLength, rc, result;
    STRPTR path   = NULL;
    APTR   buffer = NULL;
    BPTR   output = NULL;
    
    /* Read the path length */
    rc = PKG_Read( pkg, &pathLength, sizeof( pathLength ) );
    pathLength = AROS_BE2LONG(pathLength);
    
    if( rc == -1 ) { result = -1; goto cleanup; }
    if( rc == 0  ) { result =  0; goto cleanup; }
    
    /* Read the path */
    path = AllocMem( pathLength + 1, MEMF_ANY );
    if( path == NULL ) { result = -1; goto cleanup; }
    rc = PKG_Read( pkg, path, pathLength + 1 );
    if( rc == -1 || rc == 0) { result = -1; goto cleanup; }
 
    /* Read the data lendth */
    rc = PKG_Read( pkg, &dataLength, sizeof( dataLength ) );
    dataLength = AROS_BE2LONG(dataLength);
    
    if( rc == -1 || rc == 0 ) { result = -1; goto cleanup; }
    
    //Printf( "Extracting %s (%ld bytes)...\n", path, dataLength );
    
    /* Make sure the destination directory exists */
    if( !MakeDirs( path ) ) { Printf("E:makedirs\n"); result = -1; goto cleanup; }
    
    /* Read and write the data in pieces */
    buffer = AllocMem( PKG_BUFFER_SIZE, MEMF_ANY );
    if( buffer == NULL ) { Printf("E:mem\n"); result = -1; goto cleanup; }
    output = Open( path, MODE_NEWFILE );
    if( output == NULL ) { Printf("E:create\n"); result = -1; goto cleanup; }
    
    {
        LONG total = 0;
        
        while( total < dataLength )
        {
            LONG length = 0;
            
            if( dataLength - total >= PKG_BUFFER_SIZE )
            {
                length = PKG_BUFFER_SIZE;
            }
            else
            {
                length = dataLength - total;
            }
            
            rc = PKG_Read( pkg, buffer, length );
            if( rc == -1 || rc == 0 ) { Printf("E:read\n"); result = -1; goto cleanup; }
            
            rc = FILE_Write( output, buffer, length );
            if( rc == -1 ) { Printf("E:write\n"); result = -1; goto cleanup; }
            
            total += length;
        }
    }
    
    result = 1;
    
cleanup:
    if( path != NULL )   FreeMem( path, pathLength + 1 );
    if( buffer != NULL ) FreeMem( buffer, PKG_BUFFER_SIZE );
    if( output != NULL ) Close( output );

    return result;
}

LONG /* error */ PKG_ExtractEverything( APTR pkg )
{
    LONG result;
    
    PKG_ReadHeader( pkg );
    
    result = PKG_ExtractFile( pkg );
    while( result != -1 && result != 0 )
    {
        result = PKG_ExtractFile( pkg );
    }
    
    return result;
}
