/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Intuition function RethinkDisplay()
    Lang: english
*/
#include "intuition_intern.h"

/*****************************************************************************

    NAME */
#include <proto/intuition.h>

	AROS_LH0(LONG, RethinkDisplay,

/*  SYNOPSIS */

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 65, Intuition)

/*  FUNCTION
	Check and update, ie. redisplay the whole Intuition display.

    INPUTS
	None.

    RESULT
	Zero for success, non-zero for failure.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	RemakeDisplay(), MakeScreen(), graphics.library/MakeVPort(),
	graphics.library/MrgCop(), graphics.library/LoadView()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

#warning TODO: Write intuition/RethinkDisplay()
    aros_print_not_implemented ("RethinkDisplay");

    return 0L;

    AROS_LIBFUNC_EXIT
} /* RethinkDisplay */
