/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$

    Desc: Remove an icon from Workbench's list of AppIcons.
    Lang: english
*/

#include "workbench_intern.h"
#include <workbench/workbench.h>

/*****************************************************************************

    NAME */

        #include <proto/workbench.h>

        AROS_LH1(BOOL   , RemoveAppIcon,
/*  SYNOPSIS */
        AROS_LHA(struct AppIcon *, appIcon, A0),
/*  LOCATION */

        struct WorkbenchBase *, WorkbenchBase, 11, Workbench)
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

    if( appIcon ) {
        Remove( (struct Node *) appIcon );
        FreeVec( appIcon );

        /* TODO: Notify the Workbench Apps about this. */
        return TRUE;
    }

    return FALSE;

    AROS_LIBFUNC_EXIT
} /* RemoveAppIcon */
