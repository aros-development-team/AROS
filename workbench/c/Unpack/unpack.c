/*
    Copyright � 2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <dos/dos.h>
#include <dos/rdargs.h>
#include <proto/exec.h>
#include <proto/dos.h>

#define SH_GLOBAL_DOSBASE 1
#define SH_GLOBAL_SYSBASE 1

#include <aros/shcommands.h>

#include "modes.h"
#include "package.h"
#include "gui.h"

struct IntuitionBase *IntuitionBase;
struct GfxBase *GfxBase;

AROS_SH2
(
    Unpack, 1.0,
    AROS_SHA( STRPTR, , FILE, /A, NULL ),
    AROS_SHA( STRPTR, , TO,   /A, NULL )
)
{
    AROS_SHCOMMAND_INIT
    
    BPTR oldDir           = NULL, 
         newDir           = NULL;
    APTR pkg              = NULL;
    
    if( SHArg(FILE) == NULL ) goto cleanup;
    if( SHArg(TO) == NULL ) goto cleanup;

    IntuitionBase = OpenLibrary( "intuition.library", 0 );
    GfxBase = OpenLibrary( "graphics.library", 0 );
    
    //Printf( "%s, %s\n", SHArg(FILE), SHArg(TO) );
    
    pkg = PKG_Open( SHArg(FILE), MODE_READ );
    if( pkg == NULL ) goto cleanup;
    
    newDir = Lock( SHArg(TO), SHARED_LOCK );
    if( newDir == NULL ) goto cleanup;
    oldDir = CurrentDir( newDir );
    
    if( !GUI_Open() ) goto cleanup;
    
    PKG_ExtractEverything( pkg );
    
cleanup:
    GUI_Close();
    
    if( oldDir != NULL ) CurrentDir( oldDir );
    if( newDir != NULL ) UnLock( newDir );
    if( pkg != NULL ) PKG_Close( pkg );
    
    if( IntuitionBase != NULL ) CloseLibrary( (struct Library *) IntuitionBase );
    if( GfxBase != NULL ) CloseLibrary( (struct Library *) GfxBase );
    
    return 0;

    AROS_SHCOMMAND_EXIT
}

