/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/
#include <proto/exec.h>
#include "realtime_intern.h"

/*****************************************************************************

    NAME */
#include <libraries/realtime.h>

    AROS_LH1(struct Conductor *, NextConductor,

/*  SYNOPSIS */

	AROS_LHA(struct Conductor *, previousConductor, A0),

/*  LOCATION */

	struct Library *, RealTimeBase, 12, RealTime)

/*  FUNCTION

    Return the next conductor on the conductor list. If 'previousConductor'
    is NULL, return the first conductor in the list; if not, return the
    conductor following 'previousConductor'. If 'previousConductor' is the
    last conductor, this function returns NULL.

    INPUTS

    previousConductor  --  The previous conductor or NULL to get the first
                           conductor.

    RESULT

    A pointer to the next conductor or NULL if there are no more conductors.

    NOTES

    You have to lock the conductors with LockRealTime(RT_CONDUCTORS)
    before calling this function.

    EXAMPLE

    BUGS

    SEE ALSO

    FindConductor(), LockRealTime(), UnlockRealTime()

    INTERNALS

    Should private conductors be filtered out?

    HISTORY

    26.7.99  SDuvan  implemented

******************************************************************************/

{
    AROS_LIBFUNC_INIT

    if (previousConductor == NULL)
    {
	return (struct Conductor *)GetHead((struct List *)&GPB(RealTimeBase)->rtb_ConductorList);
    }

    return (struct Conductor *)GetSucc((struct Node *)previousConductor);

    AROS_LIBFUNC_EXIT
} /* NextConductor */
