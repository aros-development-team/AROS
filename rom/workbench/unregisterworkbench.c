/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: english
*/

#include "workbench_intern.h"
#include <workbench/workbench.h>

/*****************************************************************************

    NAME */

        #include <proto/workbench.h>

        AROS_LH1(BOOL   , UnregisterWorkbench,
/*  SYNOPSIS */
        AROS_LHA(struct MsgPort *, msgport, A0),
/*  LOCATION */

        struct WorkbenchBase *, WorkbenchBase, 24, Workbench)
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

    if( (msgport != NULL) && (WorkbenchBase->wb_AppPort == msgport) ) {
        AlohaWorkbench( NULL );
        WorkbenchBase->wb_AppPort = NULL;

        return TRUE;
    }

    return FALSE;

    AROS_LIBFUNC_EXIT
} /* UnregisterWorkbench */