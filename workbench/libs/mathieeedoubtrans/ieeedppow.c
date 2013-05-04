/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "mathieeedoubtrans_intern.h"

/*****************************************************************************

    NAME */

        AROS_LHQUAD2(double, IEEEDPPow,

/*  SYNOPSIS */
        AROS_LHAQUAD(double, x, D2, D3),
        AROS_LHAQUAD(double, y, D0, D1),

/*  LOCATION */
        struct MathIeeeDoubTransBase *, MathIeeeDoubTransBase, 15, MathIeeeDoubTrans)

/*  FUNCTION
        Calculate y raised to the x power (y^x)

    INPUTS

    RESULT
        IEEE double precision floating point number

        flags:
        zero     : result is zero
        negative : result is negative
        overflow : result is too big

    BUGS

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    /* 
        a ^ b  = e^(b * ln a)
        y ^ x  = e^(x * ln y)
    */
    QUAD Res, tmp;
    
    /* y^x is illegal if y<0 and x is not an integer-value */
    if (is_lessSC(y, 0x0, 0x0) && is_neq(x, IEEEDPCeil(x)))
    {
        Set_Value64C(Res, 0x0, 0x0);
        return Res;
    }
    
    if (is_eqC(y, 0x0, 0x0))
    {
        Set_Value64C(Res, 0x3ff00000, 0x0);
        return Res;
    }
    Set_Value64(tmp, y);
    AND64QC
    (
        tmp, 
        (IEEEDPMantisse_Mask_Hi + IEEEDPExponent_Mask_Hi),
        (IEEEDPMantisse_Mask_Lo + IEEEDPExponent_Mask_Lo)
    );
    
    Res = IEEEDPLog(tmp);
    Res = IEEEDPMul(Res, x);
    Res = IEEEDPExp(Res);
    
    /* 
        if y < 0 and x was and even integer, the result is positive, otherwise
        it is negative.
    */
    if
    (
           is_lessSC(y, 0x0, 0x0) 
        && TRUE == intern_IEEEDPisodd(x)
    )
    {
        OR64QC(Res, IEEEDPSign_Mask_Hi, IEEEDPSign_Mask_Lo);
    }
    
    if (is_eqC(Res, 0x0, 0x0))
    {
        SetSR(Zero_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        Set_Value64C(Res, 0x0, 0x0);
        return Res;
    }
    
    SetSR(0, Zero_Bit | Negative_Bit | Overflow_Bit);
    
    if (is_lessSC(Res, 0x0, 0x0))
    {
        SetSR(Negative_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
    }
    
    Set_Value64(tmp, Res);
    AND64QC
    (
        tmp, 
        (IEEEDPMantisse_Mask_Hi + IEEEDPExponent_Mask_Hi),
        (IEEEDPMantisse_Mask_Lo + IEEEDPExponent_Mask_Lo)
    );
    
    if (is_eqC(Res, IEEEDPPInfty_Hi, IEEEDPPInfty_Lo))
    {
        /* don`t touch the Negative_Bit now!*/
        SetSR(Overflow_Bit, Zero_Bit | Overflow_Bit);
    }
    
    return Res;

    AROS_LIBFUNC_EXIT
}
