/*
    (C) 1995-99 AROS - The Amiga Research OS
    $Id$

    Desc: Intuition function WBenchToBack()
    Lang: english
*/
#include "intuition_intern.h"

/*****************************************************************************

    NAME */
#include <proto/intuition.h>

	AROS_LH0(BOOL, WBenchToBack,

/*  SYNOPSIS */

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 56, Intuition)

/*  FUNCTION
	Bring the WorkBench behind all other screens.

    INPUTS

    RESULT
	TRUE if the Workbench screen is open, FALSE else.

    NOTES
	This function does not influence the position of the screen,
	it just changes the depth-arrangement of the screens.

    EXAMPLE

    BUGS

    SEE ALSO
	ScreenToBack(), ScreenToFront(), WBenchToFront()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

#warning TODO: Write intuition/WBenchToBack()
    aros_print_not_implemented ("WBenchToBack");

    return FALSE;

    AROS_LIBFUNC_EXIT
} /* WBenchToBack */
