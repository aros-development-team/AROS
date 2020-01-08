/*
    Copyright © 2020, The AROS Development Team. All rights reserved.
    $Id$
*/

/*****************************************************************************

    NAME */
#include <proto/utility.h>

	AROS_LH3(APTR, SetMem,

/*  SYNOPSIS */
	AROS_LHA(APTR, destination, A0),
	AROS_LHA(UBYTE, value, D0),
	AROS_LHA(LONG, length, D1),

/*  LOCATION */
	struct UtilityBase *, UtilityBase, 47, Utility)

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


    AROS_LIBFUNC_EXIT
} /*  SetMem */
