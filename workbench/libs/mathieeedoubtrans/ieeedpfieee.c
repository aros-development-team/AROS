/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "mathieeedoubtrans_intern.h"

/*****************************************************************************

    NAME */

        AROS_LH1(double, IEEEDPFieee,

/*  SYNOPSIS */
        AROS_LHA(LONG, y, D0),

/*  LOCATION */
        struct MathIeeeDoubTransBase *, MathIeeeDoubTransBase, 18, MathIeeeDoubTrans)

/*  FUNCTION
        Convert IEEE single to IEEE double precision

    INPUTS

    RESULT
        IEEE double precision floting point number

        Flags:
          zero     : result is zero
          negative : result is negative
          overflow : 0

    BUGS

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    LONG tmpL = y & IEEESPExponent_Mask; /* get the Exponent */
    QUAD Res, tmpQ;
    
    SetSR( 0, Zero_Bit | Overflow_Bit | Negative_Bit);
    
    if (0 == y)
    {
        SetSR( Zero_Bit, Zero_Bit | Overflow_Bit | Negative_Bit);
        Set_Value64C(Res, 0x0, 0x0);
        return Res;
    }
    
    tmpL >>= 23;
    tmpL = tmpL - 0x7e + 0x3fe;
    
    /* set the Exponent */
    SHL32(Res, tmpL, 52);
    
    /* set the Mantisse */
    tmpL = y & IEEESPMantisse_Mask;
    SHL32(tmpQ, tmpL, 29);
    OR64Q(Res, tmpQ);
    
    if (y < 0)
    {
        OR64QC(Res,  IEEEDPSign_Mask_Hi, IEEEDPSign_Mask_Lo);
        SetSR( Negative_Bit, Zero_Bit | Overflow_Bit | Negative_Bit);
    }
    
    return Res;

    AROS_LIBFUNC_EXIT
}
