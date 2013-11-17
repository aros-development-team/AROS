/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include "intuition_intern.h"

/*****************************************************************************

    NAME */
#include <proto/intuition.h>

        AROS_LH0(struct List *, LockPubScreenList,

/*  SYNOPSIS */

/*  LOCATION */
        struct IntuitionBase *, IntuitionBase, 87, Intuition)

/*  FUNCTION
        Arbitrates access to the system public screen list. This is for Public
        Screen Manager programs only! The list should be locked for as short a
        time as possible.

    INPUTS

    RESULT

    NOTES
        The list's nodes are PubScreenNodes as defined in
        <intuition/screens.h>.

    EXAMPLE

    BUGS

    SEE ALSO
        UnlockPubScreenList()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    DEBUG_LOCKPUBSCREENLIST(dprintf("LockPubScreenList: <%s>\n",
                                    FindTask(NULL)->tc_Node.ln_Name));
    ObtainSemaphore(&GetPrivIBase(IntuitionBase)->PubScrListLock);
    DEBUG_LOCKPUBSCREENLIST(dprintf("LockPubScreenList: done\n"));
    
    return (struct List *)&(GetPrivIBase(IntuitionBase)->PubScreenList);

    AROS_LIBFUNC_EXIT
} /* LockPubScreenList */
