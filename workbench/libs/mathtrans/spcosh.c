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

      AROS_LH1(LONG, SPCosh,

/*  SYNOPSIS */

      AROS_LHA(LONG, fnum1, D0),

/*  LOCATION */

      struct Library *, MathTransBase, 11, Mathtrans)

/*  FUNCTION

      Calculate the hyperbolic cosine of the ffp number

    INPUTS

      fnum - Motorola fast floating point number

    RESULT

      Motorola fast floating point number


      flags:
        zero     : result is zero
        negative : 0 (not possible)
        overflow : result too big for ffp-number

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS
      cosh(x) = (1/2)*( e^x + e^(-x) )

      cosh( |x| >= 44 ) = infinity;
      cosh( |x| >= 9  ) = (1/2) * (e^x);

    HISTORY

******************************************************************************/

{
ULONG Res;
LONG tmp;
  /* cosh(-x) = cosh(x) */
  fnum1 &= ( FFPMantisse_Mask + FFPExponent_Mask );

  Res = SPExp(fnum1);

  if ( FFP_Pinfty == Res )
  {
    SetSR(Overflow_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
    return Res;
  }

  tmp = (fnum1 & FFPExponent_Mask) - 0x41;

  if ( tmp <= 2 || (tmp == 3 && (fnum1 & FFPMantisse_Mask) < 0x90000000) )
    Res = SPAdd(Res, SPDiv(Res, one));

  /* Res = Res / 2 */
  ((char)Res) --;

  if (0 == Res || (char)Res < 0 )
  {
    SetSR(Zero_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
    return 0;
  }
  return Res;
} /* SPCosh */
