/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include "intuition_intern.h"

/*****************************************************************************
 
    NAME */
#include <proto/intuition.h>

AROS_LH2(void, UnlockPubScreen,

         /*  SYNOPSIS */
         AROS_LHA(UBYTE         *, name, A0),
         AROS_LHA(struct Screen *, screen, A1),

         /*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 86, Intuition)

/*  FUNCTION
    Release a lock to a screen locked by LockPubScreen().
    Identify screen by the pointer returned from LockPubScreen()
    and pass NULL name in normal cases.
    Sometimes it might be useful to specify the name string. In
    this case the screen pointer will be ignored.
 
    INPUTS
    name - Name of the public screen to unlock
    screen - Pointer to the screen to unlock
 
    RESULT
 
    NOTES
    The screen parameter will be ignored if name is non-NULL
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
    LockPubScreen()
 
    INTERNALS
 
    HISTORY
    29-10-95    digulla automatically created from
                intuition_lib.fd and clib/intuition_protos.h
 
*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    struct List *publist;

    DEBUG_UNLOCKPUBSCREEN(dprintf("UnlockPubScreen: name <%s> screen %p\n",
                                  name ? name : "NULL", screen));
    DEBUG_UNLOCKPUBSCREEN(dprintf("LockPubScreen: task %p <%s>\n",
                                  SysBase->ThisTask,
                                  SysBase->ThisTask->tc_Node.ln_Name ? SysBase->ThisTask->tc_Node.ln_Name : "NULL"));

    publist = LockPubScreenList();

    if (name != NULL)
    {
        struct PubScreenNode *psn;

        /* Get screen by its name */
        if ((psn = (struct PubScreenNode *)FindName(publist, name)))
            screen = psn->psn_Screen;
        else
            screen = NULL;

        DEBUG_UNLOCKPUBSCREEN(dprintf("UnlockPubScreen: found screen %p\n",
                                      screen));
    }

    if (screen != NULL)
    {
        struct PubScreenNode *ps = GetPrivScreen(screen)->pubScrNode;

        //ASSERT(GetPrivScreen(screen)->pubScrNode != NULL);
        //ASSERT(GetPrivScreen(screen)->pubScrNode->psn_VisitorCount > 0);

        /* Unlock screen */
        ps->psn_VisitorCount--;

        DEBUG_VISITOR(dprintf("UnlockPubScreen: count %d <%s>\n",
                              ps->psn_VisitorCount,
                              FindTask(NULL)->tc_Node.ln_Name));


        DEBUG_UNLOCKPUBSCREEN(dprintf("UnlockPubScreen: count %d\n",
                                      ps->psn_VisitorCount));

        /* Notify screen owner if this is (was) the last window on the
           screen */
        if(ps->psn_VisitorCount == 0)
        {
            if(ps->psn_SigTask != NULL)
                Signal(ps->psn_SigTask, 1 << ps->psn_SigBit);
        }

    }

    UnlockPubScreenList();
    AROS_LIBFUNC_EXIT
} /* UnlockPubScreen */
