/*
    (C) 1995-99 AROS - The Amiga Research OS
    $Id$

    Desc: Intuition function OpenWorkBench()
    Lang: english
*/
#include "intuition_intern.h"

/*****************************************************************************

    NAME */
#include <proto/intuition.h>

	AROS_LH0(ULONG, OpenWorkBench,

/*  SYNOPSIS */

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 35, Intuition)

/*  FUNCTION
	Attempt to open the Workbench screen.

    INPUTS
	None.

    RESULT
	Tries to (re)open WorkBench screen. If successful return value
	is a pointer to the screen structure, which shouldn't be used,
	because other programs may close the WorkBench and make the
	pointer invalid.
	If this function fails the return value in NULL

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	CloseWorkBench()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

struct Screen *wbscreen = NULL;

#warning TODO: Write intuition/OpenWorkBench()
    aros_print_not_implemented ("OpenWorkBench");

    return (ULONG)wbscreen;

    AROS_LIBFUNC_EXIT
} /* openWorkBench */
