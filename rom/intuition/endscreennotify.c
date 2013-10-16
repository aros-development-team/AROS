/*
    Copyright  1995-2013, The AROS Development Team. All rights reserved.
    Copyright  2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$

    Remove an Intuition Notification.
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
         AROS_LHA(APTR, notify, A0),

/*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 155, Intuition)

/*  FUNCTION
	Remove a Screen Notification from Intuition.

    INPUTS
	notify - notification returned from StartScreenNotifyTagList()

    RESULT
	BOOL - if FALSE, Notification is in use and cannot be removed; try later

    NOTES
	This function is compatible with AmigaOS v4.

    EXAMPLE

    BUGS

    SEE ALSO
	StartScreenNotifyTagList()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    BOOL back;

    if (notify == NULL) return TRUE;

    if ((back = AttemptSemaphore(&GetPrivIBase(IntuitionBase)->ScreenNotificationListLock)))
    {
        FreeVec(((struct IntScreenNotify*) notify)->pubname);
        Remove((struct Node *) notify);
        FreeVec((void *)notify);
    	ReleaseSemaphore(&GetPrivIBase(IntuitionBase)->ScreenNotificationListLock);
    }

    ReturnPtr ("EndScreenNotify", BOOL, back);

    AROS_LIBFUNC_EXIT
} /* EndScreenNotify */
