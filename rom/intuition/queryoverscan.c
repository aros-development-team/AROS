/*
    (C) 1995-99 AROS - The Amiga Research OS
    $Id$

    Desc: Intuition function QueryOverscan()
    Lang: english
*/
#include "intuition_intern.h"

/*****************************************************************************

    NAME */
#include <proto/intuition.h>

	AROS_LH3(LONG, QueryOverscan,

/*  SYNOPSIS */
	AROS_LHA(ULONG             , displayid, A0),
	AROS_LHA(struct Rectangle *, rect     , A1),
	AROS_LHA(WORD              , oscantype, D0),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 79, Intuition)

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

#warning TODO: Write intuition/QueryOverscan()
    aros_print_not_implemented ("QueryOverscan");

    return 0L;

    AROS_LIBFUNC_EXIT
} /* QueryOverscan */
