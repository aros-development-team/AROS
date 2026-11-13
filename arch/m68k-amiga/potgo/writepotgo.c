/*
    Copyright (C) 1995-2001, The AROS Development Team. All rights reserved.

    Desc: WritePotgo() function.
*/

#include <proto/potgo.h>
#include <hardware/custom.h>

#include "potgo_intern.h"

/*****************************************************************************

    NAME */
        AROS_LH2(void, WritePotgo,

/*  SYNOPSIS */
         AROS_LHA(UWORD, word, D0),
         AROS_LHA(UWORD, mask, D1),

/*  LOCATION */
         struct PotgoBase *, PotgoBase, 3, Potgo)
/*  FUNCTION
        Update the saved POTGO output value and write it to the Amiga
        custom chip. For output bits 8 through 15, mask selects the bits
        to replace and word supplies their new values. Unselected output
        bits retain the resource's saved values.

        Bit 0 of word is the START command. When set, it is included in
        the hardware write independently of mask and is not latched in
        the saved output value. Bits 1 through 7 of word are ignored.

    INPUTS
        word - New output values and optional START command.
        mask - Mask of output bits to update. Use only bits reserved by
               the caller with AllocPotBits().

    RESULT
        None.

    NOTES
        The resource does not check mask against its allocation bitmap
        or track task ownership. Setting START requires coordination with
        other users even when mask is zero. Direct writes to POTGO do not
        update the resource's saved output value and can be overwritten
        by subsequent resource calls.

        This describes the native Amiga implementation. No internal
        critical section protects the saved value or hardware write.
        Callers must serialize competing operations.

    SEE ALSO
        AllocPotBits(), FreePotBits()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

        volatile struct Custom *custom = (struct Custom*)0xdff000;

        word &= 0xff01;
        PotgoBase->data |= (word & ~1) & mask;
        PotgoBase->data &= word | (~mask);

        custom->potgo = PotgoBase->data | (word & 1);

    AROS_LIBFUNC_EXIT
}
