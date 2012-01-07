/*
    Copyright © 2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <workbench/workbench.h>
#include "notifyworkbench.h"
#include "support_messages.h"
#include "handler.h"
#include "handler_support.h"

#include <aros/debug.h>

BOOL NotifyWorkbench(enum WBNOTIFY_Action action, enum WBNOTIFY_Target target,
        struct WorkbenchBase * WorkbenchBase)
{
    BOOL success = FALSE;
    struct WBCommandMessage *wbcm     = NULL;
    struct WBHandlerMessage *wbhm     = NULL;

    if ((target == WBNOTIFY_AppIcon) && ((action == WBNOTIFY_Create) || (action == WBNOTIFY_Delete)))
    {
        if ((wbcm = CreateWBCM(WBCM_TYPE_RELAY)) != NULL
                && (wbhm = CreateWBHM(WBHM_TYPE_UPDATE)) != NULL)
        {
            wbhm->wbhm_Data.Update.Name = NULL;
            wbhm->wbhm_Data.Update.Type = WBAPPICON;
            wbcm->wbcm_Data.Relay.Message = wbhm;

            PutMsg(&(WorkbenchBase->wb_HandlerPort), (struct Message *) wbcm);
            success = TRUE;
        }
        else
        {
            DestroyWBHM(wbhm);
            DestroyWBCM(wbcm);
        }
    }

    return success;
}
