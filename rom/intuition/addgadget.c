/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Add a single gadget to a window.
    Lang: english
*/
#include "intuition_intern.h"

/*****************************************************************************

    NAME */
#include <intuition/intuition.h>
#include <proto/intuition.h>

	AROS_LH3(UWORD, AddGadget,

/*  SYNOPSIS */
	AROS_LHA(struct Window *, window, A0),
	AROS_LHA(struct Gadget *, gadget, A1),
	AROS_LHA(ULONG          , position, D0),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 7, Intuition)

/*  FUNCTION
	Adds a single gadget to a window.

    INPUTS
	window - Add gadget to this window
	gadget - Add this gadget
	position - The position to add the gadget in the list of
		gadgets already in the window. Use 0 to insert the
		gadget before all others or ~0 to append it to the
		list.

    RESULT
	The position where the gadget was really inserted.

    NOTES
	This just adds the gadget to the list. It will not be visible
	until you refresh the window.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    return AddGList (window, gadget, position, 1, NULL);
    AROS_LIBFUNC_EXIT
} /* AddGadget */
