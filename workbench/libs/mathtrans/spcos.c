/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "mathtrans_intern.h"

/*
    FUNCTION
      Calculate the cosine of a given ffp number in radians

    RESULT
      Motorola fast floating point number

      flags:
        zero     : result is zero
        negative : result is negative
        overflow : 0

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS
      Algorithm for Calculation of cos(y):
     <code>
         z    = floor ( |y| / pi );
         y_1  = |y| - z * pi;        => 0 <= y_1 < pi

         if (y_1 > pi/2 ) then y_1 = pi - y_1;

         => 0 <= y_1 < pi/2

         Res = 1 - y^2/2! + y^4/4! - y^6/6! + y^8/8! - y^10/10! =
             = 1 -(y^2(-1/2!+y^2(1/4!+y^2(-1/6!+y^2(1/8!-1/10!y^2)))));

         if (z was an odd number)
           Res = -Res;

         if (y_1 was greater than pi/2 in the test above)
           Res = -Res;
     </code>


    HISTORY
*/

AROS_LH1(float, SPCos,
    AROS_LHA(float, fnum1, D0),
    struct Library *, MathTransBase, 7, MathTrans
)
{
    AROS_LIBFUNC_INIT
    
    LONG z,Res,ysquared,yabs,tmp;
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
    
    if 
    ( 
            (char)yabs > (char)pio2  
        &&  (yabs & FFPMantisse_Mask) > (pio2 & FFPMantisse_Mask) 
    )
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
    Res = SPAdd(cosf1,
          SPMul(ysquared,
          SPAdd(cosf2,
          SPMul(ysquared,
          SPAdd(cosf3,
          SPMul(ysquared,
          SPAdd(cosf4,
          SPMul(ysquared,
          SPAdd(cosf5,
          SPMul(ysquared, cosf6))))))))));
    
    if (0 == Res)
    {
        SetSR(Zero_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        return 0;
    }
    
    if (TRUE == intern_SPisodd(z)) Res ^= FFPSign_Mask;
    
    if (TRUE == tmp) Res ^= FFPSign_Mask;
    
    if ((char)Res < 0) SetSR(Negative_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
    
    return Res;

    AROS_LIBFUNC_EXIT
}
