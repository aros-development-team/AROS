/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$
    $Log$
    Revision 1.8  2000/07/09 22:04:44  stegerg
    (almost) no more flickering prop gadgets.
    there's still a strange gfx trash with the prop gadgets used by
    the BGUI demo GroupView (apart from the right/bottom window borders
    which have wrong color)

    When the prop gadget knob is moved right or down it doesnt clear everything
    away at the left/top side :-\ Dont know what happens here, other prop gadgets
    in window borders work fine, for example the ones in "More".

    Revision 1.7  1999/07/19 21:59:36  stegerg
    prop gadgets now in amiga look

    Revision 1.6  1998/10/20 16:46:00  hkiel
    Amiga Research OS

    Revision 1.5  1997/01/27 00:36:41  ldp
    Polish

    Revision 1.4  1996/12/10 14:00:06  aros
    Moved #include into first column to allow makedepend to see it.

    Revision 1.3  1996/11/08 11:28:04  aros
    All OS function use now Amiga types

    Moved intuition-driver protos to intuition_intern.h

    Revision 1.2  1996/10/24 15:51:22  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.1  1996/10/10 13:09:55  digulla
    New function: NewModifyProp()


    Desc:
    Lang: english
*/
#include "intuition_intern.h"
#include "propgadgets.h"

/*****************************************************************************

    NAME */
#include <intuition/intuition.h>
#include <proto/intuition.h>

	AROS_LH9(void, NewModifyProp,

/*  SYNOPSIS */
	AROS_LHA(struct Gadget    *, gadget, A0),
	AROS_LHA(struct Window    *, window, A1),
	AROS_LHA(struct Requester *, requester, A2),
	AROS_LHA(ULONG             , flags, D0),
	AROS_LHA(ULONG             , horizPot, D1),
	AROS_LHA(ULONG             , vertPot, D2),
	AROS_LHA(ULONG             , horizBody, D3),
	AROS_LHA(ULONG             , vertBody, D4),
	AROS_LHA(LONG              , numGad, D5),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 78, Intuition)

/*  FUNCTION
	Changes the values in the PropInfo-structure of a proportional
	gadget and refreshes the specified number of gadgets beginning
	at the proportional gadget. If numGad is 0 (zero), then no
	refreshing is done.

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
	numGad - How many gadgets to refresh. 0 means none (not even
		the current gadget) and -1 means all of them.

    RESULT
	None.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	NewModifyProp(), RefreshGadgets(), RefreshGList()

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    intuition_lib.fd and clib/intuition_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    struct PropInfo 	*pi;
    struct BBox 	old, new;
    BOOL		knobok1, knobok2;
    
    if ((gadget->GadgetType & GTYP_GTYPEMASK) != GTYP_PROPGADGET
	|| !gadget->SpecialInfo
    )
	return;

    pi = gadget->SpecialInfo;

    CalcBBox (window, gadget, &old);

    new = old;

    knobok1 = CalcKnobSize (gadget, &old);

    pi->Flags = flags;
    pi->HorizPot = horizPot;
    pi->VertPot = vertPot;
    pi->HorizBody = horizBody;
    pi->VertBody = vertBody;

    knobok2 = CalcKnobSize (gadget, &new);

    if (knobok2)
        RefreshPropGadgetKnob (gadget, knobok1 ? &old : 0, &new, window, IntuitionBase);

    if (numGad > 1 && gadget->NextGadget)
	RefreshGList (gadget->NextGadget, window, requester, numGad - 1);

    AROS_LIBFUNC_EXIT
    
} /* NewModifyProp */
