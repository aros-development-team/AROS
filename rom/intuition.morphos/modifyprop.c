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

AROS_LH8(void, ModifyProp,

         /*  SYNOPSIS */
         AROS_LHA(struct Gadget    *, gadget, A0),
         AROS_LHA(struct Window    *, window, A1),
         AROS_LHA(struct Requester *, requester, A2),
         AROS_LHA(ULONG             , flags, D0),
         AROS_LHA(ULONG             , horizPot, D1),
         AROS_LHA(ULONG             , vertPot, D2),
         AROS_LHA(ULONG             , horizBody, D3),
         AROS_LHA(ULONG             , vertBody, D4),

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
        || !gadget->SpecialInfo || !window)
    {
        return;
    }
    
    EXTENDUWORD(horizPot);
    EXTENDUWORD(vertPot);
    EXTENDUWORD(horizBody);
    EXTENDUWORD(vertBody);
    EXTENDUWORD(flags);

    pi = gadget->SpecialInfo;

    /* We don't want the inputhandler to redraw the knob with values
     * partially changed, so use some protection.
     */
#ifdef USEGADGETLOCK
    ObtainSemaphore(&GetPrivIBase(IntuitionBase)->InputHandlerLock);
#else
    LOCKWINDOWLAYERS(window);
#endif
    pi->Flags = flags;
    pi->HorizPot = horizPot;
    pi->VertPot = vertPot;
    pi->HorizBody = horizBody;
    pi->VertBody = vertBody;

    RefreshGadgets (gadget, window, requester);

#ifdef USEGADGETLOCK
    ReleaseSemaphore(&GetPrivIBase(IntuitionBase)->InputHandlerLock);
#else
    UNLOCKWINDOWLAYERS(window);
#endif

    AROS_LIBFUNC_EXIT
} /* ModifyProp */
