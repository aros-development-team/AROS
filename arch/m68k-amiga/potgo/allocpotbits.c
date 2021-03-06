/*
    Copyright (C) 1995-2001, The AROS Development Team. All rights reserved.

    Desc: AllocPotBits() function.
*/

#include <proto/potgo.h>

#include "potgo_intern.h"

AROS_LH1(UWORD, AllocPotBits,
         AROS_LHA(UWORD, bits, D0),
         struct PotgoBase *, PotgoBase, 1, Potgo)
{
    AROS_LIBFUNC_INIT

        bits &= ~PotgoBase->allocated;
        PotgoBase->allocated |= bits;
        // TODO! check START special cases
    return bits;

    AROS_LIBFUNC_EXIT
}
