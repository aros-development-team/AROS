/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.10  1996/12/10 14:00:08  aros
    Moved #include into first column to allow makedepend to see it.

    Revision 1.9  1996/11/08 11:28:04  aros
    All OS function use now Amiga types

    Moved intuition-driver protos to intuition_intern.h

    Revision 1.8  1996/10/25 14:25:56  aros
    Handle BOOPSI Gadgets

    Revision 1.7  1996/10/24 15:51:24  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.6  1996/10/10 13:31:07  digulla
    Move Gadget code in own files

    Revision 1.5  1996/10/04 15:33:57  digulla
    Added a comment

    Revision 1.4  1996/10/02 18:12:03  digulla
    Draw text after border for IMAGE and BORDER gadgets and before for HCOMP-type
	gadgets (The text of IMAGE-Gadgets was not visible)

    Revision 1.3  1996/08/29 13:57:38  digulla
    Commented
    Moved common code from driver to Intuition

    Revision 1.2  1996/08/29 07:50:49  digulla
    Fixed a small bug in PropGadgets. The jumpsize of the knob was too small.

    Revision 1.1  1996/08/28 17:55:36  digulla
    Proportional gadgets
    BOOPSI


    Desc:
    Lang: english
*/
#include <clib/graphics_protos.h>
#include "intuition_intern.h"
#include "boolgadgets.h"
#include "boopsigadgets.h"
#include "propgadgets.h"

/*****************************************************************************

    NAME */
#include <intuition/intuition.h>
#include <clib/intuition_protos.h>

	AROS_LH4(void, RefreshGList,

/*  SYNOPSIS */
	AROS_LHA(struct Gadget    *, gadgets, A0),
	AROS_LHA(struct Window    *, window, A1),
	AROS_LHA(struct Requester *, requester, A2),
	AROS_LHA(LONG              , numGad, D0),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 72, Intuition)

/*  FUNCTION
	Refresh (draw anew) the specified number of gadgets starting
	at the specified gadget.

    INPUTS
	gadgets - This is the first gadget which will be refreshed.
	window - The window which contains the gadget
	requester - If the gadget has GTYP_REQGADGET set, this must be
		a pointer to a Requester; otherwise the value is
		ignored.
	numGad - How many gadgets should be refreshed. The value
		may range from 0 to MAXLONG. If there are less gadgets
		in the list than numGad, only the gadgets in the
		list will be refreshed.

    RESULT
	None.

    NOTES
	This function *must not* be called inside a
	BeginRefresh()/EndRefresh() pair.

    EXAMPLE
	// Refresh one gadget
	RefreshGList (&gadget, win, NULL, 1);

	// Refresh all gadgets in the window
	RefreshGList (win->FirstGadget, win, NULL, -1L);

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    intuition_lib.fd and clib/intuition_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    for ( ; gadgets && numGad; gadgets=gadgets->NextGadget, numGad --)
    {
	switch (gadgets->GadgetType & GTYP_GTYPEMASK)
	{
	case GTYP_BOOLGADGET:
	    RefreshBoolGadget (gadgets, window, IntuitionBase);
	    break;

	case GTYP_GADGET0002:
	    break;

	case GTYP_PROPGADGET:
	    RefreshPropGadget (gadgets, window, IntuitionBase);
	    break;

	case GTYP_STRGADGET:
	    break;

	case GTYP_CUSTOMGADGET:
	    RefreshBoopsiGadget (gadgets, window, IntuitionBase);
	    break;

	} /* switch GadgetType */
    }

    AROS_LIBFUNC_EXIT
} /* RefreshGList */
