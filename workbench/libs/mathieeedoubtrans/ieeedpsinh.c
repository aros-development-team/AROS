/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "mathieeedoubtrans_intern.h"

/*****************************************************************************

    NAME */

        AROS_LHQUAD1(double, IEEEDPSinh,

/*  SYNOPSIS */
        AROS_LHAQUAD(double, y, D0, D1),

/*  LOCATION */
        struct MathIeeeDoubTransBase *, MathIeeeDoubTransBase, 10, MathIeeeDoubTrans)

/*  FUNCTION
        Calculate the hyperbolic sine of the IEEE double precision number

    INPUTS

    RESULT
        IEEE double precision floating point number

        flags:
        zero     : result is zero
        negative : result is negative
        overflow : result is too big for IEEE double precsion format

    BUGS

    INTERNALS
        sinh(x) = (1/2)*( e^x- e^(-x) )

        sinh( |x| >=  18 ) = (1/2) * (e^x);

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    QUAD Res;
    QUAD y2;
    
    /* y2 = y & (IEEEDPMantisse_Mask + IEEEDPExponent_Mask); */
    Set_Value64(y2, y);
    AND64QC
    (
        y2, 
        (IEEEDPMantisse_Mask_Hi + IEEEDPExponent_Mask_Hi),
        (IEEEDPMantisse_Mask_Lo + IEEEDPExponent_Mask_Lo)
    );
    
    if ( is_eqC(y, IEEEDPPInfty_Hi, IEEEDPPInfty_Lo))
    {
        SetSR(Overflow_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        return y;
    }
    
    /* sinh(-x) = -sinh(x) */
    Res = IEEEDPExp(y2);
    
    if ( is_eqC(Res, IEEEDPPInfty_Hi, IEEEDPPInfty_Lo ))
    {
        SetSR(Overflow_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        if ( is_lessSC(y, 0x0, 0x0))
        {
            OR64QC(Res, IEEEDPSign_Mask_Hi, IEEEDPSign_Mask_Lo );
        }
        return Res;
    }
    
    if ( is_lessC(y2, 0x40320000, 0x0) )
    {
        QUAD One, ResTmp;
        Set_Value64C(One, one_Hi, one_Lo);
        ResTmp = IEEEDPDiv(One, Res);
        OR64QC(ResTmp, IEEEDPSign_Mask_Hi, IEEEDPSign_Mask_Lo);
        
        Res = IEEEDPAdd(Res, ResTmp);
    }
    /* Res = Res / 2 */
    ADD64QC(Res, 0xFFF00000, 0x0);
    
    /* at this point Res has to be positive to be valid */
    if ( is_leqSC(Res, 0x0, 0x0))
    {
        SetSR(Zero_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        AND64QC(y, IEEEDPSign_Mask_Hi, IEEEDPSign_Mask_Lo);
        return y;
    }
    
    if ( is_lessSC(y, 0x0, 0x0))
    {
        SetSR(Negative_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        OR64QC(Res, IEEEDPSign_Mask_Hi, IEEEDPSign_Mask_Lo );
        
        return Res;
    }
    
    return Res;

    AROS_LIBFUNC_EXIT
}
