/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "mathieeesingtrans_intern.h"

/*
    FUNCTION
      Calculate the tangens of a given IEEE single precision number in radians

    RESULT
      IEEE single precision floating point number

      flags:
        zero     : result is zero
        negative : result is negative
        overflow :

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
*/

AROS_LH1(float, IEEESPTan,
    AROS_LHA(float, y, D0),
    struct Library *, MathIeeeSingTransBase, 8, MathIeeeSingTrans
)
{
    AROS_LIBFUNC_INIT
    
    LONG z,SIN,Res,ysquared,yabs,tmp;
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
        tmp = TRUE;
    }
    else
    {
        tmp = FALSE;
    }
    
    ysquared = IEEESPMul(yabs,yabs);
    SIN = IEEESPMul(yabs,
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
    /* cos: */
    z   = IEEESPAdd(cosf1,
          IEEESPMul(ysquared,
          IEEESPAdd(cosf2,
          IEEESPMul(ysquared,
          IEEESPAdd(cosf3,
          IEEESPMul(ysquared,
          IEEESPAdd(cosf4,
          IEEESPMul(ysquared,
          IEEESPAdd(cosf5,
          IEEESPMul(ysquared, cosf6))))))))));
    
    Res = IEEESPDiv(SIN, z);
    
    if (y < 0 )      Res ^= IEEESPSign_Mask;
    if (TRUE == tmp) Res ^= IEEESPSign_Mask;
    
    if (0 == Res)
    {
        SetSR(Zero_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        return 0;
    }
    
    if (Res < 0) SetSR(Negative_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
    
    return Res;

    AROS_LIBFUNC_EXIT
}
