/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang:
*/

#include <libraries/mathieeesp.h>
#include <aros/libcall.h>
#include <proto/mathffp.h>
#include <proto/exec.h>
#include <exec/types.h>
#include "mathieeesp_intern.h"

/*****************************************************************************

    NAME */

        AROS_LH1(LONG, IEEESPNeg,

/*  SYNOPSIS */
        AROS_LHA(LONG, y, D0),

/*  LOCATION */
        struct MathIeeeSingBasBase *, MathIeeeSingBasBase, 10, Mathieeespbas)

/*  FUNCTION
        Switch the sign of the given ieeesp number

    INPUTS
        y  - IEEE single precision floating point

    RESULT
        -y

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
        Return -0 if y == 0.
        Otherwise flip the sign-bit.

    HISTORY

******************************************************************************/

{
  if (0 == y || 0x80000000 == y)
  {
    SetSR( Zero_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
    return (IEEESPSign_Mask ^ y);
  }

  /* flip sign-bit */
  y ^= IEEESPSign_Mask;

  if(y < 0)
  /* result is negative */
    SetSR(Negative_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
  else
  /* result is positive */
    SetSR(0, Zero_Bit | Overflow_Bit | Negative_Bit );

  return y;
} /* IEEESPNeg */

