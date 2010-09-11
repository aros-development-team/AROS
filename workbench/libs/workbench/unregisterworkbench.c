/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "workbench_intern.h"
#include <workbench/workbench.h>

/*****************************************************************************

    NAME */

        #include <proto/workbench.h>

        AROS_LH1(BOOL, UnregisterWorkbench,

/*  SYNOPSIS */
        AROS_LHA(struct MsgPort *, messageport, A0),

/*  LOCATION */

        struct WorkbenchBase *, WorkbenchBase, 24, Workbench)

/*  FUNCTION
        The workbench application uses this functions to unregister itself
        with the library. When it is done, messages will no longer be sent.

    INPUTS
        msgport - The message port of that was earlier passed in to 
                  RegisterWorkbench().

    RESULT
        TRUE if the message port was successfully unregistered, FALSE otherwise.
        The unregistration will fail if the message port isn't the same that 
        was passed in with RegisterWorkbench() earlier or if the passed
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
    
    BOOL success = FALSE;
    
    if (messageport != NULL)
    {
        ObtainSemaphore(&(WorkbenchBase->wb_WorkbenchPortSemaphore));
        
        if (WorkbenchBase->wb_WorkbenchPort == messageport)
        {
            WorkbenchBase->wb_WorkbenchPort = NULL;
            success = TRUE;
        }
        
        ReleaseSemaphore(&(WorkbenchBase->wb_WorkbenchPortSemaphore));
    }

    return success;

    AROS_LIBFUNC_EXIT
} /* UnregisterWorkbench() */
