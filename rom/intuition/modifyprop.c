/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.2  1996/10/24 15:51:22  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.1  1996/08/28 17:55:35  digulla
    Proportional gadgets
    BOOPSI


    Desc:
    Lang: english
*/
#include "intuition_intern.h"

/*****************************************************************************

    NAME */
	#include <intuition/intuition.h>
	#include <clib/intuition_protos.h>

	AROS_LH8(void, ModifyProp,

/*  SYNOPSIS */
	AROS_LHA(struct Gadget    *, gadget, A0),
	AROS_LHA(struct Window    *, window, A1),
	AROS_LHA(struct Requester *, requester, A2),
	AROS_LHA(unsigned long     , flags, D0),
	AROS_LHA(unsigned long     , horizPot, D1),
	AROS_LHA(unsigned long     , vertPot, D2),
	AROS_LHA(unsigned long     , horizBody, D3),
	AROS_LHA(unsigned long     , vertBody, D4),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 26, Intuition)

/*  FUNCTION
	Changes the values in the PropInfo-structure of a proportional
	gadget and refreshes the display.

    INPUTS
	gadget - Must be a PROPGADGET
	window - The window which contains the gadget
	requester - If the gadget has GTYP_REQGADGET set, this must be
		non-NULL.
	flags - New flags
	horizPot - New value for the HorizPot field of the PropInfo
	vertPot - New value for the VertPot field of the PropInfo
	horizBody - New value for the HorizBody field of the PropInfo
	vertBody - New value for the VertBody field of the PropInfo

    RESULT
	None.

    NOTES
	This function causes all gadgets from this gadget to the end of
	the gadget list to be refreshed. If you want a better behaviour,
	use NewModifProp().

    EXAMPLE

    BUGS

    SEE ALSO
	NewModifyProp()

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    intuition_lib.fd and clib/intuition_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)
    struct PropInfo * pi;

    if ((gadget->GadgetType & GTYP_GTYPEMASK) != GTYP_PROPGADGET
	|| !gadget->SpecialInfo
    )
	return;

    pi = gadget->SpecialInfo;

    pi->Flags = flags;
    pi->HorizPot = horizPot;
    pi->VertPot = vertPot;
    pi->HorizBody = horizBody;
    pi->VertBody = vertBody;

    RefreshGadgets (gadget, window, requester);

    AROS_LIBFUNC_EXIT
} /* ModifyProp */
