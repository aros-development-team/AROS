/*
    Copyright  1995-2011, The AROS Development Team. All rights reserved.
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

    AROS_LH1(APTR, StartScreenNotifyTagList,

/*  SYNOPSIS */
         AROS_LHA(struct TagItem *, tags, A0),

/*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 154, Intuition)

/*  FUNCTION
	Add Notifications to Intuition. You will be notified when
	the screen changes.
 
    INPUTS
	tags - see below

    TAGS 
	SNA_PubName (STRPTR)          - Name of the public screen. NULL means
                                        you'll get notifications for all screens.
	SNA_MsgPort (struct MsgPort*) - Notifications will be sent to this port.
	SNA_SigBit (BYTE)             - The signal bit to use
	SNA_SigTask (struct Task*)    - The task to signal
	SNA_UserData (IPTR)           - For your personal use. Will be copied
                                        into snm_UserData of the messages you receive
	SNA_Hook (struct Hook*)
	SNA_Priority (Byte)           - Priority in the notification queue.
	SNA_Notify (ULONG)            - SNOTIFY_ flags, see intuition/intuition.h

    RESULT
	The value is private, only a test against ZERO is allowed and means Failure
 
    NOTES
	This function is compatible with AmigaOS v4.
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
	EndScreenNotify() 

    INTERNALS
 
    HISTORY
 
*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    struct Library *UtilityBase = GetPrivIBase(IntuitionBase)->UtilityBase;
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
