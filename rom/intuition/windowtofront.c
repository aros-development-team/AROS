/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Move window in front of all other windows
    Lang: english
*/
#include "intuition_intern.h"

/*****************************************************************************

    NAME */
#include <intuition/intuition.h>
#include <proto/intuition.h>

	AROS_LH1(void, WindowToFront,

/*  SYNOPSIS */
	AROS_LHA(struct Window *, window, A0),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 52, Intuition)

/*  FUNCTION
	Bring a window to the front (ie. before any other window).

    INPUTS
	window - Which window

    RESULT
	None.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    intuition_lib.fd and clib/intuition_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    AllocAndSendIntuiActionMsg(AMCODE_WINDOWTOFRONT, window, IntuitionBase);

    AROS_LIBFUNC_EXIT
    
} /* WindowToFront */
