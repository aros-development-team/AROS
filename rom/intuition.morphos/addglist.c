/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include <proto/layers.h>
#include "intuition_intern.h"

/*****************************************************************************
 
    NAME */
#include <intuition/intuition.h>
#include <proto/intuition.h>
#include "boopsigadgets.h"

#undef DEBUG
#define DEBUG 0
#   include <aros/debug.h>

AROS_LH5(UWORD, AddGList,

         /*  SYNOPSIS */
         AROS_LHA(struct Window    *, window, A0),
         AROS_LHA(struct Gadget    *, gadget, A1),
         AROS_LHA(ULONG             , position, D0),
         AROS_LHA(LONG              , numGad, D1),
         AROS_LHA(struct Requester *, requester, A2),

         /*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 73, Intuition)

/*  FUNCTION
    Add some gadgets to a window.
 
    INPUTS
    window - Add gadgets to this window
    gadget - This is the list of gadgets to add
    position - Where to insert the gadgets in the list of gadgets
        already in the window. Use 0 to insert the gadgets
        before all others in the window or ~0 to append them.
    numGad - How many gadgets of the list should be added.
        Use -1 to add all gadgets in the list.
    requester - Pointer to the requester structure if the window is
        a requester.
 
    RESULT
    The actual position where the gadgets were inserted.
 
    NOTES
    The gadgets will just be added. To make them visible, you must
    refresh the window or the gadgets.
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
    RefreshGadgets(), RefresgGList()
 
    INTERNALS
 
    HISTORY
 
*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    struct Gadget *pred;
    struct Gadget *last;
    UWORD   	   count, count2;

    EXTENDUWORD(position);
    EXTENDWORD(numGad);

    DEBUG_ADDGLIST(dprintf("AddGList: Window 0x%lx Gadget 0x%lx Pos %ld Num %ld Req 0x%lx\n",
                           window, gadget, position, numGad, requester));

    if (!gadget || !numGad || !window)
        return (UWORD)-1;

    D(bug("AddGList()\n"));

    if (gadget->GadgetType & GTYP_REQGADGET)
    {
        for (last=gadget, count2 = numGad; last && count2--; last=last->NextGadget)
        {
            DEBUG_ADDGLIST(dprintf("AddGList: Gadget 0x%lx Flags 0x%lx Type 0x%lx Activation 0x%lx %d,%d %d×%d ID 0x%x\n",
                                   last, last->Flags, last->GadgetType, last->Activation,
                                   last->LeftEdge, last->TopEdge, last->Width, last->Height,
                                   last->GadgetID));
            last->GadgetType |= GTYP_REQGADGET;
        }
        pred = (struct Gadget *)&requester->ReqGadget;
    }
    else
    {
        for (last=gadget, count2 = numGad; last && count2--; last=last->NextGadget)
        {
            DEBUG_ADDGLIST(dprintf("AddGList: Gadget 0x%lx Flags 0x%lx Type 0x%lx Activation 0x%lx %d,%d %d×%d ID 0x%lx\n",
                                   last, last->Flags, last->GadgetType, last->Activation,
                                   last->LeftEdge, last->TopEdge, last->Width, last->Height,
                                   last->GadgetID));
            last->GadgetType &= ~GTYP_REQGADGET;
        }
	
        pred = (struct Gadget *)&window->FirstGadget;
        requester = NULL;
    }

    /* Send all GA_RelSpecial BOOPSI gadgets in the list the GM_LAYOUT msg */
    if (!requester || requester->ReqLayer)
    {
        DEBUG_ADDGLIST(dprintf("AddGList: send layout message\n"));
        DoGMLayout(gadget, window, requester, numGad, TRUE, IntuitionBase);
        DEBUG_ADDGLIST(dprintf("AddGList: send layout message done ...\n"));
    }

    /* gadget list must NOT be modified while gadget accesses are possible */
#ifdef USEGADGETLOCK
    LOCKGADGET
#else
    LOCKWINDOWLAYERS(window);
#endif

    count = 0;
    while (position && pred->NextGadget)
    {
        position --;
        pred = pred->NextGadget;
        count ++;
        D(bug("count=%d\n", count));
    }
    D(bug("Finished iterating window list\n"));

    count2 = numGad;
    for (last=gadget; last->NextGadget && --count2; last=last->NextGadget);

    D(bug("Finished finding end of supplied glist\n"));
    last->NextGadget = pred->NextGadget;
    pred->NextGadget = gadget;

    DEBUG_ADDGLIST(dprintf("AddGList: Pos %ld\n", count));

    if (!requester || requester->ReqLayer)
    {
    #if 1
        /* Refresh REL gadgets first. Wizard.library (die, die, die!) seems to rely on that. */
        int_refreshglist(gadget, window, requester, numGad, REFRESHGAD_REL, 0, IntuitionBase);
        int_refreshglist(gadget, window, requester, numGad, 0, REFRESHGAD_REL, IntuitionBase);
    #else
        RefreshGList(gadget, window, requester, numGad);
    #endif
    }

#ifdef USEGADGETLOCK
    UNLOCKGADGET
#else
    UNLOCKWINDOWLAYERS(window);
#endif

    ReturnInt ("AddGList", UWORD, count);
    AROS_LIBFUNC_EXIT
} /* AddGList */
