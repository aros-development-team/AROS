/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
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
    Attempt to close the Workbench screen:
    - Check for open application windows. return FALSE if there are any
    - Clean up all special buffers
    - Close the Workbench screen
    - Make the Workbench program mostly inactive
      (disk activity will still be monitored)
    - Return TRUE
 
    INPUTS
 
    RESULT
 
    NOTES
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
    OpenWorkBench()
 
    INTERNALS
 
    HISTORY
 
*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    struct Screen *wbscreen;
    BOOL           retval = TRUE;

    DEBUG_CLOSEWORKBENCH(dprintf("CloseWorkBench: <%s>\n",
                                 FindTask(NULL)->tc_Node.ln_Name));

#ifdef INTUITION_NOTIFY_SUPPORT
    /* Notify that the Workbench screen is going to close */
    sn_DoNotify(SCREENNOTIFY_TYPE_WORKBENCH, (APTR) FALSE, GetPrivIBase(IntuitionBase)->ScreenNotifyBase);
#endif

    DEBUG_CLOSEWORKBENCH(dprintf("CloseWorkBench: LockPubScreenList\n"));
    LockPubScreenList();
    DEBUG_CLOSEWORKBENCH(dprintf("CloseWorkBench: LockPubScreenList done\n"));

    wbscreen = GetPrivIBase(IntuitionBase)->WorkBench;

    DEBUG_CLOSEWORKBENCH(dprintf("CloseWorkBench: Workbench Screen 0x%lx\n",
                                 (ULONG) wbscreen));

    if (!wbscreen)
    {
        DEBUG_CLOSEWORKBENCH(dprintf("CloseWorkBench: no wbscreen, do nothing\n"));
        UnlockPubScreenList();

#ifdef INTUITION_NOTIFY_SUPPORT
        /* Workbench screen failed to close, notify that the screen is open again */
        /* NOTE: Original screennotify.library notify in this case, too! */
        sn_DoNotify(SCREENNOTIFY_TYPE_WORKBENCH, (APTR) TRUE, GetPrivIBase(IntuitionBase)->ScreenNotifyBase);
#endif
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


    /* If there is a Workbench process running, tell it to close it's windows. */

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
        if( CloseScreen( wbscreen ) == TRUE )
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
    if (wbscreen && (retval == FALSE))
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

#ifdef INTUITION_NOTIFY_SUPPORT
    if (!retval)
    {
        /* Workbench screen failed to close, notify that the screen is open again */
        sn_DoNotify(SCREENNOTIFY_TYPE_WORKBENCH, (APTR) TRUE, GetPrivIBase(IntuitionBase)->ScreenNotifyBase);
    }
#endif

    return retval;

    AROS_LIBFUNC_EXIT

} /* CloseWorkBench */
