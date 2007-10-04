/*
    Copyright  1995-2007, The AROS Development Team. All rights reserved.
    Copyright  2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
 
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
    struct IntScreenNotify *notify;

    notify = (struct IntScreenNotify *) AllocVec(sizeof(struct IntScreenNotify), MEMF_CLEAR);

    if (notify)
    {
	notify->port = (struct MsgPort *) GetTagData(SNA_MsgPort, 0, tags);
	notify->sigbit = (BYTE) GetTagData(SNA_SigBit, 0, tags);
	notify->sigtask = (struct Task *) GetTagData(SNA_SigTask, 0, tags);
 	notify->flags = (ULONG) GetTagData(SNA_Notify, 0, tags);
 	notify->userdata = (IPTR) GetTagData(SNA_UserData, 0, tags);
 	notify->hook = (struct Hook *) GetTagData(SNA_Hook, 0, tags);
 	notify->node.ln_Pri = (BYTE) GetTagData(SNA_Priority, 0, tags);
        notify->pubname = NULL;

        char *pubname = (char *) GetTagData(SNA_PubName, 0, tags);
        if (pubname)
        {
            notify->pubname = AllocVec(strlen(pubname) + 1, MEMF_CLEAR);
            if (notify->pubname)
            {
                strcpy(notify->pubname, pubname);
            }
            else
            {
                FreeVec(notify);
                notify = NULL;
            }
        }
        if (notify)
        {
    	    ObtainSemaphore(&GetPrivIBase(IntuitionBase)->ScreenNotificationListLock);
            Enqueue(&GetPrivIBase(IntuitionBase)->ScreenNotificationList, (struct Node *) notify);
    	    ReleaseSemaphore(&GetPrivIBase(IntuitionBase)->ScreenNotificationListLock);
        }
    }

    ReturnPtr ("StartScreenNotifyTagList", APTR, notify);

    AROS_LIBFUNC_EXIT
} /* StartScreenNotifyTagList */
