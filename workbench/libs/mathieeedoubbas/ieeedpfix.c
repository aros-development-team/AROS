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
  LONG Res;
  LONG Shift;

/*
  if ((y & IEEESPExponent_Mask) > 0x60000000 )
    if(y < 0)
    {
      SetSR(Overflow_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
      return 0x80000000;
    }
    else
    {
      SetSR(Overflow_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
      return 0x7fffffff;
    }
*/

  if (is_eqC(y,0,0,0) || 
      is_eqC(y,IEEEDPSign_Mask_Hi, 
               IEEEDPSign_Mask_Lo,
               IEEEDPSign_Mask_64)) /* y=+-0; */
  {
    SetSR(Zero_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
    return 0;
  }

  Shift = (Get_High32OF64(y) & IEEEDPExponent_Mask_Hi) >> 20;
  Shift -= 0x3fe;
  SHRU64(y,y,Shift);
  Res = Get_Low32OF64(y);

  /* Test for a negative sign  */
  if (is_lessSC(y,0,0,0)) /* y < 0 */
  {
    Res = -Res;
    SetSR(Negative_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
  }

  return Res;
AROS_LIBFUNC_EXIT
} /* IEEEDPFix */

