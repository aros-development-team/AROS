/*
    (C) 1995-2001 AROS - The Amiga Research OS
    $Id$
 
    Desc: Intuition function OpenWorkBench()
    Lang: english
*/

#include "intuition_intern.h"

#include <intuition/intuition.h>
#include <proto/intuition.h>

/*****************************************************************************
 
    NAME */

AROS_LH0(IPTR, OpenWorkBench,

         /*  SYNOPSIS */

         /*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 35, Intuition)

/*  FUNCTION
    Attempt to open the Workbench screen.
 
    INPUTS
    None.
 
    RESULT
    Tries to (re)open WorkBench screen. If successful return value
    is a pointer to the screen structure, which shouldn't be used,
    because other programs may close the WorkBench and make the
    pointer invalid.
    If this function fails the return value is NULL.
 
    NOTES
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
    CloseWorkBench()
 
    INTERNALS
 
    HISTORY
 
*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    struct Screen *wbscreen;

    DEBUG_OPENWORKBENCH(dprintf("OpenWorkBench: <%s>\n",
                                FindTask(NULL)->tc_Node.ln_Name));

    LockPubScreenList();

    wbscreen = GetPrivIBase(IntuitionBase)->WorkBench;

    DEBUG_OPENWORKBENCH(dprintf("OpenWorkBench: Workbench 0x%lx\n",
                                (ULONG) wbscreen));

    if (wbscreen)
    {
        DEBUG_OPENWORKBENCH(dprintf("OpenWorkBench: returning Workbench screen at 0x%lx\n",
                                    (ULONG) wbscreen));

        UnlockPubScreenList();

#ifdef INTUITION_NOTIFY_SUPPORT
        /* Notify that the Workbench screen is open */
        /* NOTE: Original screennotify.library notify in this case, too! */
        sn_DoNotify(SCREENNOTIFY_TYPE_WORKBENCH, (APTR) TRUE, GetPrivIBase(IntuitionBase)->ScreenNotifyBase);
#endif
        return (IPTR)wbscreen;
    }
    else
    {
        /* Open the Workbench screen if we don't have one. */
        struct TagItem screenTags[] =
            {
                { SA_LikeWorkbench, TRUE                },
                { SA_Type         , WBENCHSCREEN        },
                { SA_Title        , (IPTR)"Workbench Screen"},
                { SA_PubName      , (IPTR)"Workbench"   },
                { SA_SharePens    , TRUE            },
                { TAG_END         , 0           }
            };

        DEBUG_OPENWORKBENCH(dprintf("OpenWorkBench: Trying to open Workbench screen\n"));

        wbscreen = OpenScreenTagList( NULL, screenTags );

        if( !wbscreen )
        {
            DEBUG_OPENWORKBENCH(dprintf("OpenWorkBench: failed to open Workbench screen !!!!\n"));

            UnlockPubScreenList();
            return 0;
        }

        GetPrivIBase(IntuitionBase)->WorkBench = wbscreen;

        /* Make the screen public. */
        PubScreenStatus( wbscreen, 0 );

    }

    /* We have opened the Workbench Screen. Now  tell the Workbench process
       to open it's windows, if there is one. We still do have the pub screen
       list locked. But while sending the Message to the Workbench task we
       must unlock the semaphore, otherwise there can be deadlocks if the
       Workbench task itself does something which locks the pub screen list.

       But if we unlock the pub screen list, then some other task could try
       to close the Workbench screen in the meantime. The trick to solve
       this problem is to increase the psn_VisitorCount of the Workbench
       screen here, before unlocking the pub screen list. This way the
       Workbench screen cannot go away. */

    GetPrivScreen(wbscreen)->pubScrNode->psn_VisitorCount++;
    DEBUG_VISITOR(dprintf("OpenWorkbench: new VisitorCount %ld\n",
                          GetPrivScreen(wbscreen)->pubScrNode->psn_VisitorCount));

    UnlockPubScreenList();

    DEBUG_VISITOR(dprintf("OpenWorkbench: notify Workbench\n"));

    /* Don't call this function while pub screen list is locked! */
    TellWBTaskToOpenWindows(IntuitionBase);

    /* Now fix the psn_VisitorCount we have increased by one, above. It's probably
       better to do this by hand, instead of calling UnlockPubScreen, because Un-
       lockPubScreen can send signal to psn_SigTask. */

    LockPubScreenList();
    GetPrivScreen(wbscreen)->pubScrNode->psn_VisitorCount--;
    DEBUG_VISITOR(dprintf("OpenWorkbench: new VisitorCount %ld\n",
                          GetPrivScreen(wbscreen)->pubScrNode->psn_VisitorCount));
    UnlockPubScreenList();

#ifdef INTUITION_NOTIFY_SUPPORT
    /* Notify that the Workbench screen is open again */
    sn_DoNotify(SCREENNOTIFY_TYPE_WORKBENCH, (APTR) TRUE, GetPrivIBase(IntuitionBase)->ScreenNotifyBase);
#endif

    return (IPTR)wbscreen;

    AROS_LIBFUNC_EXIT

} /* OpenWorkBench */
