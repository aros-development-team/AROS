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

      AROS_LHQUAD1(QUAD, IEEEDPSinh,

/*  SYNOPSIS */

      AROS_LHAQUAD(QUAD, y , D0, D1),

/*  LOCATION */

      struct MathIeeeDoubTransBase *, MathIeeeDoubTransBase, 13, Mathieeedoubtrans)

/*  FUNCTION

      Calculate the hyperbolic sine of the IEEE double precision number

    INPUTS

      y - IEEE double precision floating point number

    RESULT

      IEEE double precision floating point number


      flags:
        zero     : result is zero
        negative : result is negative
        overflow : result is too big for IEEE double precsion format

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS
      sinh(x) = (1/2)*( e^x- e^(-x) )

      sinh( |x| >=  18 ) = (1/2) * (e^x);
    HISTORY

******************************************************************************/

{
QUAD Res;
QUAD y2;

  /* y2 = y & (IEEEDPMantisse_Mask + IEEEDPExponent_Mask); */
  Set_Value64(y2, y);
  AND64QC(y2, (IEEEDPMantisse_Mask_Hi + IEEEDPExponent_Mask_Hi),
              (IEEEDPMantisse_Mask_Lo + IEEEDPExponent_Mask_Lo),
              (IEEEDPMantisse_Mask_64 + IEEEDPExponent_Mask_64));

  if ( is_eqC(y, IEEEDPPInfty_Hi,
                 IEEEDPPInfty_Lo,
                 IEEEDPPInfty_64))
  {
    SetSR(Overflow_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
    return y;
  }

  /* sinh(-x) = -sinh(x) */
  Res = IEEEDPExp(y2);

  if ( is_eqC(Res, IEEEDPPInfty_Hi,
                   IEEEDPPInfty_Lo,
                   IEEEDPPInfty_64))
   {
    SetSR(Overflow_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
    if ( is_lessSC(y, 0x0, 0x0, 0x0UUL))
      OR64QC(Res, IEEEDPSign_Mask_Hi,
                  IEEEDPSign_Mask_Lo,
                  IEEEDPSign_Mask_64);
    return Res;
  }

  if ( is_lessC(y2, 0x40320000, 0x0, 0x4032000000000000UUL) )
  {
    QUAD One, ResTmp;
    Set_Value64C(One, 0x3ff00000, 0x0, 0x3ff0000000000000UUL);
    ResTmp = IEEEDPDiv(One, Res);
    OR64QC(ResTmp, IEEEDPSign_Mask_Hi,
                   IEEEDPSign_Mask_Lo,
                   IEEEDPSign_Mask_64);

    Res = IEEEDPAdd(Res, ResTmp);
  }
  /* Res = Res / 2 */
  ADD64QC(Res, 0xFFF00000, 0x0, 0xFFF0000000000000UUL);

  /* at this point Res has to be positive to be valid */
  if ( is_leqSC(Res, 0x0, 0x0, 0x0UUL))
  {
    SetSR(Zero_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
    AND64QC(y, IEEEDPSign_Mask_Hi,
               IEEEDPSign_Mask_Lo,
               IEEEDPSign_Mask_64);
    return y;
  }

  if ( is_lessSC(y, 0x0, 0x0, 0x0UUL))
  {
    SetSR(Negative_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
    OR64QC(Res, IEEEDPSign_Mask_Hi,
                IEEEDPSign_Mask_Lo,
                IEEEDPSign_Mask_64);

    return Res;
  }
  return Res;
} /* IEEEDPSinh */
