/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$
    $Log$
    Revision 1.6  1998/10/20 16:46:03  hkiel
    Amiga Research OS

    Revision 1.5  1997/01/27 00:36:42  ldp
    Polish

    Revision 1.4  1996/12/10 14:00:07  aros
    Moved #include into first column to allow makedepend to see it.

    Revision 1.3  1996/10/24 15:51:24  aros
    Use the official AROS macros over the __AROS versions.

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
#include <proto/intuition.h>

	AROS_LH3(void, RefreshGadgets,

/*  SYNOPSIS */
	AROS_LHA(struct Gadget    *, gadgets, A0),
	AROS_LHA(struct Window    *, window, A1),
	AROS_LHA(struct Requester *, requester, A2),

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
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    RefreshGList (gadgets, window, requester, ~0L);

    AROS_LIBFUNC_EXIT
} /* RefreshGadgets */
