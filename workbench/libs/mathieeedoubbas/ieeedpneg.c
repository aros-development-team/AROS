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

      AROS_LHQUAD1(double, IEEEDPNeg,

/*  SYNOPSIS */
      AROS_LHAQUAD(double, y, D0, D1),

/*  LOCATION */
      struct MathIeeeDoubBasBase *, MathIeeeDoubBasBase, 10, MathIeeeDoubBas)

/*  FUNCTION
	Switch the sign of the given IEEE double precision floating point
	number

    INPUTS
	y  - IEEE double precision floating point

    RESULT
	-y

	Flags:
	  zero	   : result is zero
	  negative : result is negative
	  overflow : 0

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO


    INTERNALS
      ALGORITHM:

	Flip the sign-bit (even for zeroes).

    HISTORY

******************************************************************************/
{
AROS_LIBFUNC_INIT
  /* change the sign-bit */
  XOR64QC(y, IEEEDPSign_Mask_Hi,
	     IEEEDPSign_Mask_Lo,
	     IEEEDPSign_Mask_64);

  if (is_eqC(y, 0x0, 0x0, 0x0ULL) ||
      is_eqC(y, IEEEDPSign_Mask_Hi,
		IEEEDPSign_Mask_Lo,
		IEEEDPSign_Mask_64) )
  {
    SetSR( Zero_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
   return y;
  }

  /* if (y < 0) */
  if(is_lessSC(y, 0x0, 0x0, 0x0ULL) )
  /* result is negative */
    SetSR(Negative_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
  else
  /* result is positive */
    SetSR(0, Zero_Bit | Overflow_Bit | Negative_Bit );

  return y;
AROS_LIBFUNC_EXIT
} /* IEEEDPNeg */

