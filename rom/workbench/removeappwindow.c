/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$

    Desc: Remove a window from Workbench's list of AppWindows.
    Lang: english
*/

#include "workbench_intern.h"
#include <workbench/workbench.h>

/*****************************************************************************

    NAME */

        #include <proto/workbench.h>

        AROS_LH1(BOOL	, RemoveAppWindow,

/*  SYNOPSIS */

        AROS_LHA(struct AppWindow *, appWindow, A0),

/*  LOCATION */
        struct WorkbenchBase *, WorkbenchBase, 9, Workbench)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct WorkbenchBase *, WorkbenchBase)

    if( appWindow ) {
        struct Node *current;

        while( (current = RemHead( &(appWindow->aw_DropZones) )) ) {
            FreeVec( current );
        }

        Remove( (struct Node *) appWindow );
        FreeVec( appWindow );

        /* TODO: Notify the Workbench Apps about this. */

        return TRUE;
    }

    return FALSE;

    AROS_LIBFUNC_EXIT

} /* RemoveAppWindow */
