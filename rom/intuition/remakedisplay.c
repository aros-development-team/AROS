/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Intuition function RemakeDisplay()
    Lang: english
*/
#include "intuition_intern.h"

/*****************************************************************************

    NAME */
#include <proto/intuition.h>

	AROS_LH0(LONG, RemakeDisplay,

/*  SYNOPSIS */

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 64, Intuition)

/*  FUNCTION

    INPUTS
	None.

    RESULT
	Zero for success, non-zero for failure.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	RethinkDisplay(), MakeScreen(), graphics.library/MakeVPort(),
	graphics.library/MrgCop(), graphics.library/LoadView()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

#warning TODO: Write intuition/RemakeDisplay()
    aros_print_not_implemented ("RemakeDisplay");

    return 0L;

    AROS_LIBFUNC_EXIT
} /* RemakeDisplay */
