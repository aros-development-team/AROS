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

/*
 * Assumes lock in already acquired. Returns pointer to AppIcon from workbench.library list, if the passed
 * pointer is actually on the list.
 */
STATIC struct AppIcon * FindAppIconByPtr(struct AppIcon * appicon)
{
    struct AppIcon * entry = NULL;
    ForeachNode(&LB(WorkbenchBase)->wb_AppIcons, entry)
    {
        if (entry == appicon)
            return entry;
    }

    return NULL;
}

BOOL SendAppIconMessage(struct AppIcon * appicon, LONG numargs, STRPTR args)
{
    struct AppMessage * msg = NULL;
    struct MsgPort *reply = NULL;
    BOOL success = FALSE;
    struct AppIcon * entry = NULL;

    if (!appicon)
        return success;

    LockWorkbench();

    /* Check is passed entry is valid */
    if ((entry = FindAppIconByPtr(appicon)) != NULL)
    {

        /* TODO: Handle the case when icons are dropped on appicon */
        msg = AllocVec(sizeof(struct AppMessage), MEMF_CLEAR);
        msg->am_Type = AMTYPE_APPICON;
        msg->am_ArgList = NULL;
        msg->am_NumArgs = 0;
        msg->am_ID = entry->ai_ID;
        msg->am_UserData = entry->ai_UserData;
        msg->am_Version = AM_VERSION;

        reply = CreateMsgPort();
        if (reply)
        {
            struct MsgPort *port = entry->ai_MsgPort;
            msg->am_Message.mn_ReplyPort = reply;
            PutMsg(port, (struct Message *) msg);
            WaitPort(reply);
            GetMsg(reply);
            DeleteMsgPort(reply);
            success = TRUE;
        }
    }

    UnlockWorkbench();

    FreeVec(msg);

    return success;
}

APTR AppObjectsLock()
{
    static LONG lock = 1;

    LockWorkbench();

    return (APTR)&lock;
}

VOID AppObjectsUnlock(APTR lock)
{
    UnlockWorkbench();
}

/* Assumes lock is already held */
struct AppIcon * GetNextAppIconLocked(struct AppIcon * lastappicon, APTR lock)
{
    struct List *appiconlist = &LB(WorkbenchBase)->wb_AppIcons;

    /* return NULL if list empty or argument dob is already from last appicon in list*/
    if (!IsListEmpty(appiconlist))
    {
        struct AppIcon * _return = NULL;

        /* return NULL if last entry reached */
        if (lastappicon == (struct AppIcon *)appiconlist->lh_TailPred)
            return NULL;

        _return = FindAppIconByPtr(lastappicon);

        if (_return)
        {
            /* Return next */
            return (struct AppIcon *)GetSucc((struct Node *)_return);
        }
        else
        {
            /* Return first */
            return (struct AppIcon *)appiconlist->lh_Head;
        }
    }

    return NULL;
}

struct DiskObject * AppIcon_GetDiskObject(struct AppIcon * appicon)
{
    struct AppIcon * entry = NULL;
    struct DiskObject * _return = NULL;

    LockWorkbench();

    entry = FindAppIconByPtr(appicon);

    if (entry)
        _return = entry->ai_DiskObject;

    UnlockWorkbench();

    return _return;
}

CONST_STRPTR AppIcon_GetLabel(struct AppIcon * appicon)
{
    struct AppIcon * entry = NULL;
    CONST_STRPTR _return = NULL;

    LockWorkbench();

    entry = FindAppIconByPtr(appicon);

    if (entry)
        _return = entry->ai_Text;

    UnlockWorkbench();

    return _return;
}

BOOL AppIcon_Supports(struct AppIcon * appicon, ULONG tag)
{
    BOOL _return = FALSE;
    /* This is a low-risk function, query passed object directly */
    switch(tag)
    {
    case(WBAPPICONA_SupportsOpen):          _return = !!(appicon->ai_Flags & WBAPPICONF_SupportsOpen); break;
    case(WBAPPICONA_SupportsCopy):          _return = !!(appicon->ai_Flags & WBAPPICONF_SupportsCopy); break;
    case(WBAPPICONA_SupportsRename):        _return = !!(appicon->ai_Flags & WBAPPICONF_SupportsRename); break;
    case(WBAPPICONA_SupportsInformation):   _return = !!(appicon->ai_Flags & WBAPPICONF_SupportsInformation); break;
    case(WBAPPICONA_SupportsSnapshot):      _return = !!(appicon->ai_Flags & WBAPPICONF_SupportsSnapshot); break;
    case(WBAPPICONA_SupportsUnSnapshot):    _return = !!(appicon->ai_Flags & WBAPPICONF_SupportsUnSnapshot); break;
    case(WBAPPICONA_SupportsLeaveOut):      _return = !!(appicon->ai_Flags & WBAPPICONF_SupportsLeaveOut); break;
    case(WBAPPICONA_SupportsPutAway):       _return = !!(appicon->ai_Flags & WBAPPICONF_SupportsPutAway); break;
    case(WBAPPICONA_SupportsDelete):        _return = !!(appicon->ai_Flags & WBAPPICONF_SupportsDelete); break;
    case(WBAPPICONA_SupportsFormatDisk):    _return = !!(appicon->ai_Flags & WBAPPICONF_SupportsFormatDisk); break;
    case(WBAPPICONA_SupportsEmptyTrash):    _return = !!(appicon->ai_Flags & WBAPPICONF_SupportsEmptyTrash); break;
    }
    return _return;
}
