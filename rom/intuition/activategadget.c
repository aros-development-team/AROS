/*
    (C) 1995-99 AROS - The Amiga Research OS
    $Id$

    Desc: Intuition function ActivateGadget()
    Lang: english
*/
#include "intuition_intern.h"

/*****************************************************************************

    NAME */
#include <proto/intuition.h>

	AROS_LH3(BOOL, ActivateGadget,

/*  SYNOPSIS */
	AROS_LHA(struct Gadget *   , gadget, A0),
	AROS_LHA(struct Window *   , window, A1),
	AROS_LHA(struct Requester *, requester, A2),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 77, Intuition)

/*  FUNCTION
	Activates the specified gadget.

    INPUTS
	gadget - The gadget to activate
	window - The window which contains the gadget
	requester - The requester which contains the gadget or
		NULL if it is not a requester gadget

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

#warning TODO: Write intuition/ActivateGadget()
    aros_print_not_implemented ("ActivateGadget");

    return FALSE;

    AROS_LIBFUNC_EXIT
} /* ActivateGadget */
