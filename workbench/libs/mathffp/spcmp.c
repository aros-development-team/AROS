/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "mathffp_intern.h"

/*****************************************************************************

    NAME */

        AROS_LH2(LONG, SPCmp,

/*  SYNOPSIS */
        AROS_LHA(float, fnum1, D1),
        AROS_LHA(float, fnum2, D0),

/*  LOCATION */
        struct LibHeader *, MathBase, 7, Mathffp)

/*  FUNCTION
        Compares two FFP numbers.

    INPUTS

    RESULT
       +1 : fnum1 > fnum2
        0 : fnum1 = fnum2
       -1 : fnum1 < fnum2


        Flags:
          zero     : fnum2 = fnum1
          negative : fnum2 < fnum1
          overflow : 0

    BUGS

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    LONG result;
    ULONG i1 = fnum1, i2 = fnum2, cc;

    D(kprintf("SPCmp(%08x,%08x)=", fnum1, fnum2));

    /* Convert numbers into a format that can be compared as if integers */
    fnum1 = i1 << 24 | i1 >> 8;
    if (fnum1 < 0)
        fnum1 ^= 0x7fffffff;
    fnum2 = i2 << 24 | i2 >> 8;
    if (fnum2 < 0)
        fnum2 ^= 0x7fffffff;

    if (fnum1 > fnum2)
    {
        result = 1;
        cc = 0;
    }
    else if (fnum1 == fnum2)
    {
        result = 0;
        cc = Zero_Bit;
    }
    else
    {
        result = -1;
        cc = Negative_Bit;
    }

    D(kprintf("%ld\n", result));
    SetSR(cc, Zero_Bit | Negative_Bit | Overflow_Bit);
    return result;

    AROS_LIBFUNC_EXIT
}
