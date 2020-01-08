/*
    Copyright © 2020, The AROS Development Team. All rights reserved.
    $Id$
*/

/*****************************************************************************

    NAME */
#include <proto/utility.h>

	AROS_LH2(VOID, ClearMem,

/*  SYNOPSIS */
	AROS_LHA(APTR, destination, A0),
	AROS_LHA(ULONG, size, D0),

/*  LOCATION */
	struct UtilityBase *, UtilityBase, 50, Utility)

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
} /*  ClearMem */
