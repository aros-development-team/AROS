/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "mathieeesingtrans_intern.h"

/*
    FUNCTION
      Calculate the sine of a given IEEE single precision number in radians

    RESULT
      IEEE single precision floating point number

      flags:
        zero     : result is zero
        negative : result is negative
        overflow : 0

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS
      Algorithm for Calculation of sin(y):
      <code>
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
      </code>

    HISTORY
*/

AROS_LH1(float, IEEESPSin,
    AROS_LHA(float, y, D0),
    struct Library *, MathIeeeSingTransBase, 6, MathIeeeSingTrans
)
{
    AROS_LIBFUNC_INIT
    
    LONG z,Res,ysquared,yabs,tmp;
    yabs = y & (IEEESPMantisse_Mask + IEEESPExponent_Mask);
    
    if (IEEESP_Pinfty == yabs)
    {
        SetSR(Overflow_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        return IEEESP_NAN;
    }
    
    z = IEEESPFloor(IEEESPDiv(yabs, pi));
    tmp  = IEEESPMul(z,pi);
    tmp |= IEEESPSign_Mask; /* tmp = -tmp; */
    yabs = IEEESPAdd(yabs, tmp);
    if (yabs > pio2)
    {
        yabs |= IEEESPSign_Mask;
        yabs  =IEEESPAdd(pi, yabs);
    }
    ysquared = IEEESPMul(yabs,yabs);
    Res = IEEESPMul(yabs,
          IEEESPAdd(sinf1,
          IEEESPMul(ysquared,
          IEEESPAdd(sinf2,
          IEEESPMul(ysquared,
          IEEESPAdd(sinf3,
          IEEESPMul(ysquared,
          IEEESPAdd(sinf4,
          IEEESPMul(ysquared,
          IEEESPAdd(sinf5,
          IEEESPMul(ysquared, sinf6)))))))))));
    
    if (y < 0 )                        Res ^= IEEESPSign_Mask;
    if (TRUE == intern_IEEESPisodd(z)) Res ^= IEEESPSign_Mask;
    
    if (0 == Res)
    {
        SetSR(Zero_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        return 0;
    }
    
    if (Res < 0) SetSR(Negative_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
    
    return Res;

    AROS_LIBFUNC_EXIT
}
