/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Intuition function OffGadget()
    Lang: english
*/
#include "intuition_intern.h"

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

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    gadget->Flags |= GFLG_DISABLED;
    RefreshGList (gadget, window, requester, 1);

    AROS_LIBFUNC_EXIT
} /* OffGadget */
