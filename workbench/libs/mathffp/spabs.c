/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "mathffp_intern.h"

/*****************************************************************************

    NAME */

        AROS_LH1(float, SPAbs,

/*  SYNOPSIS */
        AROS_LHA(float, fnum1, D0),

/*  LOCATION */
        struct LibHeader *, MathBase, 9, Mathffp)

/*  FUNCTION
        Calculate the absolute value of a given floating point number

    INPUTS

    RESULT
        absolute value of fnum1

        Flags:
          zero     : result is zero
          negative : result is negative
          overflow : 0
    BUGS

    INTERNALS
        Set the sign-bit to zero

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    D(kprintf("SPAbs(%08x)\n", fnum1));
    if (0 == fnum1)
    {
        /* value is 0 -> set the Zero Flag */
        SetSR( Zero_Bit, Zero_Bit | Overflow_Bit | Negative_Bit);
    }
    else
    {
        fnum1 &= (FFPMantisse_Mask | FFPExponent_Mask);
        /* set the sign-bit to zero */
        SetSR(0, Zero_Bit | Overflow_Bit | Negative_Bit );
    }
    return fnum1;

    AROS_LIBFUNC_EXIT
}
