/*
    (C) 1995-2000 AROS - The Amiga Research OS
    $Id$

    Desc: Tells intuition to open the Workbench.
    Lang: English
*/
       /*****************************************************************************

    NAME

        LoadWB

    FORMAT

        LoadWB

    TEMPLATE

    LOCATION

        Workbench:C/

    FUNCTION

        Tells intuition to open the Workbench.

    INPUTS

    RESULT

        Standard DOS return codes.

    EXAMPLE

    BUGS

        Does not support any arguments. Are they needed though?

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/

#include <exec/types.h>
#include <exec/memory.h>

#include <proto/exec.h>
#include <proto/intuition.h>

static const char version[] = "$VER: LoadWB 41.1 (30.11.2000)";

struct IntuitionBase *IntuitionBase;

int main( void ) {
    IntuitionBase = (struct IntuitionBase *) OpenLibrary( "intuition.library", 0L );

    if( IntuitionBase != NULL ) {
        OpenWorkBench();

        CloseLibrary( (struct Library *) IntuitionBase );
    }
}