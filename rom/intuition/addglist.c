/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include "intuition_intern.h"

/*****************************************************************************

    NAME */
#include <intuition/intuition.h>
#include <proto/intuition.h>
#include "boopsigadgets.h"

#undef DEBUG
#define DEBUG 0
#	include <aros/debug.h>

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
    struct Gadget * pred;
    struct Gadget * last;
    UWORD count;

    D(bug("AddGList()\n"));
    pred = (struct Gadget *)&window->FirstGadget;
    count = 0;
    
    /* Send all GA_RelSpecial BOOPSI gadgets in the list the GM_LAYOUT msg */
    DoGMLayout(gadget, window, requester, numGad, TRUE, IntuitionBase);
    
    while (position && pred->NextGadget)
    {
	position --;
	pred = pred->NextGadget;
	count ++;
	D(bug("count=%d\n", count));
    }
    D(bug("Finished iterating window list\n"));

    for (last=gadget; last->NextGadget && --numGad; last=last->NextGadget);

    D(bug("Finished finding end of supplied glist\n"));
    last->NextGadget = pred->NextGadget;
    pred->NextGadget = gadget;
    
    ReturnInt ("AddGList", UWORD, count);
    AROS_LIBFUNC_EXIT
} /* AddGList */
