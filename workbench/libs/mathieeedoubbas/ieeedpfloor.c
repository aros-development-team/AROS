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

      AROS_LHQUAD1(double, IEEEDPFloor,

/*  SYNOPSIS */
      AROS_LHAQUAD(double, y, D0, D1),

/*  LOCATION */
      struct MathIeeeDoubBasBase *, MathIeeeDoubBasBase, 15, MathIeeeDoubBas)

/*  FUNCTION
	Calculates the floor-value of a IEEE double precision number

    INPUTS
	y  - IEEE double precision floating point
	z  - IEEE double precision floating point

    RESULT
       +1 : y > z
	0 : y = z
       -1 : y < z


	Flags:
	  zero	   : y = z
	  negative : y < z
	  overflow : 0

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/
{
AROS_LIBFUNC_INIT
  QUAD Mask;
  QUAD y_tmp,y2,tmp;
  int shift;

  if (is_eqC(y,0,0,0))
    return y;

  Set_Value64(y_tmp, y);
  AND64QC(y_tmp, IEEEDPExponent_Mask_Hi,
                 IEEEDPExponent_Mask_Lo,
                 IEEEDPExponent_Mask_64);

  if (is_lessC(y_tmp, one_Hi, one_Lo, one_64))
  {
    if (is_lessSC(y,0,0,0))
    {
      SetSR(Negative_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
    	Set_Value64C(tmp, one_Hi | IEEEDPSign_Mask_Hi,
    	                  one_Lo | IEEEDPSign_Mask_Lo,
    	                  one_64 | IEEEDPSign_Mask_64);
      return tmp;
    }
    else
    {
      SetSR(Zero_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
      Set_Value64C(tmp, 0,0,0);
      return tmp;
    }
  }

  /* |fnum| >= 1 */
  Set_Value64C(Mask, 0x80000000,
                     0x00000000,
                     0x8000000000000000ULL);
  SHRU32(shift, y_tmp, 52);
  SHRS64(Mask, Mask, (shift-0x3ff+11));  /* leave the () there! StormC 1.1 needs 'em! */

  /* y is negative */
  if (is_leqSC(y, 0x0, 0x0, 0x0))
  {
    QUAD Mask2;
    NOT64(Mask2, Mask);
    Set_Value64(y2,y);
    AND64Q(y2, Mask2);
    if (is_neqC(y2,0x0,0x0,0x0))
    {
      QUAD minusone;
    	Set_Value64C(minusone, one_Hi | IEEEDPSign_Mask_Hi,
    	                       one_Lo | IEEEDPSign_Mask_Lo,
    	                       one_64 | IEEEDPSign_Mask_64);
      y = IEEEDPAdd(y, minusone);
      Set_Value64C(Mask, 0x80000000,
                         0x00000000,
                         0x8000000000000000);
      SHRU32(shift, y_tmp, 52);
      SHRS64(Mask, Mask, (shift-0x3ff+11)); /* leave the () there! StormC 1.1 needs 'em! */
    }
  }

  if (is_lessSC(y,0x0,0x0,0x0))
    SetSR(Negative_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);

  AND64Q(y, Mask);

  return y;

AROS_LIBFUNC_EXIT
} /* IEEEDPFloor */

