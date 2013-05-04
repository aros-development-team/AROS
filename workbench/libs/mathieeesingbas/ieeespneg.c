/*
    Copyright � 1995-2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "mathieeesingbas_intern.h"

/*****************************************************************************

    NAME */

        AROS_LH1(float, IEEESPNeg,

/*  SYNOPSIS */
        AROS_LHA(float, y, D0),

/*  LOCATION */
        struct LibHeader *, MathIeeeSingBasBase, 10, Mathieeesingbas)

/*  FUNCTION
        Switch the sign of the given ieeesp number

    INPUTS

    RESULT
        -y

        Flags:
          zero     : result is zero
          negative : result is negative
          overflow : 0

    BUGS

    INTERNALS
        ALGORITHM:
        Return -0 if y == 0.
        Otherwise flip the sign-bit.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    if (0 == y || 0x80000000 == y)
    {
        SetSR( Zero_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        return (IEEESPSign_Mask ^ y);
    }
    
    /* flip sign-bit */
    y ^= IEEESPSign_Mask;
    
    if(y < 0)
    {
        /* result is negative */
        SetSR(Negative_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
    }
    else
    {
        /* result is positive */
        SetSR(0, Zero_Bit | Overflow_Bit | Negative_Bit );
    }
    
    return y;

    AROS_LIBFUNC_EXIT
}
