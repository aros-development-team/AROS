/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.1  1997/05/30 20:50:59  aros
    *** empty log message ***


    Desc:
    Lang: english
*/
#include <libraries/mathffp.h>
#include <aros/libcall.h>
#include <proto/mathffp.h>
#include <proto/exec.h>
#include <exec/types.h>
#include "mathffp_intern.h"

/*****************************************************************************

    NAME */

        AROS_LH1(LONG, SPNeg,

/*  SYNOPSIS */
        AROS_LHA(LONG, fnum1, D0),

/*  LOCATION */
        struct MathffpBase *, MathBase, 10, Mathffp)

/*  FUNCTION
        Calculate fnum1*(-1)

    INPUTS
        fnum1  - ffp number

    RESULT
        -fnum1

        Flags:
          zero     : result is zero
          negative : result is negative
          overflow : 0

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO


    INTERNALS

    HISTORY

******************************************************************************/
/*
    ALGORITHM
        Return zero if fnum == 0.
        Otherwise flip the sign-bit.

*/
{
  if (0 == fnum1)
  {
    SetSR( Zero_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
    return 0;
  }

  // flip sign-bit
  fnum1 ^= FFPSign_Mask;

  if((char) fnum1 < 0)
  // result is negative
    SetSR(Negative_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
  else
    // result is positive
    SetSR(0, Zero_Bit | Overflow_Bit | Negative_Bit );

  return fnum1;
} /* SPNeg */

