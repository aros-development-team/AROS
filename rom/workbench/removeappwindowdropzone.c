/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: english
*/

#include <exec/types.h>
#include <exec/ports.h>
#include <utility/tagitem.h>
#include <intuition/intuition.h>

#include "workbench_intern.h"
#include <workbench/workbench.h>

/*****************************************************************************

    NAME */

        #include <proto/workbench.h>

        AROS_LH2(BOOL, RemoveAppWindowDropZone,

/*  SYNOPSIS */
        AROS_LHA(struct AppWindow *        , aw      , A0),
        AROS_LHA(struct AppWindowDropZone *, dropZone, A1),

/*  LOCATION */
        struct WorkbenchBase *, WorkbenchBase, 20, Workbench)

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

    if( (aw != NULL) && (dropZone != NULL) ) {
        Remove( (struct Node *) dropZone );
        FreeVec( dropZone );

        /* TODO: Notify the Workbench Apps about this. */

        return TRUE;
    }

    return FALSE;

    AROS_LIBFUNC_EXIT
} /* RemoveAppWindowDropZone */

