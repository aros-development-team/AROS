/*
    Copyright © 2003, 2009, The AROS Development Team. All rights reserved.
    $Id$
*/

/******************************************************************************

    NAME

        Unpack

    SYNOPSIS

        FILE/A, TO/A

    LOCATION

        C:

    FUNCTION

        Command to unpack/unarchive AROS .pkg files.

    INPUTS

        NAME    - The name of the file to unpack.
        TO – The drive or path to be unpacked.


    RESULT

        Standard DOS error codes.
	
    NOTES

        This command is not a tool like lha, lzx or unzip. 
        The .pkg files may be created with the python program
        in tools/package/pkg and compressed with bzip2 afterwards.

	
    EXAMPLE

        Unpack AROS.pkg TO Ram:
	
    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

    04.05.2000  SDuvan  implemented

******************************************************************************/

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
struct Library *BZ2Base;

AROS_SH2
(
    Unpack, 1.1,
    AROS_SHA( STRPTR, , FILE, /A, NULL ),
    AROS_SHA( STRPTR, , TO,   /A, NULL )
)
{
    AROS_SHCOMMAND_INIT
    
    BPTR oldDir           = BNULL, 
         newDir           = BNULL;
    APTR pkg              = NULL;
    
    if( SHArg(FILE) == NULL ) goto cleanup;
    if( SHArg(TO) == NULL ) goto cleanup;

    IntuitionBase = (struct IntuitionBase *)OpenLibrary( "intuition.library", 0 );
    GfxBase = (struct GfxBase *)OpenLibrary( "graphics.library", 0 );
    BZ2Base = OpenLibrary( "bz2.library", 0 );
    
    //Printf( "%s, %s\n", SHArg(FILE), SHArg(TO) );
    
    pkg = PKG_Open( SHArg(FILE), MODE_READ );
    if( pkg == NULL ) goto cleanup;
    
    newDir = Lock( SHArg(TO), SHARED_LOCK );
    if( newDir == BNULL ) goto cleanup;
    oldDir = CurrentDir( newDir );
    
    if( !GUI_Open() ) goto cleanup;
    
    PKG_ExtractEverything( pkg );
    
cleanup:
    GUI_Close();
    
    if( oldDir != BNULL ) CurrentDir( oldDir );
    if( newDir != BNULL ) UnLock( newDir );
    if( pkg != NULL ) PKG_Close( pkg );
    
    if( BZ2Base != NULL ) CloseLibrary( BZ2Base );
    if( IntuitionBase != NULL ) CloseLibrary( (struct Library *) IntuitionBase );
    if( GfxBase != NULL ) CloseLibrary( (struct Library *) GfxBase );
    
    return 0;

    AROS_SHCOMMAND_EXIT
}

