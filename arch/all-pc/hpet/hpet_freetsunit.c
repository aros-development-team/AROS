/*
    Copyright © 1995-2020, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "hpet_intern.h"

/*****************************************************************************

    NAME */
#include <proto/hpet.h>

	AROS_LH1(void, FreeTSUnit,

/*  SYNOPSIS */
	AROS_LHA(IPTR, unit, D0),

/*  LOCATION */
	struct HPETBase *, base, 3, Hpet)

/*  FUNCTION
	Free the specified HPET unit.

    INPUTS
	unit - a number of previously allocated HPET unit.

    RESULT
    	None.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    if (unit < base->unitCnt)
    {
        /* Very simple. Even don't need a semaphore. */
        base->units[unit].Owner = NULL;
    }

    AROS_LIBFUNC_EXIT
}
