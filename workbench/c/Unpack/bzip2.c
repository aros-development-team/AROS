/*
    Copyright © 2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>

#include "modes.h"
#include "file.h"
#include "bzip2.h"
#include "bzip2_private.h"

#define BZIP2_BUFFER_SIZE (32*1024)    /* 32 kiB */

int bz_internal_error;

void *malloc( size_t size )
{
    return AllocVec( size, MEMF_ANY );
}

void free( void *p )
{
    FreeVec( p );
}

APTR BZ2_Open( CONST_STRPTR path, LONG mode )
{
    struct bzFile *bzf = NULL;
    int            rc;
    
    if( mode != MODE_READ ) goto error;
    
    bzf = AllocMem( sizeof( struct bzFile ), MEMF_ANY | MEMF_CLEAR );
    if( bzf == NULL ) goto error;
    
    bzf->bzf_Buffer = AllocMem( BZIP2_BUFFER_SIZE, MEMF_ANY );
    if( bzf->bzf_Buffer == NULL ) goto error;
    bzf->bzf_BufferAmount = 0;
    
    bzf->bzf_File = FILE_Open( path, MODE_READ );
    if( bzf->bzf_File == NULL ) goto error;    

    rc = BZ2_bzDecompressInit( &(bzf->bzf_Stream), 0, 0 );
    if( rc != BZ_OK ) goto error;
    
    bzf->bzf_Stream.next_in  = bzf->bzf_Buffer;
    bzf->bzf_Stream.avail_in = bzf->bzf_BufferAmount;
    
    return bzf;
    
error:
    //printf( "BZ2_Open: Error!\n" );
    
    BZ2_Close( bzf );
    
    return NULL;
}

void BZ2_Close( APTR file )
{
    struct bzFile *bzf = (struct bzFile *) file;
    
    if( bzf != NULL )
    {
        if( bzf->bzf_File )   Close( bzf->bzf_File );
        if( bzf->bzf_Buffer ) FreeMem( bzf->bzf_Buffer, BZIP2_BUFFER_SIZE );
        
        BZ2_bzDecompressEnd( &(bzf->bzf_Stream) );
        
        FreeMem( bzf, sizeof( struct bzFile ) );
    }
}

LONG BZ2_Read( APTR file, APTR buffer, LONG length )
{
    struct bzFile *bzf  = (struct bzFile *) file;
    LONG           read = 0;
    int            rc;
    
    bzf->bzf_Stream.next_out  = buffer;
    bzf->bzf_Stream.avail_out = length;
    
    while( TRUE )
    {
        if( bzf->bzf_Stream.avail_in == 0 )
        {
            read = FILE_Read( bzf->bzf_File, bzf->bzf_Buffer, BZIP2_BUFFER_SIZE );
            if( read == -1 ) goto error;
            if( read == 0 )  return 0;          /* EOF */
            bzf->bzf_BufferAmount     = read;
            
            bzf->bzf_Stream.next_in   = bzf->bzf_Buffer;
            bzf->bzf_Stream.avail_in  = bzf->bzf_BufferAmount;
        }
        
        rc = BZ2_bzDecompress( &(bzf->bzf_Stream) );
        
        if( rc != BZ_OK && rc != BZ_STREAM_END ) goto error;
        
        if( rc == BZ_STREAM_END )
        {
            return length - bzf->bzf_Stream.avail_out;
        }
        
        if( bzf->bzf_Stream.avail_out == 0 ) 
        {
            return length;
        }
    }
    
error:

    return -1;
}
