/*
    (C) 1995-99 AROS - The Amiga Research OS
    $Id$

    Desc: Intuition function SetMouseQueue()
    Lang: english
*/
#include "intuition_intern.h"

/*****************************************************************************

    NAME */
	#include <clib/intuition_protos.h>

	AROS_LH2(LONG, SetMouseQueue,

/*  SYNOPSIS */
	AROS_LHA(struct Window *, window, A0),
	AROS_LHA(UWORD          , queuelength, D0),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 83, Intuition)

/*  FUNCTION
	Change the number of mouse messages for your window to be allowed
	to be outstanding.

    INPUTS
	window - the window
	queuelength - the number of mouse messages to be allowed to be
		outstanding

    RESULT
	Returns -1 if the window is unknown otherwise the old value of the
	queuelength is returned.

    NOTES
	There should be a function for changing the repeat key queue limit, too.

    EXAMPLE

    BUGS

    SEE ALSO
	OpenWindow()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

#warning TODO: Write intuition/SetMouseQueue()
    aros_print_not_implemented ("SetMouseQueue");

    return -1;

    AROS_LIBFUNC_EXIT
} /* SetMouseQueue */
