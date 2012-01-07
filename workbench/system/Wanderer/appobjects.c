/*
    Copyright © 2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "appobjects.h"

#include <proto/workbench.h>
#include <proto/exec.h>

/*
 * Wanderer needs access to internals of workbench.library to provided needed functionality.
 * In general, "workbench replacements" needs to provide their own workbench.library or patch the
 * existing one. All Wanderer code should treat the internals of workbench.library as READ-ONLY.
 * Only this compilation unit may access the workbench.library internals.
 */

#include "../../libs/workbench/workbench_intern.h"

BOOL SendAppIconMessage(struct AppIcon * appicon, LONG numargs, STRPTR args)
{
    struct AppMessage * msg = NULL;
    struct MsgPort *reply = NULL;
    BOOL success = FALSE;

    if (!appicon)
        return success;

    LockWorkbench();

    /* TODO: Check if appicon is still on the list of appicons handled by workbench.library */

    /* TODO: Handle the case when icons are dropped on appicon */
    msg = AllocVec(sizeof(struct AppMessage), MEMF_CLEAR);
    msg->am_Type = AMTYPE_APPICON;
    msg->am_ArgList = NULL;
    msg->am_NumArgs = 0;
    msg->am_ID = appicon->ai_ID;
    msg->am_UserData = appicon->ai_UserData;
    msg->am_Version = AM_VERSION;

    reply = CreateMsgPort();
    if (reply)
    {
        struct MsgPort *port = appicon->ai_MsgPort;
        msg->am_Message.mn_ReplyPort = reply;
        PutMsg(port, (struct Message *) msg);
        WaitPort(reply);
        GetMsg(reply);
        DeleteMsgPort(reply);
        success = TRUE;
    }

    UnlockWorkbench();

    FreeVec(msg);

    return success;
}
