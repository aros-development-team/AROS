/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include "intuition_intern.h"

#include <intuition/intuition.h>
#include <proto/intuition.h>

/*****************************************************************************

    NAME */
        AROS_LH0(LONG, CloseWorkBench,

/*  SYNOPSIS */

/*  LOCATION */
        struct IntuitionBase *, IntuitionBase, 13, Intuition)

/*  FUNCTION
        Attempt to close the Workbench screen. This will fail if there are any
        non-Drawer windows open on it.

    INPUTS

    RESULT
        success - TRUE if Workbench screen could be closed.

    NOTES
        If the Workbench screen is already closed when this function is called,
        FALSE is returned.

    EXAMPLE

    BUGS

    SEE ALSO
        OpenWorkBench()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Screen *wbscreen;
    BOOL           retval = TRUE;

    DEBUG_CLOSEWORKBENCH(dprintf("CloseWorkBench: <%s>\n",
                                 FindTask(NULL)->tc_Node.ln_Name));

    wbscreen = GetPrivIBase(IntuitionBase)->WorkBench;
    DEBUG_CLOSEWORKBENCH(dprintf("CloseWorkBench: Workbench Screen 0x%p\n", wbscreen));

    if (wbscreen) FireScreenNotifyMessage((IPTR) wbscreen, SNOTIFY_BEFORE_CLOSEWB, IntuitionBase);

    DEBUG_CLOSEWORKBENCH(dprintf("CloseWorkBench: LockPubScreenList\n"));
    LockPubScreenList();
    DEBUG_CLOSEWORKBENCH(dprintf("CloseWorkBench: LockPubScreenList done\n"));

    if (!wbscreen)
    {
        DEBUG_CLOSEWORKBENCH(dprintf("CloseWorkBench: no wbscreen, do nothing\n"));
        UnlockPubScreenList();

        FireScreenNotifyMessage((IPTR) wbscreen, SNOTIFY_AFTER_OPENWB, IntuitionBase);

        return(FALSE);
    }
    else
    {
        DEBUG_CLOSEWORKBENCH(dprintf("CloseWorkBench: Workbench Screen's pubnode 0x%lx\n",
                                     (ULONG) GetPrivScreen(wbscreen)->pubScrNode));
        if (GetPrivScreen(wbscreen)->pubScrNode->psn_VisitorCount != 0)
        {
            DEBUG_CLOSEWORKBENCH(dprintf("CloseWorkBench: visitor count %ld, do nothing\n",
                                         (ULONG) GetPrivScreen(wbscreen)->pubScrNode->psn_VisitorCount));
            retval = FALSE;
        }
    }

    DEBUG_CLOSEWORKBENCH(dprintf("CloseWorkBench: UnlockPubScreenList\n"));
    UnlockPubScreenList();
    DEBUG_CLOSEWORKBENCH(dprintf("CloseWorkBench: UnlockPubScreenList done\n"));

    /* If there is a Workbench process running, tell it to close its windows. */

    /* Don't call this function while pub screen list is locked! */

    DEBUG_CLOSEWORKBENCH(dprintf("CloseWorkBench: Tell WB to close windows\n"));
    TellWBTaskToCloseWindows(IntuitionBase);

    DEBUG_CLOSEWORKBENCH(dprintf("CloseWorkBench: LockPubScreenList\n"));
    LockPubScreenList();
    DEBUG_CLOSEWORKBENCH(dprintf("CloseWorkBench: LockPubScreenList done\n"));

    wbscreen = GetPrivIBase(IntuitionBase)->WorkBench;

    DEBUG_CLOSEWORKBENCH(dprintf("CloseWorkBench: current wbscreen 0x%lx\n",
                                 (ULONG) wbscreen));

    /* Try to close the Workbench screen, if there is any. */
    if( wbscreen != NULL )
    {
        DEBUG_CLOSEWORKBENCH(dprintf("CloseWorkBench: CloseScreen\n"));
        if (CloseScreen(wbscreen))
        {
            DEBUG_CLOSEWORKBENCH(dprintf("CloseWorkBench: CloseScreen worked\n"));
            GetPrivIBase(IntuitionBase)->WorkBench = NULL;
            wbscreen = NULL;
            retval = TRUE;
        }
        else
        {
            /* Grrr ... closing screen failed. I inc psn_VisitorCount by hand here,
             * to avoid that someone else can kill it, because I must tell Workbench
             * task that it shall open its windows again
             */

            DEBUG_CLOSEWORKBENCH(dprintf("CloseWorkBench: CloseScreen failed\n"));
            GetPrivScreen(wbscreen)->pubScrNode->psn_VisitorCount++;
            retval = FALSE;
        }
    }
    else
    {
        retval = FALSE;
    }
    
    DEBUG_CLOSEWORKBENCH(dprintf("CloseWorkBench: UnLockPubScreenList\n"));
    UnlockPubScreenList();
    DEBUG_CLOSEWORKBENCH(dprintf("CloseWorkBench: UnLockPubScreenList done\n"));

    DEBUG_CLOSEWORKBENCH(dprintf("CloseWorkBench: wbscreen 0x%lx retval %ld\n",
                                 (ULONG) wbscreen,
                                 (ULONG) retval));
    if (wbscreen && !retval)
    {
        /* Don't call this function while pub screen list is locked! */
        DEBUG_CLOSEWORKBENCH(dprintf("CloseWorkBench: Tell WB to reopen windows\n"));
        TellWBTaskToOpenWindows(IntuitionBase);

        /* Fix psn_VisitorCount which was increased above */
        DEBUG_CLOSEWORKBENCH(dprintf("CloseWorkBench: LockPubScreenList\n"));
        LockPubScreenList();
        DEBUG_CLOSEWORKBENCH(dprintf("CloseWorkBench: LockPubScreenList done\n"));
        DEBUG_CLOSEWORKBENCH(dprintf("CloseWorkBench: WBScreen's pubnode 0x%lx\n",
                                     (ULONG) GetPrivScreen(wbscreen)->pubScrNode));
        GetPrivScreen(wbscreen)->pubScrNode->psn_VisitorCount--;
        DEBUG_CLOSEWORKBENCH(dprintf("CloseWorkBench: UnLockPubScreenList\n"));
        UnlockPubScreenList();
        DEBUG_CLOSEWORKBENCH(dprintf("CloseWorkBench: UnLockPubScreenList done\n"));
    }

    DEBUG_CLOSEWORKBENCH(dprintf("CloseWorkBench: Return %d\n", retval));

    if (!retval)
    {
        /* Workbench screen failed to close, notify that the screen is open again */
        FireScreenNotifyMessage((IPTR) wbscreen, SNOTIFY_AFTER_OPENWB, IntuitionBase);
    }
    else
    {
        FireScreenNotifyMessage((IPTR) wbscreen, SNOTIFY_AFTER_CLOSEWB, IntuitionBase);
    }

    return retval;

    AROS_LIBFUNC_EXIT

} /* CloseWorkBench */
