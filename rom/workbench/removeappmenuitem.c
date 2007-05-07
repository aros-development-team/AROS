/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Remove a menuitem from Workbench's list of AppMenuItems.
    Lang: English
*/

#include "workbench_intern.h"
#include <workbench/workbench.h>

/*****************************************************************************

    NAME */

        #include <proto/workbench.h>

        AROS_LH1(BOOL, RemoveAppMenuItem,
/*  SYNOPSIS */

        AROS_LHA(struct AppMenuItem *, appMenuItem, A0),

/*  LOCATION */
        struct WorkbenchBase *, WorkbenchBase, 13, Workbench)

/*  FUNCTION

    Try to remove an AppMenuItem from workbench.library's list of AppMenuItems.

    INPUTS

    Pointer to an AppMenuItem structure as returned by AddAppMenuItem().

    RESULT

    TRUE if the menu item could be removed, FALSE otherwise.

    NOTES

    You have to do a final check for messages on your AppMenuItem message
    port as messages may have arrived between the last time you checked this
    and the call to this function.

    EXAMPLE

    BUGS

    SEE ALSO

    AddAppMenuItem()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    if (appMenuItem == NULL)
    {
	return FALSE;
    }

    LockWorkbench();
    Remove((struct Node *)appMenuItem);
    UnlockWorkbench();

    FreeVec(appMenuItem);

    /* NotifyWorkbench(WBNOTIFY_Delete, WBNOTIFY_AppMenuItem, WorkbenchBase);
     */

    return TRUE;

    AROS_LIBFUNC_EXIT
} /* RemoveAppMenuItem */


