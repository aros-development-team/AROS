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

      AROS_LHQUAD1(double, IEEEDPAbs,

/*  SYNOPSIS */
      AROS_LHAQUAD(double, y, D0, D1),

/*  LOCATION */
      struct MathIeeeDoubBasBase *, MathIeeeDoubBasBase, 9, MathIeeeDoubBas)

/*  FUNCTION
      Calculate the absolute value of the given IEEE double precision
      floating point number

    INPUTS
	y  - IEEE double precision floting point number

    RESULT
	absolute value of y

	Flags:
	  zero	   : result is zero
	  negative : 0
	  overflow : 0

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO


    INTERNALS
      ALGORITHM:
	set the sign-bit to zero

    HISTORY

******************************************************************************/
{
    /*if (0 == y) */
    if (is_eqC(y,0,0,0ULL))
      /* value is 0 -> set the Zero Flag */
      SetSR( Zero_Bit, Zero_Bit | Overflow_Bit | Negative_Bit);
    else
    {
      /* set the sign-bit to zero */
      /* y &= (IEEEDPMantisse_Mask | IEEEDPExponent_Mask) */
      AND64QC(y, (IEEEDPMantisse_Mask_Hi | IEEEDPExponent_Mask_Hi),
		 (IEEEDPMantisse_Mask_Lo | IEEEDPExponent_Mask_Lo),
		 (IEEEDPMantisse_Mask_64 | IEEEDPExponent_Mask_64) )
      SetSR(0, Zero_Bit | Overflow_Bit | Negative_Bit );
    }
    return y;
} /* IEEEDPAbs */

