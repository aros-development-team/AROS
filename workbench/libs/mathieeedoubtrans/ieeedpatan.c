/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "mathieeedoubtrans_intern.h"

/*****************************************************************************

    NAME */

        AROS_LHQUAD1(double, IEEEDPAtan,

/*  SYNOPSIS */
        AROS_LHAQUAD(double, y, D0, D1),

/*  LOCATION */
        struct MathIeeeDoubTransBase *, MathIeeeDoubTransBase, 5, MathIeeeDoubTrans)

/*  FUNCTION
        Calculates the angle of a given number representing the tangent
        of that angle. The angle will be in radians.

    INPUTS

    RESULT
        IEEE double precision floating point number

    BUGS

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    QUAD yabs;
    QUAD ysquared, ycubed, ypow5, one;
    
    AND64C
    (
        yabs, y, 
        (IEEEDPMantisse_Mask_Hi | IEEEDPExponent_Mask_Hi), 
        (IEEEDPMantisse_Mask_Lo | IEEEDPExponent_Mask_Lo)
    );
    
    /* check for +- infinity -> output: +-pi/2 */
    if (is_eqC(yabs, IEEEDPPInfty_Hi, IEEEDPPInfty_Lo))
    {
        AND64QC(y, IEEEDPSign_Mask_Hi, IEEEDPSign_Mask_Lo);
        OR64QC(y, pio2_hi_Hi, pio2_hi_Lo);
        return y;
    }
    /* atan(0) = 0 */
    if (is_eqC(yabs, 0, 0) /* 0 == yabs */ )
    {
        SetSR(Zero_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        return y;
    }
    
    ysquared = IEEEDPMul(yabs, yabs);
    ycubed = IEEEDPMul(yabs, ysquared);
    
    Set_Value64C(one, one_Hi, one_Lo);
    
    /* atan(x >= 860) = pi/2 - 1/x + 1/(3*x^3) */
    if (Get_High32of64(yabs)  >= 0x408ae000)
    {
        QUAD tmp1, onethird;
        Set_Value64C(tmp1, pio2_hi_Hi, pio2_hi_Lo);
        Set_Value64C(onethird, onethird_Hi, onethird_Lo);
        
        tmp1 = IEEEDPAdd(tmp1,IEEEDPDiv(IEEEDPSub(onethird, ysquared),ycubed)); 
        
        if (is_eq(yabs,y)) /* arg has positive sign */
        {
            SetSR(0, Zero_Bit | Negative_Bit | Overflow_Bit);
            return tmp1;
        }
        else
        {
            SetSR(Negative_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
            OR64QC(tmp1, IEEEDPSign_Mask_Hi, IEEEDPSign_Mask_Lo);
            return tmp1;
        }
    }
    
    /* 
          atan(x >= 128) = pi/2 - 1/x + 1/(3*x^3) -1/(5*x^5) 
        = pi/2 + (1/3*x^2 - x^4 - 1/5) / x^5
    */
    if (Get_High32of64(yabs) >= 0x40600000)
    {
        QUAD tmp1, onethird, onefifth;
        Set_Value64C(tmp1, pio2_hi_Hi, pio2_hi_Lo);
        Set_Value64C(onethird, onethird_Hi, onethird_Lo);
        Set_Value64C(onefifth, onefifth_Hi, onefifth_Lo);
        ypow5 = IEEEDPMul(ycubed, ysquared);
        
        tmp1 = IEEEDPAdd
        (
            tmp1, 
            IEEEDPDiv( 
            IEEEDPSub(
            IEEEDPSub(
            IEEEDPMul(onethird,ysquared),
            IEEEDPMul(ysquared,ysquared)),onefifth),ypow5 )
        );
        
        if (is_eq(yabs,y)) /* arg has positive sign */
        {
            SetSR(0, Zero_Bit | Negative_Bit | Overflow_Bit);
            return tmp1;     
        }
        else
        {
            SetSR(Negative_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
            OR64QC(tmp1, IEEEDPSign_Mask_Hi, IEEEDPSign_Mask_Lo);
            return tmp1;
        }
    }
    
    /* atan(x) = asin(x/sqrt(1+x^2)) */
    
    return IEEEDPAsin(IEEEDPDiv(y,IEEEDPSqrt(IEEEDPAdd(one,ysquared))));

    AROS_LIBFUNC_EXIT
}
