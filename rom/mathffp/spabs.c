/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.4  1998/02/01 21:47:19  bergers
    Use float instead of LONG when calling these functions now.
    A define in mathffp_intern.h does the trick.

    Revision 1.3  1997/09/16 23:00:46  bergers
    Added the missing AROS_LIBFUNC_INITs and EXITs

    Revision 1.2  1997/06/25 21:36:43  bergers
    *** empty log message ***

    Revision 1.1  1997/05/30 20:50:57  aros
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

        AROS_LH1(float, SPAbs,

/*  SYNOPSIS */
        AROS_LHA(float, fnum1, D0),

/*  LOCATION */
        struct MathBase *, MathBase, 9, Mathffp)

/*  FUNCTION
        Calculate the absolute value of a given floating point number
    INPUTS
        fnum1  - ffp number

    RESULT
        absolute value of fnum1

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
    if (0 == fnum1)
      /* value is 0 -> set the Zero Flag */
      SetSR( Zero_Bit, Zero_Bit | Overflow_Bit | Negative_Bit);
    else
    {
      fnum1 &= (FFPMantisse_Mask | FFPExponent_Mask);
      /* set the sign-bit to zero */
      SetSR(0, Zero_Bit | Overflow_Bit | Negative_Bit );
    }
    return fnum1;
AROS_LIBFUNC_EXIT
} /* SPAbs */
