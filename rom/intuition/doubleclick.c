/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <clib/macros.h>
#include "intuition_intern.h"

/*****************************************************************************

    NAME */
#include <intuition/intuition.h>
#include <proto/intuition.h>

	AROS_LH4(BOOL, DoubleClick,

/*  SYNOPSIS */
	AROS_LHA(ULONG, sSeconds, D0),
	AROS_LHA(ULONG, sMicros, D1),
	AROS_LHA(ULONG, cSeconds, D2),
	AROS_LHA(ULONG, cMicros, D3),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 17, Intuition)

/*  FUNCTION
	Check if two times are within the doubleclick intervall.

    INPUTS
	sSeconds, sMicros - Seconds and microseconds of the first event.
	cSeconds, cMicros - Seconds and microseconds of the second event.

    RESULT
	TRUE if the times are within the doubleclick intervall, FALSE
	otherwise.

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

    if (ABS(cSeconds - sSeconds) <= 1)
    {
	ULONG base;

	base = MIN(cSeconds, sSeconds);

	sMicros += 1000000 * (sSeconds - base);
	cMicros += 1000000 * (cSeconds - base);

	base = ABS((LONG)(sMicros - cMicros));

	return (base <= 500000);
    }

    return FALSE;
    AROS_LIBFUNC_EXIT
} /* DoubleClick */
