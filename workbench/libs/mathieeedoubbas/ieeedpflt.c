/*
    (C) 1995-97 AROS - The Amiga Replacement OS
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

      AROS_LHQUAD1(QUAD, IEEEDPFlt,

/*  SYNOPSIS */
      AROS_LHAQUAD(LONG, y, D0, D1),

/*  LOCATION */
      struct MathIeeeDoubBasBase *, MathIeeeDoubBasBase, 6, MathIeeeDoubBas)

/*  FUNCTION


    INPUTS
	y  - signed integer number

    RESULT
      IEEE double precision number

	Flags:
	  zero	   : result is zero
	  negative : result is negative
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
  LONG Exponent = 0;
  LONG TestMask = 0xFFFFFFFF;
  QUAD Res, yQuad, ExponentQuad;

  Set_Value64C(Res,0,0,0);

  if (y == 0)
  {
    SetSR(Zero_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
    return Res; /* return 0 */
  }

  if (y < 0 )
  {
    Set_Value64C(Res, IEEEDPSign_Mask_Hi,
		      IEEEDPSign_Mask_Lo,
		      IEEEDPSign_Mask_64);
    y = -y;
  }
  /* find out which is the number of the highest set bit */
  while (TestMask & y)
  {
    Exponent ++;
    TestMask <<= 1;
  }

  SHL32(yQuad , y , (53 - Exponent) );

  AND64QC(yQuad,  IEEEDPMantisse_Mask_Hi,
		  IEEEDPMantisse_Mask_Lo,
		  IEEEDPMantisse_Mask_64);

  Exponent += 0x3fe;

  /* adapt Exponent to IEEEDP-Format */
  SHL32(ExponentQuad, Exponent, 52);
  OR64Q(Res, yQuad);
  OR64Q(Res, ExponentQuad);
  if ( is_lessSC(Res,0,0,0) ) /* Res < 0 */
    SetSR(Negative_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);

  return Res;
AROS_LIBFUNC_EXIT
} /* IEEEDPFlt */
