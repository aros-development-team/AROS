/*
    Copyright  1995-2007, The AROS Development Team. All rights reserved.
    Copyright  2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
 
    Remove a Intuition Notification.
*/

#include <intuition/intuition.h>

#include "intuition_intern.h"

/*****************************************************************************
 
    NAME */

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>

    AROS_LH1(BOOL, EndScreenNotify,

/*  SYNOPSIS */
         AROS_LHA(IPTR, notify, A0),

/*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 162, Intuition)

/*  FUNCTION
    Remove a Screen Notifications from Intuition.
 
    INPUTS
    notify - notification returned from StartScreenNotifyTagList() 
 
    RESULT
    BOOL - if false Notification is in use and cannot be removed, try later
 
    NOTES
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
    StartScreenNotifyTagList() 
    
    INTERNALS
 
    HISTORY
 
*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    BOOL back;

    if (notify == 0) return TRUE;

    if ((back = AttemptSemaphore(&GetPrivIBase(IntuitionBase)->ScreenNotificationListLock)))
    {
        if (((struct IntScreenNotify*) notify)->pubname) FreeVec(((struct IntScreenNotify*) notify)->pubname);
        Remove((struct Node *) notify);
        FreeVec((void *)notify);
    	ReleaseSemaphore(&GetPrivIBase(IntuitionBase)->ScreenNotificationListLock);
    }

    ReturnPtr ("EndScreenNotify", BOOL, back);

    AROS_LIBFUNC_EXIT
} /* EndScreenNotify */
