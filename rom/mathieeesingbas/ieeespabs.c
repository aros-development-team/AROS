/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang:
*/

#include <libraries/mathieeesp.h>
#include <aros/libcall.h>
#include <proto/mathieeesingbas.h>
#include <proto/exec.h>
#include <exec/types.h>
#include "mathieeesingbas_intern.h"

/*****************************************************************************

    NAME */

        AROS_LH1(float, IEEESPAbs,

/*  SYNOPSIS */
        AROS_LHA(float, y, D0),

/*  LOCATION */
        struct MathIeeeSingBasBase *, MathIeeeSingBasBase, 9, Mathieeesingbas)

/*  FUNCTION
        Calculate the absolute value of a given floating point number
    INPUTS
        y  - ieeesp number

    RESULT
        absolute value of y

        Flags:
          zero     : result is zero
          negative : result is negative
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
AROS_LIBFUNC_INIT
    if (0 == y)
      /* value is 0 -> set the Zero Flag */
      SetSR( Zero_Bit, Zero_Bit | Overflow_Bit | Negative_Bit);
    else
    {
      /* set the sign-bit to zero */
      y &= (IEEESPMantisse_Mask | IEEESPExponent_Mask);
      SetSR(0, Zero_Bit | Overflow_Bit | Negative_Bit );
    }
    return y;
AROS_LIBFUNC_EXIT
} /* IEEESPAbs */

