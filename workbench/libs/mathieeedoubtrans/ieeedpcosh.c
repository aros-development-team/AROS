/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang: english
*/


#include <aros/libcall.h>
#include <proto/mathieeedoubbas.h>
#include <proto/mathieeedoubtrans.h>
#include <proto/exec.h>
#include <exec/types.h>
#include "mathieeedoubtrans_intern.h"


/*****************************************************************************

    NAME */

      AROS_LHQUAD1(QUAD, IEEEDPCosh,

/*  SYNOPSIS */

      AROS_LHAQUAD(QUAD, y, D0, D1),

/*  LOCATION */

      struct MathIeeeDoubTransBase *, MathIeeeDoubTransBase, 13, Mathieeedoubtrans)

/*  FUNCTION

      Calculate the hyperbolic cosine of the IEEE single precision number

    INPUTS

      y - IEEE single precision floating point number

    RESULT

      IEEE single precision floating point number


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

      cosh( |x| >= 18 ) = (1/2) * (e^x);

    HISTORY

******************************************************************************/
{
QUAD Res;
  /* cosh(-x) = cosh(x) */
  /* y &= ( IEEEDPMantisse_Mask + IEEEDPExponent_Mask ); */
  AND64QC(y, (IEEEDPMantisse_Mask_Hi + IEEEDPExponent_Mask_Hi),
             (IEEEDPMantisse_Mask_Lo + IEEEDPExponent_Mask_Lo),
             (IEEEDPMantisse_Mask_64 + IEEEDPExponent_Mask_64));

  if ( is_eqC(y, IEEEDPPInfty_Hi,
                 IEEEDPPInfty_Lo,
                 IEEEDPPInfty_64))
  {
    SetSR(Overflow_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
    return y;
  }

  Res = IEEEDPExp(y);

  if ( is_eqC(Res, IEEEDPPInfty_Hi,
                   IEEEDPPInfty_Lo,
                   IEEEDPPInfty_64))
  {
    SetSR(Overflow_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
    return  Res;
  }

  /* does adding of 1/(e^x) still change the result? */
  if ( is_lessC(y, 0x40320000, 0x0, 0x4032000000000000ULL ))
  {
    QUAD One;
    Set_Value64C(One, 0x3ff00000, 0x0, 0x3ff0000000000000ULL);

    Res = IEEEDPAdd(Res, IEEEDPDiv(One, Res));
  }
  /* Res = Res / 2 */
  ADD64QC(Res, 0xFFF00000, 0x0, 0xFFF0000000000000ULL);

  if ( is_eqC   (Res, 0x0, 0x0, 0x0ULL) ||
       is_lessSC(Res, 0x0, 0x0, 0x0ULL) )
  {
    SetSR(Zero_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
    Set_Value64C(Res, 0x0, 0x0, 0x0ULL);
    return Res;
  }
  return Res;
} /* IEEEDPCosh */
