/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Intuition function TimedDisplayAlert()
    Lang: english
*/
#include "intuition_intern.h"

/*****************************************************************************

    NAME */
#include <proto/intuition.h>

	AROS_LH4(BOOL, TimedDisplayAlert,

/*  SYNOPSIS */
	AROS_LHA(ULONG  , alertnumber, D0),
	AROS_LHA(UBYTE *, string     , A0),
	AROS_LHA(UWORD  , height     , D1),
	AROS_LHA(ULONG  , time       , A1),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 137, Intuition)

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

#warning TODO: Write intuition/TimedDisplayAlert()
    aros_print_not_implemented ("TimedDisplayAlert");

    return FALSE;

    AROS_LIBFUNC_EXIT
} /* TimedDisplayAlert */
