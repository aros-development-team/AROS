/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include <proto/layers.h>
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
