/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Intuition function MakeScreen()
    Lang: english
*/
#include "intuition_intern.h"

/*****************************************************************************

    NAME */
#include <proto/intuition.h>

	AROS_LH1(LONG, MakeScreen,

/*  SYNOPSIS */
	AROS_LHA(struct Screen *, screen, A0),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 63, Intuition)

/*  FUNCTION

    INPUTS
	Pointer to your custom screen.

    RESULT
	Zero for success, non-zero for failure.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	RemakeDisplay(), RethinkDisplay(), graphics.library/MakeVPort(),

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

#warning TODO: Write intuition/MakeScreen()
    aros_print_not_implemented ("MakeScreen");

    return 0L;

    AROS_LIBFUNC_EXIT
} /* MakeScreen */
