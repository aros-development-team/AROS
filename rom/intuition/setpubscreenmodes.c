/*
    (C) 1995-99 AROS - The Amiga Research OS
    $Id$

    Desc: Intuition function SetPubScreenModes()
    Lang: english
*/
#include "intuition_intern.h"

/*****************************************************************************

    NAME */
#include <proto/intuition.h>

	AROS_LH1(UWORD, SetPubScreenModes,

/*  SYNOPSIS */
	AROS_LHA(UWORD, modes, D0),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 91, Intuition)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

#warning TODO: Write intuition/SetPubScreenModes()
    aros_print_not_implemented ("SetPubScreenModes");

    return 0;

    AROS_LIBFUNC_EXIT
} /* SetPubScreenModes */
