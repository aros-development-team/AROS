/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Remove a window from Workbench's list of AppWindows.
    Lang: English
*/

#include <exec/lists.h>

#include "workbench_intern.h"
#include <workbench/workbench.h>

/*****************************************************************************

    NAME */

        #include <proto/workbench.h>

        AROS_LH1(BOOL, RemoveAppWindow,

/*  SYNOPSIS */

        AROS_LHA(struct AppWindow *, appWindow, A0),

/*  LOCATION */
        struct WorkbenchBase *, WorkbenchBase, 9, Workbench)

/*  FUNCTION

    Try to remove an AppWindow from workbench.library's list of AppWindow:s.

    INPUTS

    appWindow  --  pointer to AppWindow structure got from AddAppWindow().

    RESULT

    TRUE if the window could be removed, FALSE otherwise.

    NOTES

    You have to do another check for messages on the AppWindow message port
    you specified at the AppWindow creation time (AddAppWindow()) after the
    window is removed as it may have arrived messages between the last time
    you checked and the call of this function.
        Before the AppWindow is removed, all its drop zones will be removed.
    Thus there is no need to call RemoveAppWindowDropZone() explicitly.

    EXAMPLE

    BUGS

    SEE ALSO

    AddAppWindow(), RemoveAppWindowDropZone()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    if (appWindow == NULL)
    {
	return FALSE;
    }
    
    LockWorkbench();

    while (!IsListEmpty(&appWindow->aw_DropZones))
    {
	RemoveAppWindowDropZone(appWindow, 
	      (struct AppWindowDropZone *)GetHead(&appWindow->aw_DropZones));
    }

    Remove((struct Node *)appWindow);

    UnlockWorkbench();

    FreeVec(appWindow);

    /* NotifyWorkbench(WBNOTIFY_Create, WBNOTIFY_AppWindow, WorkbenchBase); */
    
    return TRUE;

    AROS_LIBFUNC_EXIT
} /* RemoveAppWindow */
