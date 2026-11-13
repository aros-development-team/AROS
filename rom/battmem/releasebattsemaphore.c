/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: ReleaseBattSemaphore() function.
*/
#include <proto/exec.h>

#include "battmem_intern.h"

/*****************************************************************************

    NAME */
#include <proto/battmem.h>

        AROS_LH0(void, ReleaseBattSemaphore,

/*  SYNOPSIS */
        /* void */

/*  LOCATION */
        struct BattMemBase *, BattMemBase, 2, Battmem)

/*  FUNCTION
        Give up the exclusive access to the battery backed up memory
        gained with ObtainBattSemaphore().

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        ObtainBattSemaphore()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    ReleaseSemaphore(&BattMemBase->bm_Semaphore);

    AROS_LIBFUNC_EXIT
} /* ReleaseBattSemaphore */
