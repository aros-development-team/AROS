/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang:
*/

#include <libraries/mathieeesp.h>
#include <aros/libcall.h>
#include <proto/mathieeesingbas.h>
#include <proto/exec.h>
#include <exec/types.h>
#include "mathieeesp_intern.h"

/*****************************************************************************

    NAME */

        AROS_LH1(LONG, IEEESPTst,

/*  SYNOPSIS */
        AROS_LHA(LONG, y, D0),

/*  LOCATION */
        struct MathIeeeSingBasBase *, MathIeeeSingBasBase, 8, Mathieeespbas)

/*  FUNCTION
        Compare a ieeesp-number against zero.


    INPUTS
        y  - ieeesp number

    RESULT

        +1 : y > 0.0
         0 : y = 0.0
        -1 : y < 0.0

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
        Sign is negative: return -1
        y == 0          : return 0
        Otherwise       : return 1

    HISTORY

******************************************************************************/

{
  /* y is negative */
  if (y < 0)
  {
    SetSR(Negative_Bit, Zero_Bit | Overflow_Bit | Negative_Bit);
    return -1;
  }

  /* fnum1 is zero */
  if (0 == y)
  {
    SetSR(Zero_Bit, Zero_Bit | Overflow_Bit | Negative_Bit);
    return 0;
  }

  /* fnum1 is positive */
  SetSR(0, Zero_Bit | Overflow_Bit | Negative_Bit );
  return 1;
} /* IEEESPTst */

