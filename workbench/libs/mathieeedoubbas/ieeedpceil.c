/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "mathieeedoubbas_intern.h"

/*
    FUNCTION
        Calculates the ceil-value of a IEEE double precision number

    RESULT
        Flags:
          zero     : result is zero
          negative : result is negative
          overflow : 0

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        IEEEDPFloor()

    INTERNALS
      ALGORITHM:
         Ceil(y) = - Floor(-y)

    HISTORY
*/

AROS_LHQUAD1(double, IEEEDPCeil,
    AROS_LHAQUAD(double, y, D0, D1),
    struct MathIeeeDoubBasBase *, MathIeeeDoubBasBase, 16, MathIeeeDoubBas
)
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
