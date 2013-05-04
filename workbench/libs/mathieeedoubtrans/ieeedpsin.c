/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "mathieeedoubtrans_intern.h"

/*****************************************************************************

    NAME */

        AROS_LHQUAD1(double, IEEEDPSin,

/*  SYNOPSIS */
        AROS_LHAQUAD(double, y, D0, D1),

/*  LOCATION */
        struct MathIeeeDoubTransBase *, MathIeeeDoubTransBase, 6, MathIeeeDoubTrans)

/*  FUNCTION
        Calculate the sine of a given IEEE double precision number in radians

    INPUTS

    RESULT
        IEEE double precision floating point number

        flags:
        zero     : result is zero
        negative : result is negative
        overflow : 0

    BUGS

    INTERNALS
        Algorithm for Calculation of sin(y):
        z    = floor ( |y| / pi );
        y_1  = |y| - z * pi;        => 0 <= y_1 < pi

        if (y_1 > pi/2 ) then y_1 = pi - y_1;

        => 0 <= y_1 < pi/2

        Res = y - y^3/3! + y^5/5! - y^7/7! + y^9/9! - y^11/11! =
            = y(1+y^2(-1/3!+y^2(1/5!+y^2(-1/7!+y^2(1/9!-1/11!y^2)))));

        if (y < 0)
            Res = -Res;

        if (z was an odd number)
            Res = -Res;

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
        
    QUAD z,Res,ysquared,yabs, Qtmp;
    AND64C
    (
        yabs, y, 
        (IEEEDPMantisse_Mask_Hi | IEEEDPExponent_Mask_Hi),
        (IEEEDPMantisse_Mask_Lo | IEEEDPExponent_Mask_Lo)
    );
    
    if (is_eqC(yabs, IEEEDPPInfty_Hi, IEEEDPPInfty_Lo))
    {
        SetSR(Overflow_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        Set_Value64C(Res, IEEEDPNAN_Hi, IEEEDPNAN_Lo);
        return Res;
    }
    
    z = IEEEDPFloor(IEEEDPDiv(yabs, pi));
    Qtmp  = IEEEDPMul(z,pi);
    OR64QC(Qtmp, IEEEDPSign_Mask_Hi, IEEEDPSign_Mask_Lo) /* Qtmp=-Qtmp */
    yabs = IEEEDPAdd(yabs, Qtmp);
    if (is_greaterC(yabs, pio2_Hi, pio2_Lo))
    {
        OR64QC(Qtmp, IEEEDPSign_Mask_Hi, IEEEDPSign_Mask_Lo) /* Qtmp=-Qtmp */
        yabs  =IEEEDPAdd(pi, yabs);
    }
    ysquared = IEEEDPMul(yabs,yabs);
    Res = IEEEDPMul(yabs,
          IEEEDPAdd(sinf1,
          IEEEDPMul(ysquared,
          IEEEDPAdd(sinf2,
          IEEEDPMul(ysquared,
          IEEEDPAdd(sinf3,
          IEEEDPMul(ysquared,
          IEEEDPAdd(sinf4,
          IEEEDPMul(ysquared,
          IEEEDPAdd(sinf5,
          IEEEDPMul(ysquared, 
          IEEEDPAdd(sinf6,
          IEEEDPMul(ysquared,
          IEEEDPAdd(sinf7,
          IEEEDPMul(ysquared, sinf8)))))))))))))));
    
    if (is_lessSC(y, 0x0, 0x0) )
    {
        XOR64QC(Res, IEEEDPSign_Mask_Hi, IEEEDPSign_Mask_Lo);
    }
    
    if (TRUE == intern_IEEEDPisodd(z))
    {
        XOR64QC(Res, IEEEDPSign_Mask_Hi, IEEEDPSign_Mask_Lo);
    }
    
    if (is_eqC(Res, 0x0, 0x0))
    {
        SetSR(Zero_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        return Res;
    }
    
    if (is_lessSC(Res, 0x0 ,0x0))
    {
        SetSR(Negative_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
    }
    
    return Res;

    AROS_LIBFUNC_EXIT
}
