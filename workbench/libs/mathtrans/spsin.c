/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang: english
*/


#include <libraries/mathffp.h>
#include <aros/libcall.h>
#include <proto/mathffp.h>
#include <proto/mathtrans.h>
#include <proto/exec.h>
#include <exec/types.h>
#include "mathtrans_intern.h"


/*****************************************************************************

    NAME */

      AROS_LH1(LONG, SPSin,

/*  SYNOPSIS */

      AROS_LHA(LONG, fnum1, D0),

/*  LOCATION */

      struct Library *, MathTransBase, 6, Mathtrans)

/*  FUNCTION

      Calculate the sine of a given FFP number in radians

    INPUTS

      y - Motorola fast floating point number

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


    HISTORY

******************************************************************************/

{
  LONG z,Res,ysquared,yabs,tmp;
  yabs = fnum1 & (FFPMantisse_Mask + FFPExponent_Mask);

  if (FFP_Pinfty == yabs)
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
    yabs  = SPAdd(pi, yabs);
  }
  ysquared = SPMul(yabs,yabs);
  Res = SPMul(yabs,
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

  if ((char)fnum1 < 0 )
    Res ^= FFPSign_Mask;

  if (TRUE == intern_SPisodd(z))
    Res ^= FFPSign_Mask;

  if (0 == Res)
  {
    SetSR(Zero_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
    return 0;
  }

  if ((char)Res < 0)
    SetSR(Negative_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);

  return Res;

} /* SPSin */
