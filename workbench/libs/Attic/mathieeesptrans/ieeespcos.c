/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang: english
*/


#include <libraries/mathieeesp.h>
#include <aros/libcall.h>
#include <proto/mathieeesingbas.h>
#include <proto/mathieeesingtrans.h>
#include <proto/exec.h>
#include <exec/types.h>
#include "mathieeesingtrans_intern.h"

/*****************************************************************************

    NAME */

      AROS_LH1(LONG, IEEESPCos,

/*  SYNOPSIS */

      AROS_LHA(LONG, y, D0),

/*  LOCATION */

      struct Library *, MathIeeeSingTransBase, 7, Mathieeesingtrans)

/*  FUNCTION

      Calculate the cosine of a given IEEE single precision number in radians

    INPUTS

      y - IEEE single precision floating point number

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
      Algorithm for Calculation of cos(y):
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


    HISTORY

******************************************************************************/

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
    tmp = TRUE;
  }
  else
    tmp = FALSE;

  ysquared = IEEESPMul(yabs,yabs);
  Res = IEEESPAdd(cosf1,
        IEEESPMul(ysquared,
        IEEESPAdd(cosf2,
        IEEESPMul(ysquared,
        IEEESPAdd(cosf3,
        IEEESPMul(ysquared,
        IEEESPAdd(cosf4,
        IEEESPMul(ysquared,
        IEEESPAdd(cosf5,
        IEEESPMul(ysquared, cosf6))))))))));

  if (0 == Res)
  {
    SetSR(Zero_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
    return 0;
  }

  if (TRUE == intern_IEEESPisodd(z))
    Res ^= IEEESPSign_Mask;

  if (TRUE == tmp)
    Res ^= IEEESPSign_Mask;

  if (Res < 0)
    SetSR(Negative_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);

  return Res;
AROS_LIBFUNC_EXIT
} /* IEEESPCos */
