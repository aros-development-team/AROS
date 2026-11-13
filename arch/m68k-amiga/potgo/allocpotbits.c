/*
    Copyright (C) 1995-2001, The AROS Development Team. All rights reserved.

    Desc: AllocPotBits() function.
*/

#include <proto/potgo.h>

#include "potgo_intern.h"

/*****************************************************************************

    NAME */
        AROS_LH1(UWORD, AllocPotBits,

/*  SYNOPSIS */
         AROS_LHA(UWORD, bits, D0),

/*  LOCATION */
         struct PotgoBase *, PotgoBase, 1, Potgo)
/*  FUNCTION
        Reserve the currently unallocated bits from a requested POTGO bit
        mask. Partial allocation is possible: bits already reserved by
        other callers are excluded from the returned mask.

    INPUTS
        bits - Mask of POTGO bits to reserve.

    RESULT
        The mask of bits actually reserved, which may be a proper subset
        of bits or zero. If the caller requires the entire mask, compare
        the result with bits and release any partial allocation with
        FreePotBits().

    NOTES
        Allocation tracks bits, not task ownership. Release the returned
        mask with FreePotBits() when finished. Allocation does not write
        the hardware register.

        This describes the native Amiga implementation. The allocation
        bitmap is updated without an internal critical section. Callers
        must serialize competing allocation and release operations; the
        function itself does not wait.

    BUGS
        The implementation does not implement special allocation rules
        for the START bit; it treats it like any other requested bit.

    SEE ALSO
        FreePotBits(), WritePotgo()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

        bits &= ~PotgoBase->allocated;
        PotgoBase->allocated |= bits;
        // TODO! check START special cases
    return bits;

    AROS_LIBFUNC_EXIT
}
