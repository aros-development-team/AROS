/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Intuition function ClearDMRequest
    Lang: english
*/
#include "intuition_intern.h"

/*****************************************************************************

    NAME */
#include <proto/intuition.h>

	AROS_LH1(BOOL, ClearDMRequest,

/*  SYNOPSIS */
	AROS_LHA(struct Window *, window, A0),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 8, Intuition)

/*  FUNCTION

    INPUTS
	window - The window from which the DMRequest is to be cleared

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	SetDMRequest(), Request()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

#warning TODO: Write intuition/ClearDMRequest()
    aros_print_not_implemented ("ClearDMRequest");

    return FALSE;

    AROS_LIBFUNC_EXIT
} /* ClearDMRequest */
