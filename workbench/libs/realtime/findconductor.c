
/*
    (C) 1999-2001 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: English
*/

/*****************************************************************************

    NAME */

#include <proto/exec.h>
#include "realtime_intern.h"

    AROS_LH1(struct Conductor *, FindConductor,

/*  SYNOPSIS */

	AROS_LHA(STRPTR, name, A0),

/*  LOCATION */

	struct Library *, RealTimeBase, 13, RealTime)

/*  FUNCTION

    Get the conductor with name 'name' or NULL if no conductor exists
    with that name.

    INPUTS

    name   --  The name of the conductor to find.

    RESULT

    A pointer to the conductor you wanted or NULL if it didn't exist.

    NOTES

    You have to lock the conductors with LockRealTime(RT_CONDUCTORS)
    before calling this function.

    EXAMPLE

    BUGS

    SEE ALSO

    NextConductor(), LockRealTime(), UnlockRealTime()

    INTERNALS

    HISTORY

    26.7.99  SDuvan  implemented

******************************************************************************/

{
    AROS_LIBFUNC_INIT

    struct Conductor *conductor = (struct Conductor *)
	FindName((struct List *)&GPB(RealTimeBase)->rtb_ConductorList, name);

    if (conductor == NULL)
    {
	return NULL;
    }

    if (conductor->cdt_Flags & CONDUCTF_PRIVATE)
    {
	return NULL;
    }

    return conductor;

    AROS_LIBFUNC_EXIT
} /* FindConductor */
