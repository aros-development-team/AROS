/*
    Copyright  1995-2006, The AROS Development Team. All rights reserved.
    Copyright  2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id: startscreennotifytaglist.c 20651 2006-12-30 20:57:12Z d.brewka $
 
    Add a Intuition Notification.
*/

#include <intuition/intuition.h>

#include "intuition_intern.h"

/*****************************************************************************
 
    NAME */

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>

AROS_LH1(IPTR, StartScreenNotifyTagList,

         /*  SYNOPSIS */
         AROS_LHA(struct TagItem *, tags, A0),

         /*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 161, Intuition)

/*  FUNCTION
    Add Notifications to Intuitionn.
 
    INPUTS
    taglist - A OS4 Styled Taglist for 
 
    RESULT
    the value is private only a test against ZERO is allowed and means Failure
 
    NOTES
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
    EndScreenNotify() 
    INTERNALS
 
    HISTORY
 
*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)
    struct IntScreenNotify *notify;

    notify = (struct IntScreenNotify *) AllocMem(sizeof(struct IntScreenNotify), MEMF_CLEAR);

    if (notify)
    {
	notify->port = (struct MsgPort *) GetTagData(SNA_MsgPort, 0, tags);
	notify->sigbit = (BYTE) GetTagData(SNA_SigBit, 0, tags);
	notify->sigtask = (struct Task *) GetTagData(SNA_SigTask, 0, tags);
 	notify->flags = (ULONG) GetTagData(SNA_Notify, 0, tags);
 	notify->userdata = (IPTR) GetTagData(SNA_UserData, 0, tags);
 	notify->hook = (struct Hook *) GetTagData(SNA_Hook, 0, tags);
 	notify->node.ln_Pri = (BYTE) GetTagData(SNA_Priority, 0, tags);
        notify->pubname = (struct MsgPort *) GetTagData(SNA_PubName, 0, tags);
    	ObtainSemaphore(&GetPrivIBase(IntuitionBase)->ScreenNotificationListLock);
        Enqueue(&GetPrivIBase(IntuitionBase)->ScreenNotificationList, (struct Node *) notify);
    	ReleaseSemaphore(&GetPrivIBase(IntuitionBase)->ScreenNotificationListLock);
    }

    ReturnPtr ("StartScreenNotifyTagList", APTR, notify);

    AROS_LIBFUNC_EXIT
} /* StartScreenNotifyTagList */
