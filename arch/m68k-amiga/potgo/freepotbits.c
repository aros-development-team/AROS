/*
    Copyright (C) 1995-2001, The AROS Development Team. All rights reserved.

    Desc: FreePotBits() function.
*/

#include <proto/potgo.h>
#include <hardware/custom.h>

#include "potgo_intern.h"

/*****************************************************************************

    NAME */
        AROS_LH1(void, FreePotBits,

/*  SYNOPSIS */
         AROS_LHA(UWORD, allocated, D0),

/*  LOCATION */
         struct PotgoBase *, PotgoBase, 2, Potgo)
/*  FUNCTION
        Release the specified POTGO bits. The bits are cleared in both
        the allocation bitmap and the resource's saved output value, and
        the updated output value is immediately written to POTGO.

    INPUTS
        allocated - Mask of bits previously obtained by AllocPotBits()
                    and still owned by the caller.

    RESULT
        None.

    NOTES
        Releasing bits changes their hardware output state; this is not
        merely an allocation operation. The resource does not track task
        ownership or reject bits allocated by another caller.

        This describes the native Amiga implementation. No internal
        critical section protects the bitmap, saved output value or
        hardware write. Callers must serialize competing operations; the
        function itself does not wait.

    SEE ALSO
        AllocPotBits(), WritePotgo()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

        volatile struct Custom *custom = (struct Custom*)0xdff000;

        PotgoBase->allocated &= ~allocated;
        PotgoBase->data &= ~allocated;
        custom->potgo = PotgoBase->data;

    AROS_LIBFUNC_EXIT
}
