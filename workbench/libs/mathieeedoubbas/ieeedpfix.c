/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang:
*/

#include <aros/libcall.h>
#include <proto/mathieeedoubbas.h>
#include <proto/exec.h>
#include <exec/types.h>
#include "mathieeedoubbas_intern.h"

/*****************************************************************************

    NAME */

      AROS_LHQUAD1(LONG, IEEEDPFix,

/*  SYNOPSIS */
      AROS_LHAQUAD(QUAD, y, D0, D1),

/*  LOCATION */
      struct MathIeeeDoubBasBase *, MathIeeeDoubBasBase, 5, Mathieeedoubbas)

/*  FUNCTION
        Convert IEEE double precision floating point number to integer

    INPUTS
        y - IEEE double precision floating point

    RESULT
        absolute value of y

        Flags:
          zero     : result is zero
          negative : result is negative
          overflow : ieeedp out of integer-range

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO


    INTERNALS

    HISTORY

******************************************************************************/
{
AROS_LIBFUNC_INIT
LONG Res, Shift, tmp;
QUAD y2;

tmp = Get_High32of64(y) & IEEEDPExponent_Mask_Hi;

  if ( tmp > 0x41d00000 )
    if( is_lessSC(y, 0x0, 0x0, 0x0UUL))
    {
      SetSR(Overflow_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
      return 0x80000000;
    }
    else
    {
      SetSR(Overflow_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
      return 0x7fffffff;
    }


  if (is_eqC(y, 0x0, 0x0, 0x0UUL) ||
      is_eqC(y,IEEEDPSign_Mask_Hi,
               IEEEDPSign_Mask_Lo,
               IEEEDPSign_Mask_64)) /* y=+-0; */
  {
    SetSR(Zero_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
    return 0;
  }

  Shift = (Get_High32of64(y) & IEEEDPExponent_Mask_Hi) >> 20;
  Shift = 0x433 - Shift;
  tmp = Get_High32of64(y);
  AND64QC(y, IEEEDPMantisse_Mask_Hi,
             IEEEDPMantisse_Mask_Lo,
             IEEEDPMantisse_Mask_64);
  OR64QC(y,  0x00100000,
             0x00000000,
             0x0010000000000000UUL);
  SHRU64(y2, y , Shift);
  Res = Get_Low32of64(y2);

  /* Test for a negative sign  */
  if (tmp < 0) /* y < 0 */
  {
    Res = -Res;
    SetSR(Negative_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
  }

  return Res;
AROS_LIBFUNC_EXIT
} /* IEEEDPFix */

