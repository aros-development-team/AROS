/*
    (C) 1995-2001 AROS - The Amiga Research OS
    $Id$

    Desc: Remove an icon from Workbench's list of AppIcons.
    Lang: English
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

    AddAppIcon()

    INTERNALS

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct WorkbenchBase *, WorkbenchBase)

    if (appIcon == NULL)
    {
	return FALSE;
    }
    
    LockWorkbench();
    Remove((struct Node *)appIcon);
    UnlockWorkbench();

    FreeVec(appIcon);

    /*
      Question is if we should negotiate with the (possible) workbench
      application. Probably not... we just remove it from the list and
      send a message to the workbench application that the icon should
      be removed as soon as possible. 

      NotifyWorkbench(WBNOTIFY_Delete, WBNOTIFY_AppIcon, WorkbenchBase); */

    return TRUE;

    AROS_LIBFUNC_EXIT
} /* RemoveAppIcon */
