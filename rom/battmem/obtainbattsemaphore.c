/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: ObtainBattSemaphore() function.
*/
#include <proto/exec.h>

#include "battmem_intern.h"

/*****************************************************************************

    NAME */
#include <proto/battmem.h>

        AROS_LH0(void, ObtainBattSemaphore,

/*  SYNOPSIS */
        /* void */

/*  LOCATION */
        struct BattMemBase *, BattMemBase, 1, Battmem)

/*  FUNCTION
        Gain exclusive access to the battery backed up memory. Reads and
        writes of related fields have to be bracketed by this and
        ReleaseBattSemaphore(), so that they are not interleaved with
        those of another task.

    INPUTS

    RESULT

    NOTES
        ReadBattMem() and WriteBattMem() are safe to call on their own;
        the semaphore is only needed to keep a group of them together.

    EXAMPLE

    BUGS

    SEE ALSO
        ReleaseBattSemaphore()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    ObtainSemaphore(&BattMemBase->bm_Semaphore);

    AROS_LIBFUNC_EXIT
} /* ObtainBattSemaphore */
