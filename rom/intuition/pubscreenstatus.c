/*
    (C) 1995-99 AROS - The Amiga Research OS
    $Id$

    Desc: Intuition function PubScreenStatus()
    Lang: english
*/
#include "intuition_intern.h"

/*****************************************************************************

    NAME */
#include <proto/intuition.h>

	AROS_LH2(UWORD, PubScreenStatus,

/*  SYNOPSIS */
	AROS_LHA(struct Screen *, screen     , A0),
	AROS_LHA(UWORD          , statusflags, D0),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 92, Intuition)

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

#warning TODO: Write intuition/PubScreenStatus()
    aros_print_not_implemented ("PubScreenStatus");

    return NULL;

    AROS_LIBFUNC_EXIT
} /* PubScreenStatus */
