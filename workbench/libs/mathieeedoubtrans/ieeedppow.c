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

      AROS_LHQUAD2(QUAD, IEEEDPPow,

/*  SYNOPSIS */

      AROS_LHAQUAD(QUAD, x, D2, D3),
      AROS_LHAQUAD(QUAD, y, D0, D1),

/*  LOCATION */

      struct MathIeeeDoubTransBase *, MathIeeeDoubTransBase, 15, Mathieeedoubtrans)

/*  FUNCTION

      Calculate y raised to the x power (y^x)

    INPUTS

      x - IEEE double precision floating point number
      y - IEEE double precision floating point number

    RESULT

      IEEE double precision floating point number


      flags:
        zero     : result is zero
        negative : result is negative
        overflow : result is too big

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/

{
AROS_LIBFUNC_INIT

  /* a ^ b  = e^(b * ln a )
  ** y ^ x  = e^(x * ln y )
  */
  QUAD Res, tmp;

  /* y^x is illegal if y<0 and x is not an integer-value */
  if (is_lessSC(y, 0x0, 0x0, 0x0UUL) &&
      is_neq(x, IEEEDPCeil(x))            )
  {
    Set_Value64C(Res, 0x0, 0x0, 0x0UUL);
    return Res;
  }

  if (is_eqC(y, 0x0, 0x0, 0x0UUL))
  {
    Set_Value64C(Res, 0x3ff00000, 0x0, 0x3ff0000000000000UUL);
    return Res;
  }
  Set_Value64(tmp, y);
  AND64QC(tmp, (IEEEDPMantisse_Mask_Hi + IEEEDPExponent_Mask_Hi),
               (IEEEDPMantisse_Mask_Lo + IEEEDPExponent_Mask_Lo),
               (IEEEDPMantisse_Mask_64 + IEEEDPExponent_Mask_64) )

  Res = IEEEDPLog(tmp);
  Res = IEEEDPMul(Res, x);
  Res = IEEEDPExp(Res);

  /* if y < 0 and x was and even integer, the result is positive, otherwise
  ** it is negative.
  */
  if ( is_lessSC(y, 0x0, 0x0, 0x0UUL) &&
       TRUE == intern_IEEEDPisodd(x) )
    OR64QC(Res, IEEEDPSign_Mask_Hi,
                IEEEDPSign_Mask_Lo,
                IEEEDPSign_Mask_64);

  if (is_eqC(Res, 0x0, 0x0, 0x0UUL))
  {
    SetSR(Zero_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
    Set_Value64C(Res, 0x0, 0x0, 0x0UUL);
    return Res;
  }

  SetSR(0, Zero_Bit | Negative_Bit | Overflow_Bit);

  if (is_lessSC(Res, 0x0, 0x0, 0x0UUL))
    SetSR(Negative_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);

  Set_Value64(tmp, Res);
  AND64QC(tmp, (IEEEDPMantisse_Mask_Hi + IEEEDPExponent_Mask_Hi),
               (IEEEDPMantisse_Mask_Lo + IEEEDPExponent_Mask_Lo),
               (IEEEDPMantisse_Mask_64 + IEEEDPExponent_Mask_64));

  if ( is_eqC(Res, IEEEDPPInfty_Hi,
                   IEEEDPPInfty_Lo,
                   IEEEDPPInfty_64) )
    /* don`t touch the Negative_Bit now!*/
    SetSR(Overflow_Bit, Zero_Bit | Overflow_Bit);

  return Res;
AROS_LIBFUNC_EXIT
} /* IEEEDPPow */
