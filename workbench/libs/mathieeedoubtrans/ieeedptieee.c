/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "mathieeedoubtrans_intern.h"

/*****************************************************************************

    NAME */

        AROS_LHQUAD1(LONG, IEEEDPTieee,

/*  SYNOPSIS */
        AROS_LHAQUAD(double, y, D0, D1),

/*  LOCATION */
        struct MathIeeeDoubTransBase *, MathIeeeDoubTransBase, 17, MathIeeeDoubTrans)

/*  FUNCTION
        Convert IEEE double to IEEE single precision number

    INPUTS

    RESULT
        IEEE single precision number

        Flags:
          zero     : result is zero
          negative : result is negative
          overflow : value was out of range for IEEE single precision

    BUGS

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    LONG Res, tmp;
    
    SetSR(0, Zero_Bit | Overflow_Bit | Negative_Bit );
    
    if (is_eqC(y, 0x0, 0x0))
    {
        SetSR(Zero_Bit, Zero_Bit | Overflow_Bit | Negative_Bit );
        return 0;
    }
    
    SHRU32(Res, y, 52 );
    SHRU32(tmp, y, 29 );
    /* calculate the exponent */
    Res &=0x7ff;
    Res = Res + 0x7e - 0x3fe;
    Res <<= 23;
    
    Res |= (tmp & IEEESPMantisse_Mask);
    
    if (is_lessSC(y, 0x0, 0x0))
    {
        SetSR(Negative_Bit, Zero_Bit | Overflow_Bit | Negative_Bit );
        Res |= IEEESPSign_Mask;
    }
    
    return Res;
    
    AROS_LIBFUNC_EXIT
}
