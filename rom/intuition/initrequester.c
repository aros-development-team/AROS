/*
    (C) 1995-99 AROS - The Amiga Research OS
    $Id$

    Desc: Intuition function InitRequester()
    Lang: english
*/
#include "intuition_intern.h"

/*****************************************************************************

    NAME */
#include <proto/intuition.h>

	AROS_LH1(void, InitRequest,

/*  SYNOPSIS */
	AROS_LHA(struct Requester *, requester, A0),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 23, Intuition)

/*  FUNCTION
	Initialize struct Requester for use.
	Basically sets all values to NULL or zero.

    INPUTS
	requester - The struct Requester to be initialized

    RESULT
	None.

    NOTES
	Be sure to call InitRequester *before* changing values. Unlike
	stated in the early versions of the Intuition Reference Manual
	this function will kill all your infos in the struct.

    EXAMPLE

    BUGS

    SEE ALSO
	Request(), EndRequest()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    /* Set values to zero */
    memset (requester, 0, sizeof(struct Requester) );

    /* Set non-zero values */
#warning TODO: Do we need to set non-zero values?

    AROS_LIBFUNC_EXIT
} /* InitRequester */
