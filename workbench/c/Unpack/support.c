/*
    Copyright © 2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <dos/dos.h>

#include <proto/dos.h>

#include "support.h"

BOOL MakeDir( CONST_STRPTR path )
{
    BPTR lock = CreateDir( path );
    
    if( lock != BNULL )
    {
        UnLock( lock );
        return TRUE;
    }
    
    return FALSE;
}

BOOL MakeDirs( STRPTR path )
{
    STRPTR position; 
    BOOL   success = FALSE;
    BPTR   lock    = BNULL;
    
    for( position = path; *position != '\0'; position++ )
    {
        if( *position == '/' )
        {
            *position = '\0';
            
            if( (lock = Lock( path, SHARED_LOCK )) != BNULL )
            {
                UnLock( lock );
                success = TRUE;
            }
            else
            {
                success = MakeDir( path );
            }
            
            *position = '/';
            
            if( !success ) return FALSE;
        }
    }
    
    return TRUE;
}

