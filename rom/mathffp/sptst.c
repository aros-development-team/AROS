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

        AROS_LH1(LONG, SPTst,

/*  SYNOPSIS */
        AROS_LHA(LONG, fnum, D1),

/*  LOCATION */
        struct MathffpBase *, MathBase, 8, Mathffp)

/*  FUNCTION
        Compare a ffp-number against zero.


    INPUTS
        fnum  - ffp number

    RESULT

        +1 : fnum > 0.0
         0 : fnum = 0.0
        -1 : fnum < 0.0

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
        Sign is negative: return -1
        fnum == 0       : return 0
        Otherwise       : return 1

*/
{
  // fnum1 is negative
  if ((char) fnum < 0)
  {
    SetSR(Negative_Bit, Zero_Bit | Overflow_Bit | Negative_Bit);
    return -1;
  }

  // fnum1 is zero
  if (0 == fnum)
  {
    SetSR(Zero_Bit, Zero_Bit | Overflow_Bit | Negative_Bit);
    return 0;
  }

  // fnum1 is positive
  SetSR(0, Zero_Bit | Overflow_Bit | Negative_Bit );
  return 1;
} /* SPTst */

