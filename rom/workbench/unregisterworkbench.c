/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Unregister a program as the Workbench Application.
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
        The Workbench Application uses this functions to unregister itself
        with the library and intuition. When it is done, notification
        messages will no longer be sent.

    INPUTS
        msgport - The MsgPort of the Workbench Application that earlier was
                  passed in with RegisterWorkbench().

    RESULT
        TRUE if the message port was successfully unregistered, FALSE otherwise.
        The unregistration will fail if the passed in MsgPort isn't the same
        that was passed in with RegisterWorkbench() earlier or if the passed
        in pointer is NULL.

    NOTES
        Note that "Workbench Application" in this context means the program that
        is the file manager and handles the GUI of Workbench, not a program that
        is started from Workbench!

    EXAMPLE

    BUGS

    SEE ALSO
        RegisterWorkbench()

    INTERNALS

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
