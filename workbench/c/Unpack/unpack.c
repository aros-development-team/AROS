/*
    Copyright © 2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <dos/dos.h>
#include <dos/rdargs.h>
#include <proto/exec.h>
#include <proto/dos.h>

#include "modes.h"
#include "package.h"
#include "gui.h"

#define TEMPLATE "FILE/A,TO/A"

STRPTR argFile = NULL;
STRPTR argTo   = NULL;

int main()
{
    IPTR args[]           = { NULL, (IPTR) "RAM:" };
    struct RDArgs *rdargs = ReadArgs( TEMPLATE, args, NULL );
    BPTR oldDir           = NULL, 
         newDir           = NULL;
    APTR pkg              = NULL;
    
    if( rdargs == NULL ) goto cleanup;
    argFile = args[0]; if( argFile == NULL ) goto cleanup;
    argTo   = args[1]; if( argTo   == NULL ) goto cleanup;
    
    //Printf( "%s, %s\n", argFile, argTo );
    
    pkg = PKG_Open( argFile, MODE_READ );
    if( pkg == NULL ) goto cleanup;
    
    newDir = Lock( argTo, SHARED_LOCK );
    if( newDir == NULL ) goto cleanup;
    oldDir = CurrentDir( newDir );
    
    if( !GUI_Open() ) goto cleanup;
    
    PKG_ExtractEverything( pkg );
    
cleanup:
    GUI_Close();
    
    if( rdargs != NULL ) FreeArgs( rdargs );
    if( oldDir != NULL ) CurrentDir( oldDir );
    if( newDir != NULL ) UnLock( newDir );
    if( pkg != NULL ) PKG_Close( pkg );
    
    return 0;
}

