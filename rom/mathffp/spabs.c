/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
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

        AROS_LH1(LONG, SPAbs,

/*  SYNOPSIS */
        AROS_LHA(LONG, fnum1, D0),

/*  LOCATION */
        struct MathffpBase *, MathBase, 9, Mathffp)

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

    HISTORY

******************************************************************************/
/*
    ALGORITHM
        set the sign-bit to zero
*/
{
    if (0 == fnum1)
      //value is 0 -> set the Zero Flag
      SetSR( Zero_Bit, Zero_Bit | Overflow_Bit | Negative_Bit);
    else
    {
      fnum1 &= (FFPMantisse_Mask | FFPExponent_Mask);
      //set the sign-bit to zero
      SetSR(0, Zero_Bit | Overflow_Bit | Negative_Bit );
    }
    return fnum1;

} /* SPAbs */

