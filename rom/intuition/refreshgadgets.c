/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.2  1996/08/29 13:57:38  digulla
    Commented
    Moved common code from driver to Intuition

    Revision 1.1  1996/08/28 17:55:35  digulla
    Proportional gadgets
    BOOPSI


    Desc:
    Lang: english
*/
#include "intuition_intern.h"

/*****************************************************************************

    NAME */
	#include <clib/intuition_protos.h>

	__AROS_LH3(void, RefreshGadgets,

/*  SYNOPSIS */
	__AROS_LHA(struct Gadget    *, gadgets, A0),
	__AROS_LHA(struct Window    *, window, A1),
	__AROS_LHA(struct Requester *, requester, A2),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 37, Intuition)

/*  FUNCTION
	Refreshes all gadgets starting at the specified gadget.

    INPUTS
	gadgets - The first gadget to be refreshed
	window - The gadget must be in this window
	requester - If any gadget has GTYP_REQGADGET set, this must
		point to a valid Requester. Otherwise the value is
		ignored.

    RESULT
	None.

    NOTES

    EXAMPLE
	// Refresh all gadgets of a window
	RefreshGadgets (win->FirstGadget, win, NULL);

    BUGS

    SEE ALSO
	RefreshGList()

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    intuition_lib.fd and clib/intuition_protos.h

*****************************************************************************/
{
    __AROS_FUNC_INIT
    __AROS_BASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    RefreshGList (gadgets, window, requester, ~0L);

    __AROS_FUNC_EXIT
} /* RefreshGadgets */
