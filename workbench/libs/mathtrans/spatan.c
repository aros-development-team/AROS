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

      AROS_LH1(float, SPAtan,

/*  SYNOPSIS */

      AROS_LHA(float, fnum1 , D0),

/*  LOCATION */

      struct Library *, MathTransBase, 5, Mathtrans)

/*  FUNCTION

      Calculates the angle of a given number representing the tangent
      of that angle. The angle will be in radians.

    INPUTS

      fnum1   - Motorola fast floating point number

    RESULT

      Motorola fast floating point number

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/
{
  LONG fnumabs = fnum1 & (FFPMantisse_Mask | FFPExponent_Mask);
  LONG fnumsquared, fnumcubed;

  /* check for +- infinity -> output: +-pi/2 */
  if (FFP_Pinfty == fnumabs )
    return (pio2 | (fnum1 & FFPSign_Mask));

  /* atan(0) = 0 */
  if (0 == fnumabs)
  {
     SetSR(Zero_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
     return 0;
  }

  /* atan(x>= 128) = pi/2 - 1/x */
  if ((BYTE) fnumabs >= 0x48)
    if (fnumabs == fnum1) /* arg has ppositive sign */
    {
      SetSR(0, Zero_Bit | Negative_Bit | Overflow_Bit);
      return SPSub(SPDiv(fnumabs,one),pio2);
    }
    else
    {
      SetSR(Negative_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
      return (SPSub(SPDiv(fnumabs,one),pio2)) | FFPSign_Mask;
    }

  /* atan(x >= 64) = pi/2 - 1/x +1/(3*x^3) */

  fnumsquared = SPMul(fnumabs, fnumabs);

  if((BYTE) fnumabs >= 0x47)
  {
    fnumcubed   = SPMul(fnumabs, fnumsquared);

    /* pi/2 - 1/x + 1/(3*x^3) = pi/2 + (1-3*x^2)/(3*x^3)*/
    if (fnumabs == fnum1) /* arg has positive sign */
    {
      SetSR(0, Zero_Bit | Negative_Bit | Overflow_Bit);
      return SPAdd(pio2,
              SPDiv(
               SPMul(three, fnumcubed),
              SPAdd(
               SPMul(three, fnumsquared) | FFPSign_Mask,
                    one )
                   ));
    }
    else
    {
      SetSR(Negative_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
      return SPAdd(pio2,
              SPDiv(
               SPMul(three, fnumcubed),
              SPAdd(
               SPMul(three, fnumsquared) | FFPSign_Mask,
                    one )
                   )) | FFPSign_Mask;
    }
  }

  /* atan(x <= 64) */
  return SPAsin(SPDiv(SPSqrt(SPAdd(one,fnumsquared)),fnum1));

} /* SPAtan */
