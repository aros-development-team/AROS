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

    AROS_LH1(APTR, LockRealTime,

/*  SYNOPSIS */

	AROS_LHA(ULONG, lockType, D0),

/*  LOCATION */

	struct Library *, RealTimeBase, 5, RealTime)

/*  FUNCTION

    Lock a RealTime.library internal semaphore.

    INPUTS

    lockType  --  The type of lock to aquire, see <libraries/realtime.h> for
                  further information.

    RESULT

    A handle to pass to UnlockRealTime() to unlock the semaphore. If 'lockType'
    is invalid, NULL is returned.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    UnlockRealTime()

    INTERNALS

    HISTORY

    26.7.99  SDuvan  implemented

******************************************************************************/

{
    AROS_LIBFUNC_INIT

    if(lockType >= RT_MAXLOCK)
	return NULL;

    ObtainSemaphore(&GPB(RealTimeBase)->rtb_Locks[lockType]);

    return (APTR)(&GPB(RealTimeBase)->rtb_Locks[lockType]);

    AROS_LIBFUNC_EXIT
} /* LockRealTime */
