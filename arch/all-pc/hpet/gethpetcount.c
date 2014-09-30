/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "hpet_intern.h"

/*****************************************************************************

    NAME */
#include <proto/hpet.h>

	AROS_LH0(ULONG, GetHPETCount,

/*  SYNOPSIS */

/*  LOCATION */
	struct HPETBase *, base, 1, Hpet)

/*  FUNCTION
	Return the total number of HPET units in the system.

    INPUTS
	None

    RESULT
	Total number of HPET units

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    return base->unitCnt;

    AROS_LIBFUNC_EXIT
}
