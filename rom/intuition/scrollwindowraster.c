/*
    (C) 1995-99 AROS - The Amiga Research OS
    $Id$

    Desc: Intuition function ScrollWindowRaster()
    Lang: english
*/
#include "intuition_intern.h"

/*****************************************************************************

    NAME */
#include <proto/intuition.h>

	AROS_LH7(void, ScrollWindowRaster,

/*  SYNOPSIS */
	AROS_LHA(struct Window *, win , A1),
	AROS_LHA(WORD           , dx  , D0),
	AROS_LHA(WORD           , dy  , D1),
	AROS_LHA(WORD           , xmin, D2),
	AROS_LHA(WORD           , ymin, D3),
	AROS_LHA(WORD           , xmax, D4),
	AROS_LHA(WORD           , ymax, D5),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 133, Intuition)

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

#warning TODO: Write intuition/ScrollWindowRaster()
    aros_print_not_implemented ("ScrollWindowRaster");

    AROS_LIBFUNC_EXIT
} /* ScrollWindowRaster */
