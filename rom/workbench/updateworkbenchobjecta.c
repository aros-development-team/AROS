/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Inform the file manager that an object has changed.
*/

#define DEBUG 1
#include <aros/debug.h>

#include <exec/types.h>
#include <exec/ports.h>
#include <utility/tagitem.h>
#include <intuition/intuition.h>
#include <workbench/workbench.h>
#include <proto/utility.h>

#include <string.h>

#include "workbench_intern.h"
#include "support.h"
#include "support_messages.h"
#include "handler.h"
#include "handler_support.h"

/*****************************************************************************

    NAME */

        #include <proto/workbench.h>

        AROS_LH3(BOOL, UpdateWorkbenchObjectA,

/*  SYNOPSIS */
        AROS_LHA(STRPTR,           name, A0),
        AROS_LHA(LONG,             type, D1),
        AROS_LHA(struct TagItem *, tags, A1),

/*  LOCATION */
        struct WorkbenchBase *, WorkbenchBase, 25, Workbench)

/*  FUNCTION
        Informs the workbench application that an object has changed, and that
        it should update it's visual representation. 

    INPUTS
        name - Name of object that has changed.
        type - Type of object (WBDISK, WBTOOL, ...).
        tags - Additional options.
        
    TAGS
        No tags are defined at this time.

    NOTES
        This function is TEMPORARY! It will hopefully go away before AROS 1.0,
        and it might change it's API several times before that!

    EXAMPLE

    BUGS
        The existance of this function is a bug itself. It should be removed
        once there is a adequate notification API in dos.library that works.

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT
  
    BOOL                     success = FALSE;
    struct WBCommandMessage *wbcm    = NULL;
    struct WBHandlerMessage *wbhm    = NULL;
    
    if
    (
           (wbcm = CreateWBCM(WBCM_TYPE_RELAY))  != NULL
        && (wbhm = CreateWBHM(WBHM_TYPE_UPDATE)) != NULL
    )
    {
        if ((wbhm->wbhm_Data.Update.Name = StrDup(name)) != NULL)
        {
            wbhm->wbhm_Data.Update.Type   = type;
            wbcm->wbcm_Data.Relay.Message = wbhm;
            
            PutMsg(&WorkbenchBase->wb_HandlerPort, (struct Message *) wbcm);            
            
            success = TRUE;
        }
    }
    
    if (!success)
    {
        if (wbcm != NULL) DestroyWBCM(wbcm);
        if (wbhm != NULL) DestroyWBHM(wbhm);
    }
    
    return success;

    AROS_LIBFUNC_EXIT
} /* UpdateWorkbenchObjectA() */
