/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Intuition function Request()
    Lang: english
*/
#include "intuition_intern.h"

/*****************************************************************************

    NAME */
#include <proto/intuition.h>

	AROS_LH2(BOOL, Request,

/*  SYNOPSIS */
	AROS_LHA(struct Requester *, requester, A0),
	AROS_LHA(struct Window *   , window, A1),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 40, Intuition)

/*  FUNCTION
	Add a requester to specified window and display it.

    INPUTS
	requester - The requester to be displayed
	window - The window to which the requester belongs

    RESULT
	TRUE if requester was opened successfully, FALSE else.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	EndRequest(), InitRequester()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

#warning TODO: Write intuition/Request()
    aros_print_not_implemented ("Request");

    return FALSE;

    AROS_LIBFUNC_EXIT
} /* Request */
