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

	AROS_LH3(void, OnGadget,

/*  SYNOPSIS */
	AROS_LHA(struct Gadget    *, gadget, A0),
	AROS_LHA(struct Window    *, window, A1),
	AROS_LHA(struct Requester *, requester, A2),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 31, Intuition)

/*  FUNCTION
	Enable a gadget. It will appear normal.

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

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    gadget->Flags &= ~GFLG_DISABLED;
    RefreshGList (gadget, window, requester, 1);

    AROS_LIBFUNC_EXIT
} /* OnGadget */
