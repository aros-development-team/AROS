/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$

    Desc: Remove a menuitem from Workbench's list of AppMenuItems.
    Lang: english
*/

#include "workbench_intern.h"
#include <workbench/workbench.h>

/*****************************************************************************

    NAME */

        #include <proto/workbench.h>

        AROS_LH1(BOOL	, RemoveAppMenuItem,
/*  SYNOPSIS */

        AROS_LHA(struct AppMenuItem *, appMenuItem, A0),

/*  LOCATION */
        struct WorkbenchBase *, WorkbenchBase, 13, Workbench)

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

    if( appMenuItem ) {
        Remove( (struct Node *) appMenuItem );
        FreeVec( appMenuItem );

        /* TODO: Notify Workbench Apps about the change. */

        return TRUE;
    }

    return FALSE;

    AROS_LIBFUNC_EXIT
} /* RemoveAppMenuItem */

