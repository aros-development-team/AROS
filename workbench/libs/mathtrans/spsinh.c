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

      AROS_LH1(LONG, SPSinh,

/*  SYNOPSIS */

      AROS_LHA(LONG, fnum1 , D0),

/*  LOCATION */

      struct Library *, MathTransBase, 10, Mathtrans)

/*  FUNCTION

      Calculate the hyperbolic sine of the ffp number

    INPUTS

      fnum1 - Motorola fast floating point number

    RESULT

      Motorola fast floating point number


      flags:
        zero     : result is zero
        negative : result is negative
        overflow : result is too big for ffp format

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS
      sinh(x) = (1/2)*( e^x- e^(-x) )

      sinh( |x| >= 44 ) = infinity;
      sinh( |x| >=  9 ) = (1/2) * (e^x);
    HISTORY

******************************************************************************/

{
ULONG Res;
LONG tmp;
  /* sinh(-x) = -sinh(x) */
  Res = SPExp(fnum1 & (FFPMantisse_Mask + FFPExponent_Mask) );

  if ( FFP_Pinfty == Res )
  {
    SetSR(Overflow_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
    return Res;
  }

  tmp = (fnum1 & FFPExponent_Mask) - 0x41;

  if ( tmp <= 2  || (tmp == 3 && (fnum1 & FFPMantisse_Mask) < 0x90000000) )
    Res = SPAdd(Res, ((ULONG)SPDiv(Res, one) | FFPSign_Mask ));

  /* Res = Res / 2 */
  ((char)Res) --;

  if ( 0 == Res || (char)Res < 0 )
  {
    SetSR(Zero_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
    return 0;
  }

  /* if the argument was negative, the result is also negative */
  if ((char)fnum1 < 0 )
  {
    SetSR(Negative_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
    return (Res | FFPSign_Mask);
  }
  return Res;
} /* SPSinh */
