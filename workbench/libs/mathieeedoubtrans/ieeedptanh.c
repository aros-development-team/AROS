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

      AROS_LHQUAD1(QUAD, IEEEDPTanh,

/*  SYNOPSIS */

      AROS_LHAQUAD(QUAD, y, D0, D1),

/*  LOCATION */

      struct MathIeeeDoubTransBase *, MathIeeeDoubTransBase, 12, MathIeeeDoubTrans)

/*  FUNCTION

      Calculate hyperbolic tangens of the IEEE double precision number

    INPUTS

      y - IEEE double precision floating point number

    RESULT

      IEEE double precision floating point number


      flags:
        zero     : result is zero
        negative : result is negative
        overflow : (not possible)

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

                ( e^x - e^(-x) )
     tanh(x) =  ----------------
                ( e^x + e^(-x) )

     tanh( |x| >= 18 ) = 1

    HISTORY

******************************************************************************/

{
QUAD Res;
QUAD y2;
  /* y2 = y & (IEEESPMantisse_Mask + IEEESPExponent_Mask ); */
  Set_Value64(y2, y);
  AND64QC(y2, (IEEEDPMantisse_Mask_Hi + IEEEDPExponent_Mask_Hi),
              (IEEEDPMantisse_Mask_Lo + IEEEDPExponent_Mask_Lo),
              (IEEEDPMantisse_Mask_64 + IEEEDPExponent_Mask_64));

  if ( is_geqC(y2, 0x40320000, 0x0, 0x4032000000000000ULL) )
  {
  /* tanh( x > 18 ) =  1
  ** tanh( x <-18 ) = -1
  */
    if ( is_lessSC(y, 0x0, 0x0, 0x0ULL))
    {
      SetSR(Negative_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
      Set_Value64C(Res, 0xbfefffff, 0xffffffff, 0xbfefffffffffffffULL);
    }
    else
      Set_Value64C(Res, 0x3fefffff, 0xffffffff, 0x3fefffffffffffffULL);

    return Res;
  }
  /* tanh(-x) = -tanh(x) */
  {
  QUAD One, tmp1, tmp2;
  Set_Value64C(One, 0x3ff00000, 0x0, 0x3ff0000000000000ULL);

  Res = IEEEDPExp(y2);
  tmp1 = IEEEDPDiv(One, Res);
  Set_Value64(tmp2, tmp1);
  OR64QC(tmp2, IEEEDPSign_Mask_Hi,
               IEEEDPSign_Mask_Lo,
               IEEEDPSign_Mask_64);
  Res = IEEEDPDiv( IEEEDPAdd(Res, tmp2) ,
                   IEEEDPAdd(Res, tmp1) );
        }
  /* Result is zero */
  if (is_eqC(Res, 0x0, 0x0, 0x0ULL) )
  {
    if (is_lessSC(y, 0x0, 0x0, 0x0ULL))
      SetSR(Zero_Bit | Negative_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
    else
      SetSR(Zero_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
    return Res;
  }

  /* Argument is negative -> result is negative */
  if ( is_lessSC(y, 0x0, 0x0, 0x0ULL))
  {
    SetSR(Negative_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
    OR64QC(Res, IEEEDPSign_Mask_Hi,
                IEEEDPSign_Mask_Lo,
                IEEEDPSign_Mask_64);
    return Res;
  }

  return Res;
} /* IEEEDPTanh */
