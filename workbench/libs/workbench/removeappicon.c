/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Remove an icon from Workbench's list of AppIcons.
    Lang: English
*/

#include "workbench_intern.h"
#include "notifyworkbench.h"
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

    Try to remove an AppIcon from workbench.library's list of AppIcons.

    INPUTS

    appIcon  --  pointer to an AppIcon got from AddAppIconA()

    RESULT

    TRUE if the icon could be removed, FALSE otherwise.

    NOTES

    You must do a final check for messages on your AppMessage port as messages
    may have been sent between the last time you checked and the call to
    this function.

    EXAMPLE

    BUGS

    SEE ALSO

    AddAppIconA()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    if (appIcon == NULL)
    {
	return FALSE;
    }
    
    LockWorkbench();
    Remove((struct Node *)appIcon);
    UnlockWorkbench();

    FreeVec(appIcon);

    NotifyWorkbench(WBNOTIFY_Delete, WBNOTIFY_AppIcon, WorkbenchBase);

    return TRUE;

    AROS_LIBFUNC_EXIT
} /* RemoveAppIcon */
