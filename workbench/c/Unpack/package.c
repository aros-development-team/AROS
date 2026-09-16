/*
    Copyright (C) 2003, 2009, The AROS Development Team. All rights reserved.
*/

#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/dosextens.h>

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
#define PKG_VERSION 1

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

static BPTR PKG_LockAt( BPTR directory, CONST_STRPTR name, LONG mode )
{
    BPTR oldDir = CurrentDir( directory );
    BPTR lock = Lock( name, mode );

    CurrentDir( oldDir );
    return lock;
}

static BPTR PKG_CreateDirAt( BPTR directory, CONST_STRPTR name )
{
    BPTR oldDir = CurrentDir( directory );
    BPTR lock = CreateDir( name );

    CurrentDir( oldDir );
    return lock;
}

static BPTR PKG_OpenAt( BPTR directory, CONST_STRPTR name, LONG mode )
{
    BPTR oldDir = CurrentDir( directory );
    BPTR file = Open( name, mode );

    CurrentDir( oldDir );
    return file;
}

static BOOL PKG_PathIsRelative( CONST_STRPTR path )
{
    BOOL haveComponent = FALSE;

    if( path == NULL || *path == '\0' )
        return FALSE;

    while( *path != '\0' )
    {
        if( *path == ':' )
            return FALSE;

        if( *path == '/' )
        {
            if( !haveComponent )
                return FALSE;

            haveComponent = FALSE;
        }
        else
        {
            haveComponent = TRUE;
        }

        path++;
    }

    return haveComponent;
}

static BOOL PKG_LockIsDirectory( BPTR lock )
{
    struct FileInfoBlock info;

    if( !Examine( lock, &info ) )
        return FALSE;

    return info.fib_DirEntryType > 0;
}

static BOOL PKG_LockIsContained( BPTR root, BPTR lock )
{
    BPTR current = DupLock( lock );

    if( current == BNULL )
        return FALSE;

    while( current != BNULL )
    {
        BPTR parent;

        if( SameLock( root, current ) == LOCK_SAME )
        {
            UnLock( current );
            return TRUE;
        }

        parent = ParentDir( current );
        UnLock( current );
        current = parent;
    }

    return FALSE;
}

/*
 * Returns 1 for a soft link, 0 when no soft link was established and -1
 * when the filesystem could not answer safely.
 */
static LONG PKG_SoftLinkStateAt( BPTR directory, CONST_STRPTR name )
{
    struct FileLock *lock = (struct FileLock *)BADDR( directory );
    TEXT buffer[1];
    LONG result;
    LONG error;

    result = ReadLink( lock->fl_Task, directory, name, buffer, sizeof( buffer ) );
    if( result >= 0 || result == -2 )
        return 1;

    error = IoErr();
    if( error == ERROR_BUFFER_OVERFLOW )
        return 1;

    if( error == ERROR_OBJECT_NOT_FOUND ||
        error == ERROR_OBJECT_WRONG_TYPE ||
        error == ERROR_ACTION_NOT_KNOWN )
    {
        return 0;
    }

    return -1;
}

static BPTR PKG_LockOrCreateDir( BPTR root, BPTR directory, STRPTR name )
{
    BPTR next = PKG_LockAt( directory, name, SHARED_LOCK );

    if( next == BNULL )
    {
        LONG error = IoErr();

        if( error != ERROR_OBJECT_NOT_FOUND )
            return BNULL;

        if( PKG_SoftLinkStateAt( directory, name ) != 0 )
            return BNULL;

        next = PKG_CreateDirAt( directory, name );
        if( next == BNULL )
            return BNULL;

        /*
         * CreateDir() returns an exclusive lock. Release it and reacquire
         * the new directory as a normal shared traversal lock.
         */
        UnLock( next );
        next = PKG_LockAt( directory, name, SHARED_LOCK );
        if( next == BNULL )
            return BNULL;
    }

    if( !PKG_LockIsDirectory( next ) ||
        !PKG_LockIsContained( root, next ) )
    {
        UnLock( next );
        return BNULL;
    }

    return next;
}

static BPTR PKG_OpenContained( BPTR root, STRPTR path )
{
    BPTR directory = BNULL;
    BPTR output = BNULL;
    STRPTR component;
    STRPTR cursor;

    if( !PKG_PathIsRelative( path ) ||
        !PKG_LockIsDirectory( root ) )
    {
        return BNULL;
    }

    directory = DupLock( root );
    if( directory == BNULL )
        return BNULL;

    component = path;
    cursor = path;

    for( ;; )
    {
        if( *cursor == '/' )
        {
            BPTR next;

            *cursor = '\0';
            next = PKG_LockOrCreateDir( root, directory, component );
            *cursor = '/';

            if( next == BNULL )
                goto cleanup;

            UnLock( directory );
            directory = next;
            component = cursor + 1;
        }
        else if( *cursor == '\0' )
        {
            LONG linkState = PKG_SoftLinkStateAt( directory, component );

            if( linkState < 0 )
                goto cleanup;

            /*
             * Open(MODE_NEWFILE) replaces a hard-link leaf in its current
             * directory, but follows a soft link. Only soft-link leaves need
             * target containment here; directory components were already
             * checked while traversing their resolved locks above.
             */
            if( linkState > 0 )
            {
                BPTR target = PKG_LockAt( directory, component, SHARED_LOCK );

                if( target == BNULL )
                    goto cleanup;

                if( !PKG_LockIsContained( root, target ) )
                {
                    UnLock( target );
                    goto cleanup;
                }

                UnLock( target );
            }

            output = PKG_OpenAt( directory, component, MODE_NEWFILE );
            goto cleanup;
        }

        cursor++;
    }

cleanup:
    if( directory != BNULL )
        UnLock( directory );

    return output;
}

/** High-level functions ****************************************************/

LONG /* version */ PKG_ReadHeader( APTR pkg )
{
    UBYTE data[4] = { 0, 0, 0, 0 };
    ULONG packageSize;
    LONG  version;

    if ( PKG_Read( pkg, data, 4 ) != 4 )
    {
        Printf("E:read header\n");
        return -1;
    }

    version = data[3];
    if ( data[0] != 'P' || data[1] != 'K' || data[2] != 'G' )
    {
        Printf("E:invalid header\n");
        return -1;
    }

    if ( version != PKG_VERSION )
    {
        Printf("E:unsupported version\n");
        return -1;
    }

    if ( PKG_Read( pkg, &packageSize, sizeof( packageSize ) )
         != sizeof( packageSize ) )
    {
        Printf("E:read package size\n");
        return -1;
    }

    return version;
}

LONG /* error */ PKG_ExtractFile( APTR pkg )
{
    ULONG  pathLength = 0, dataLength = 0;
    LONG   rc, result;
    STRPTR path        = NULL;
    APTR   buffer      = NULL;
    BPTR   destination = BNULL;
    BPTR   output      = BNULL;

    /* Read the path length */
    rc = PKG_Read( pkg, &pathLength, sizeof( pathLength ) );
    if( rc == 0 ) { result = 0; goto cleanup; }
    if( rc != sizeof( pathLength ) ) { result = -1; goto cleanup; }

    pathLength = AROS_BE2LONG(pathLength);

    /*
     * PKG_Read() takes a signed LONG length. Keep pathLength + 1
     * representable before allocating or reading the path field.
     */
    if( pathLength >= 0x7fffffffUL ) { result = -1; goto cleanup; }

    /* Read the path */
    path = AllocMem( pathLength + 1, MEMF_ANY );
    if( path == NULL ) { result = -1; goto cleanup; }

    rc = PKG_Read( pkg, path, pathLength + 1 );
    if( rc != (LONG)(pathLength + 1) ) { result = -1; goto cleanup; }
    if( path[pathLength] != '\0' ) { result = -1; goto cleanup; }

    /* Read the data length */
    rc = PKG_Read( pkg, &dataLength, sizeof( dataLength ) );
    if( rc != sizeof( dataLength ) ) { result = -1; goto cleanup; }

    dataLength = AROS_BE2LONG(dataLength);

    /*
     * C:Unpack has already changed CurrentDir() to the selected TO path.
     * Capture that directory as a lock before resolving the package member.
     */
    destination = Lock( "", SHARED_LOCK );
    if( destination == BNULL ) { Printf("E:destination\n"); result = -1; goto cleanup; }

    /* Read and write the data in pieces */
    buffer = AllocMem( PKG_BUFFER_SIZE, MEMF_ANY );
    if( buffer == NULL ) { Printf("E:mem\n"); result = -1; goto cleanup; }

    output = PKG_OpenContained( destination, path );
    if( output == BNULL ) { Printf("E:path\n"); result = -1; goto cleanup; }

    {
        ULONG total = 0;

        while( total < dataLength )
        {
            LONG length;
            ULONG remaining = dataLength - total;

            if( remaining >= PKG_BUFFER_SIZE )
            {
                length = PKG_BUFFER_SIZE;
            }
            else
            {
                length = remaining;
            }

            rc = PKG_Read( pkg, buffer, length );
            if( rc != length ) { Printf("E:read\n"); result = -1; goto cleanup; }

            rc = FILE_Write( output, buffer, length );
            if( rc != length ) { Printf("E:write\n"); result = -1; goto cleanup; }

            total += length;
        }
    }

    result = 1;

cleanup:
    if( path != NULL )         FreeMem( path, pathLength + 1 );
    if( buffer != NULL )       FreeMem( buffer, PKG_BUFFER_SIZE );
    if( output != BNULL )      Close( output );
    if( destination != BNULL ) UnLock( destination );

    return result;
}

LONG /* error */ PKG_ExtractEverything( APTR pkg )
{
    LONG result = PKG_ReadHeader( pkg );
    if ( result < 0 )
        return result;
    
    result = PKG_ExtractFile( pkg );
    while( result != -1 && result != 0 )
    {
        result = PKG_ExtractFile( pkg );
    }
    
    return result;
}
