/*
    Copyright (C) 1995-2017, The AROS Development Team. All rights reserved.
*/

#include "mathieeedoubbas_intern.h"

/*****************************************************************************

    NAME */

        AROS_LHQUAD1(double, IEEEDPCeil,

/*  SYNOPSIS */
        AROS_LHAQUAD(double, y, D0, D1),

/*  LOCATION */
        struct MathIeeeDoubBasBase *, MathIeeeDoubBasBase, 16, MathIeeeDoubBas)

/*  FUNCTION
        Calculates the ceiling value of an IEEE double precision number.

    INPUTS
        y - IEEE double precision floating point number.

    RESULT
        x - ceiling of y.

        Flags:
          zero     : result is zero
          negative : result is negative
          overflow : 0

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        IEEEDPFloor(), IEEEDPFix()

    INTERNALS
        Algorithm:
        Ceil(y) = - Floor(-y)

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    QUAD * Qy = (QUAD *)&y;
    
    if (is_eqC((*Qy),0,0))
    {
        SetSR(Zero_Bit, Negative_Bit|Overflow_Bit|Zero_Bit);
        return y;
    }
    
    XOR64QC((*Qy), IEEEDPSign_Mask_Hi, IEEEDPSign_Mask_Lo);
    /* Ceil(y) = -Floor(-y); */
    y = IEEEDPFloor(y);
    if (is_eqC((*Qy), 0x0, 0x0))
    {
        Set_Value64C((*Qy), 0, 0);
        return y;
    }
    else
    {
        XOR64QC((*Qy), IEEEDPSign_Mask_Hi, IEEEDPSign_Mask_Lo );
        return y;
    }
  
    AROS_LIBFUNC_EXIT
}
