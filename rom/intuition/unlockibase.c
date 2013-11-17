/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$

    Permits the access to all public fields of the IntuitionBase.
*/

#include <proto/exec.h>
#include "intuition_intern.h"

/*****************************************************************************

    NAME */
#include <proto/intuition.h>

        AROS_LH1(void, UnlockIBase,

/*  SYNOPSIS */
        AROS_LHA(ULONG, ibLock, A0),

/*  LOCATION */
        struct IntuitionBase *, IntuitionBase, 70, Intuition)

/*  FUNCTION
        Release parts of Intuition which have been blocked with a prior
        call to LockIBase().

    INPUTS
        ibLock - The result of LockIBase().

    RESULT
        None.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        LockIBase()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    ibLock = ibLock;        /* shut up the compiler */

    ReleaseSemaphore (GetPrivIBase(IntuitionBase)->IBaseLock);

    AROS_LIBFUNC_EXIT
} /* UnlockIBase */
