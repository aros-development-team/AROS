
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

    AROS_LH1(VOID, UnlockRealTime,

/*  SYNOPSIS */

	AROS_LHA(APTR, lockHandle, A0),

/*  LOCATION */

	struct Library *, RTBase, 6, RealTime)

/*  FUNCTION

    Unlock a RealTime.library internal semaphore.

    INPUTS

    lockHandle  --  Handle returned by LockRealTime(); may be NULL.

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    LockRealTime()

    INTERNALS

    HISTORY

    26.7.99  SDuvan implemented

******************************************************************************/

{
    AROS_LIBFUNC_INIT

    if (lockHandle == NULL)
    {
	return;
    }

    ReleaseSemaphore((struct SignalSemaphore *)lockHandle);

    AROS_LIBFUNC_EXIT
} /* UnlockRealTime */
