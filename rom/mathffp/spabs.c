/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "mathffp_intern.h"

/*
    FUNCTION
        Calculate the absolute value of a given floating point number

    RESULT
        absolute value of fnum1

        Flags:
          zero     : result is zero
          negative : result is negative
          overflow : 0

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS
       Set the sign-bit to zero

    HISTORY
*/

AROS_LH1(float, SPAbs,
    AROS_LHA(float, fnum1, D0),
    struct LibHeader *, MathBase, 9, Mathffp
)
{
    AROS_LIBFUNC_INIT
    
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
