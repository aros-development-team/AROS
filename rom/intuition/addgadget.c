/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
 
    Add a single gadget to a window.
*/

#include <proto/layers.h>
#include "intuition_intern.h"

/*****************************************************************************
 
    NAME */
#include <intuition/intuition.h>
#include <proto/intuition.h>

AROS_LH3(UWORD, AddGadget,

         /*  SYNOPSIS */
         AROS_LHA(struct Window *, window, A0),
         AROS_LHA(struct Gadget *, gadget, A1),
         AROS_LHA(ULONG          , position, D0),

         /*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 7, Intuition)

/*  FUNCTION
    Adds a single gadget to a window.
 
    INPUTS
    window - Add gadget to this window
    gadget - Add this gadget
    position - The position to add the gadget in the list of
        gadgets already in the window. Use 0 to insert the
        gadget before all others or ~0 to append it to the
        list.
 
    RESULT
    The position where the gadget was really inserted.
 
    NOTES
    This just adds the gadget to the list. It will not be visible
    until you refresh the window.
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
 
    INTERNALS
 
    HISTORY
 
*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    struct Gadget *pred;
    UWORD   	   count;

    EXTENDUWORD(position);

    DEBUG_ADDGADGET(dprintf("AddGadget: Window 0x%lx Gadget 0x%lx Pos %ld\n",
                            window, gadget, position));

    if (!window) return 0;
    if (!gadget) return 0;

    gadget->GadgetType &= ~GTYP_REQGADGET;

    pred = (struct Gadget *)&window->FirstGadget;
    count = 0;

    /* Send all GA_RelSpecial BOOPSI gadgets in the list the GM_LAYOUT msg */
    DoGMLayout(gadget, window, NULL, 1, TRUE, IntuitionBase);

    //obtain semaphore here. gadget list must NOT be accessed while it's being modified!
#ifdef USEGADGETLOCK
    LOCKGADGET
#else
    LOCKWINDOWLAYERS(window);
#endif

    while (position && pred->NextGadget)
    {
        position --;
        pred = pred->NextGadget;
        count ++;
    }

    gadget->NextGadget = pred->NextGadget;
    pred->NextGadget = gadget;

#ifdef USEGADGETLOCK
    UNLOCKGADGET
#else
    UNLOCKWINDOWLAYERS(window);
#endif

    DEBUG_ADDGADGET(dprintf("AddGadget: Pos %ld\n", count));

    return count;
    AROS_LIBFUNC_EXIT
} /* AddGadget */
