/*
    Copyright (C) 1995-2017, The AROS Development Team. All rights reserved.
*/

#include "mathieeedoubbas_intern.h"

/*****************************************************************************

    NAME */

        AROS_LHQUAD1(double, IEEEDPAbs,

/*  SYNOPSIS */
        AROS_LHAQUAD(double, y, D0, D1),

/*  LOCATION */
        struct MathIeeeDoubBasBase *, MathIeeeDoubBasBase, 9, MathIeeeDoubBas)

/*  FUNCTION
        Calculate the absolute value of the given IEEE double precision
        floating point number.

    INPUTS
        y - IEEE double precision floating point number.

    RESULT
        x - absolute value of y.

        Flags:
          zero     : result is zero
          negative : 0
          overflow : 0

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        IEEEDPNeg()

    INTERNALS
        Algorithm:
        Set the sign-bit to zero.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    QUAD * Qy = (QUAD *)&y;

    /*if (0 == (*Qy)) */
    if (is_eqC((*Qy),0,0))
    {
        /* value is 0 -> set the Zero Flag */
        SetSR( Zero_Bit, Zero_Bit | Overflow_Bit | Negative_Bit);
    }
    else
    {
        /* set the sign-bit to zero */
        /* (*Qy) &= (IEEEDPMantisse_Mask | IEEEDPExponent_Mask) */
        AND64QC
        (
            (*Qy),
            (IEEEDPMantisse_Mask_Hi | IEEEDPExponent_Mask_Hi),
            (IEEEDPMantisse_Mask_Lo | IEEEDPExponent_Mask_Lo)
        );
        SetSR(0, Zero_Bit | Overflow_Bit | Negative_Bit );
    }
    
    return y;

    AROS_LIBFUNC_EXIT
}
