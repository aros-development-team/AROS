/*
    Copyright � 1995-2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "mathffp_intern.h"

/*****************************************************************************

    NAME */

        AROS_LH1(float, SPNeg,

/*  SYNOPSIS */
        AROS_LHA(float, fnum1, D0),

/*  LOCATION */
        struct LibHeader *, MathBase, 10, Mathffp)

/*  FUNCTION
        Calculate fnum1*(-1)

    INPUTS

    RESULT
        -fnum1

        Flags:
          zero     : result is zero
          negative : result is negative
          overflow : 0

    BUGS

    INTERNALS
        ALGORITHM:
        Return zero if fnum == 0.
        Otherwise flip the sign-bit.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    D(kprintf("SPNeg(%08x)\n", fnum1));
    if (0 == fnum1)
    {
        SetSR( Zero_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        return 0;
    }
    
    /* flip sign-bit */
    fnum1 ^= FFPSign_Mask;
    
    if((char) fnum1 < 0)
    {
        /* result is negative */
        SetSR(Negative_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
    }
    else
    {
        /* result is positive */
        SetSR(0, Zero_Bit | Overflow_Bit | Negative_Bit );
    }
    
    return fnum1;

    AROS_LIBFUNC_EXIT
}
