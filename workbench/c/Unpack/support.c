#include <exec/types.h>
#include <dos/dos.h>

#include <proto/dos.h>

#include "support.h"

BOOL MakeDir( CONST_STRPTR path )
{
    BPTR lock = CreateDir( path );
    
    if( lock != NULL )
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
    BPTR   lock    = NULL;
    
    for( position = path; *position != '\0'; position++ )
    {
        if( *position == '/' )
        {
            *position = '\0';
            
            if( (lock = Lock( path, SHARED_LOCK )) != NULL )
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
}
