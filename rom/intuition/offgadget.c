/*
    Copyright � 1995-2013, The AROS Development Team. All rights reserved.
    Copyright � 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include "intuition_intern.h"
#include <intuition/gadgetclass.h>

/*****************************************************************************

    NAME */
#include <intuition/intuition.h>
#include <proto/intuition.h>

        AROS_LH3(void, OffGadget,

/*  SYNOPSIS */
        AROS_LHA(struct Gadget    *, gadget, A0),
        AROS_LHA(struct Window    *, window, A1),
        AROS_LHA(struct Requester *, requester, A2),

/*  LOCATION */
        struct IntuitionBase *, IntuitionBase, 29, Intuition)

/*  FUNCTION
        Disable a gadget. It will appear ghosted.

    INPUTS
        gadget - The gadget to deactivate
        window - The window, the gadget is in
        requester - The requester, the gadget is in or NULL if the
            gadget is in no requester

    RESULT
        None.

    NOTES
        This function will update the gadget (unlike the original function
        which would update all gadgets in the window).

    EXAMPLE

    BUGS

    SEE ALSO
        AddGadget(), RefreshGadgets()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    DEBUG_OFFGADGET(dprintf("OffGadget: gadget 0x%lx window 0x%lx req 0x%lx\n",
                            gadget, window, requester));

    SANITY_CHECK(window)
    SANITY_CHECK(gadget)

    if ((gadget->GadgetType & GTYP_GTYPEMASK) == GTYP_CUSTOMGADGET)
    {
        struct TagItem set_tags[] =
        {
            {GA_Disabled, TRUE	},
            {TAG_END            }
        };

        SetGadgetAttrsA(gadget, window, requester, set_tags);
    }
    else
    {
        AROS_ATOMIC_OR(gadget->Flags, GFLG_DISABLED);
    }

    RefreshGList (gadget, window, requester, 1);

    AROS_LIBFUNC_EXIT
} /* OffGadget */
