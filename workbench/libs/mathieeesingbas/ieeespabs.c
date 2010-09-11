/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "mathieeesingbas_intern.h"

/*
    FUNCTION
        Calculate the absolute value of a given floating point number

    RESULT
        absolute value of y

        Flags:
          zero     : result is zero
          negative : result is negative
          overflow : 0

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO


    INTERNALS
      ALGORITHM:
        set the sign-bit to zero

    HISTORY
*/

AROS_LH1(float, IEEESPAbs,
    AROS_LHA(float, y, D0),
    struct LibHeader *, MathIeeeSingBasBase, 9, Mathieeesingbas
)
{
    AROS_LIBFUNC_INIT
    
    if (0 == y)
    {
        /* value is 0 -> set the Zero Flag */
        SetSR( Zero_Bit, Zero_Bit | Overflow_Bit | Negative_Bit);
    }
    else
    {
        /* set the sign-bit to zero */
        y &= (IEEESPMantisse_Mask | IEEESPExponent_Mask);
        SetSR(0, Zero_Bit | Overflow_Bit | Negative_Bit );
    }
    
    return y;
    
    AROS_LIBFUNC_EXIT
}
