/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang: english
*/
#include "intuition_intern.h"

/*****************************************************************************

    NAME */
	#include <clib/intuition_protos.h>

	AROS_LH1(void, ScreenToFront,

/*  SYNOPSIS */
	AROS_LHA(struct Screen *, screen, A0),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 42, Intuition)

/*  FUNCTION
	Move the screen in front of all other screens.

    INPUTS
	screen - This screen

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	ScreenToBack()

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    intuition_lib.fd and clib/intuition_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    /* Not yet */

    AROS_LIBFUNC_EXIT
} /* ScreenToFront */
