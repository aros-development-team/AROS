/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Remove a dropzone from a AppWindow's list of AppWindowDropZones.
    Lang: English
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

    Try to remove a drop zone from an AppWindow.

    INPUTS

    appWindow  --  pointer to the AppWindow (as returned by AddAppWindow()) to
                   try to remove the drop zone from; a value of NULL will
		   result in no operation
    dropZone   --  pointer to an AppWindowDropZone (as returned by
                   AddAppWindowDropZone()); a value of NULL will result in
		   no operation

    RESULT

    TRUE if the drop zone could be removed, FALSE otherwise. In case of
    failure, the reason may be obtained from dos.library/IoErr(). This
    function may fail if the specified drop zone is not registered with
    the supplied AppWindow.
   
    NOTES
    
    You must check for drop zone messages for zones that you just removed as
    there might have been messages sent between the last time you checked and
    the call to this function.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct WorkbenchBase *, WorkbenchBase)

    struct AppWindowDropZone *dz;

    BOOL found = FALSE;		/* Is the drop zone 'dropZone' added to the
				   window 'aw'? */

    if ((aw == NULL) || (dropZone == NULL))
    {
	SetIoErr(ERROR_REQUIRED_ARG_MISSING);

	return FALSE;
    }

    LockWorkbench();
    ForeachNode(&aw->aw_DropZones, dz)
    {
	if (dz == dropZone)
	{
	    found = TRUE;
	    break;
	}
    }

    if (!found)
    {
	UnlockWorkbench();
	SetIoErr(ERROR_OBJECT_NOT_FOUND);

	return FALSE;
    }

    Remove((struct Node *)dropZone);
    UnlockWorkbench();

    FreeVec(dropZone);

    /* NotifyWorkbench(WBNOTIFY_Delete, WBNOTIFY_DropZone, WorkbenchBase); */
	
    return TRUE;

    AROS_LIBFUNC_EXIT
} /* RemoveAppWindowDropZone */

