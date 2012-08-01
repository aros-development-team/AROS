/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Add a window to Workbench's list of AppWindows.
    Lang: English
*/

#include <exec/types.h>
#include <exec/ports.h>
#include <utility/tagitem.h>
#include <intuition/intuition.h>
#include <proto/alib.h>

#include "workbench_intern.h"
#include <workbench/workbench.h>

/*****************************************************************************

    NAME */

        #include <proto/workbench.h>

        AROS_LH5(struct AppWindow *, AddAppWindowA,

/*  SYNOPSIS */
        AROS_LHA(IPTR            , id      , D0),
        AROS_LHA(IPTR            , userdata, D1),
        AROS_LHA(struct Window * , window  , A0),
        AROS_LHA(struct MsgPort *, msgport , A1),
        AROS_LHA(struct TagItem *, taglist , A2),

/*  LOCATION */
        struct WorkbenchBase *, WorkbenchBase, 8, Workbench)

/*  FUNCTION

    Try to add an AppWindow to workbench.library's list of AppWindows.
    The supplied message port will be used to send notification messages
    whenever an icon is dropped on the window. The message will be of
    type 'AMTYPE_APPWINDOW' and am_ArgList will point to the list of icons
    that were dropped in the window.

    INPUTS

    id        --  window identifier; for your convenience (ignored by
                  workbench.library)
    userdata  --  user specific data (ignored by workbench.library)
    window    --  pointer to the window to add AppWindow functionality to
    msgport   --  port to which notification messages regarding the window
                  will be sent
    taglist   --  tags (must be NULL)

    RESULT

    A pointer to an AppWindow structure to use with RemoveAppWindow() when
    you want to remove the window from the list of AppWindows, or NULL
    if it was not possible to add the window to the AppWindow list.

    NOTES

    Applications generally want to call GetDiskObjectNew() -- rather than
    GetDiskObject() -- to get disk objects for icons dropped in the window.
        Contrary to AmigaOS, this function will succeed even when there
    is no running workbench application.

    EXAMPLE

    BUGS

    SEE ALSO

    AddAppWindowDropZoneA(), RemoveAppWindow()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct AppWindow *appWindow;

    if (window == NULL || msgport == NULL)
    {
        return NULL;
    }

    appWindow = AllocVec(sizeof(struct AppWindow), MEMF_ANY | MEMF_CLEAR);

    if (appWindow == NULL)
    {
        return NULL;
    }

    appWindow->aw_ID       = id;
    appWindow->aw_UserData = userdata;
    appWindow->aw_Window   = window;
    appWindow->aw_MsgPort  = msgport;

    NewList(&appWindow->aw_DropZones);

    LockWorkbench();
    AddTail(&WorkbenchBase->wb_AppWindows, (struct Node *)appWindow);
    UnlockWorkbench();

    /* NotifyWorkbench(WBNOTIFY_Create, WBNOTIFY_AppWindow, WorkbenchBase); */
    
    return appWindow;

    AROS_LIBFUNC_EXIT
} /* AddAppWindowA */

