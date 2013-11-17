/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
 
    Locks the public classes list.
*/

#include <proto/exec.h>
#include "intuition_intern.h"

/*****************************************************************************

    NAME */
#include <proto/intuition.h>

        AROS_LH0(void, lockPubClass,

/*  SYNOPSIS */

/*  LOCATION */
        struct IntuitionBase *, IntuitionBase, 120, Intuition)

/*  FUNCTION
        Locks the public classes list.

    INPUTS
        None.

    RESULT
        None.

    NOTES

    EXAMPLE
 
    BUGS
 
    SEE ALSO
        UnlockPubClass()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    ObtainSemaphoreShared(&GetPrivIBase(IntuitionBase)->ClassListLock);

    AROS_LIBFUNC_EXIT
} /* unlockPubClass */
