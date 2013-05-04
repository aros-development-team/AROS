/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "mathieeedoubtrans_intern.h"

/*****************************************************************************

    NAME */

        AROS_LHQUAD1(double, IEEEDPCosh,

/*  SYNOPSIS */
        AROS_LHAQUAD(double, y, D0, D1),

/*  LOCATION */
        struct MathIeeeDoubTransBase *, MathIeeeDoubTransBase, 11, MathIeeeDoubTrans)

/*  FUNCTION
        Calculate the hyperbolic cosine of the IEEE single precision number

    INPUTS

    RESULT
        IEEE single precision floating point number

        flags:
        zero     : result is zero
        negative : 0 (not possible)
        overflow : result too big for ffp-number

    BUGS

    INTERNALS
        cosh(x) = (1/2)*( e^x + e^(-x) )

        cosh( |x| >= 18 ) = (1/2) * (e^x);

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    QUAD Res;
    /* cosh(-x) = cosh(x) */
    /* y &= ( IEEEDPMantisse_Mask + IEEEDPExponent_Mask ); */
    AND64QC
    (
        y, 
        (IEEEDPMantisse_Mask_Hi + IEEEDPExponent_Mask_Hi),
        (IEEEDPMantisse_Mask_Lo + IEEEDPExponent_Mask_Lo)
    );
    
    if ( is_eqC(y, IEEEDPPInfty_Hi, IEEEDPPInfty_Lo))
    {
        SetSR(Overflow_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        return y;
    }
    
    Res = IEEEDPExp(y);
    
    if ( is_eqC(Res, IEEEDPPInfty_Hi, IEEEDPPInfty_Lo))
    {
        SetSR(Overflow_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        return  Res;
    }
    
    /* does adding of 1/(e^x) still change the result? */
    if ( is_lessC(y, 0x40320000, 0x0 ))
    {
        QUAD One;
        Set_Value64C(One, 0x3ff00000, 0x0);
        
        Res = IEEEDPAdd(Res, IEEEDPDiv(One, Res));
    }
    /* Res = Res / 2 */
    ADD64QC(Res, 0xFFF00000, 0x0);
    
    if
    (
           is_eqC(Res, 0x0, 0x0) 
        || is_lessSC(Res, 0x0, 0x0)
    )
    {
        SetSR(Zero_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        Set_Value64C(Res, 0x0, 0x0);
        return Res;
    }
    return Res;

    AROS_LIBFUNC_EXIT
}
