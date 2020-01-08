/*
    Copyright © 2020, The AROS Development Team. All rights reserved.
    $Id$
*/

/*****************************************************************************

    NAME */
#include <proto/utility.h>

	AROS_LH3(LONG, Strlcpy,

/*  SYNOPSIS */
	AROS_LHA(STRPTR, destination, A0),
	AROS_LHA(CONST_STRPTR, source, A0),
	AROS_LHA(LONG, destination_size, D0),

/*  LOCATION */
	struct UtilityBase *, UtilityBase, 49, Utility)

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
} /*  */
