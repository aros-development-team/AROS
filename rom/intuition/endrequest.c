/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Intuition function EndRequest()
    Lang: english
*/
#include "intuition_intern.h"

/*****************************************************************************

    NAME */
#include <proto/intuition.h>

	AROS_LH2(void, EndRequest,

/*  SYNOPSIS */
	AROS_LHA(struct Requester *, requester, A0),
	AROS_LHA(struct Window *   , window, A1),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 20, Intuition)

/*  FUNCTION
	Remove a requester from the specified window.
	Other open requesters of this window stay alive.

    INPUTS
	requester - The requester to be deleted
	window - The window to which the requester belongs

    RESULT
	None.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	InitRequester(), Request()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

#warning TODO: Write intuition/EndRequest()
    aros_print_not_implemented ("EndRequest");

    AROS_LIBFUNC_EXIT
} /* EndRequest */
