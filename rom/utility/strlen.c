/*
    Copyright © 2020, The AROS Development Team. All rights reserved.
    $Id$
*/

/*****************************************************************************

    NAME */
#include <proto/utility.h>

	AROS_LH1(ULONG, Strlen,

/*  SYNOPSIS */
	AROS_LHA(STRPTR, string, A0),

/*  LOCATION */
	struct UtilityBase *, UtilityBase, 52, Utility)

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

    if (string)
    {
        CONST_STRPTR str_start = (CONST_STRPTR)string;

        while (*string++);

        return (ULONG)(((IPTR)string) - ((IPTR)str_start));
    }
    return 0;

    AROS_LIBFUNC_EXIT
} /* Strlen */
