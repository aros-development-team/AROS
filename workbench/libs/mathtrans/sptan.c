/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "mathtrans_intern.h"

/*
    FUNCTION
      Calculate the tangens of a given FFP number in radians

    RESULT
      Motorola fast floating point number

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

AROS_LH1(float, SPTan,
    AROS_LHA(float, fnum1, D0),
    struct Library *, MathTransBase, 8, MathTrans
)
{
    AROS_LIBFUNC_INIT
    
    LONG z,SIN,Res,ysquared,yabs,tmp;
    yabs = fnum1 & (FFPMantisse_Mask + FFPExponent_Mask);
    
    if ((LONG)FFP_Pinfty == yabs)
    {
        SetSR(Overflow_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        return FFP_NAN;
    }
    
    z = SPFloor(SPDiv(pi, yabs));
    tmp  = SPMul(z,pi);
    tmp |= FFPSign_Mask; /* tmp = -tmp; */
    yabs = SPAdd(yabs, tmp);
    
    if ( (char)yabs > (char)pio2  &&  (yabs & FFPMantisse_Mask) > (pio2 & FFPMantisse_Mask) )
    {
        yabs |= FFPSign_Mask;
        yabs  =SPAdd(pi, yabs);
        tmp = TRUE;
    }
    else
    {
        tmp = FALSE;
    }
    
    ysquared = SPMul(yabs,yabs);
               SIN = SPMul(yabs,
               SPAdd(sinf1,
               SPMul(ysquared,
               SPAdd(sinf2,
               SPMul(ysquared,
               SPAdd(sinf3,
               SPMul(ysquared,
               SPAdd(sinf4,
               SPMul(ysquared,
               SPAdd(sinf5,
               SPMul(ysquared, sinf6)))))))))));
    /* cos  */
    z   = SPAdd(cosf1,
          SPMul(ysquared,
          SPAdd(cosf2,
          SPMul(ysquared,
          SPAdd(cosf3,
          SPMul(ysquared,
          SPAdd(cosf4,
          SPMul(ysquared,
          SPAdd(cosf5,
          SPMul(ysquared, cosf6))))))))));
    
    Res = SPDiv(z, SIN);
    
    if ((char)fnum1 < 0 ) Res ^= FFPSign_Mask;
    if (TRUE == tmp)      Res ^= FFPSign_Mask;
    
    if (0 == Res)
    {
        SetSR(Zero_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        return 0;
    }
    
    if ((char)Res < 0)
    {
        SetSR(Negative_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
    }
    
    return Res;

    AROS_LIBFUNC_EXIT
}
