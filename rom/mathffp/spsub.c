/*
    (C) 1995-97 AROS - The Amiga Replacement OS
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

        AROS_LH2(LONG, SPSub,

/*  SYNOPSIS */
        AROS_LHA(LONG, fnum1, D0),
        AROS_LHA(LONG, fnum2, D1),

/*  LOCATION */
        struct MathffpBase *, MathBase, 12, Mathffp)

/*  FUNCTION
        Subtract two floating point numbers
        fnum = fnum2 - fnum1;

    INPUTS
        fnum1  - ffp number
        fnum2  - ffp number

    RESULT

        Flags:
          zero     : result is zero
          negative : result is negative
          overflow : result is out of range

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO


    INTERNALS

    HISTORY

******************************************************************************/
/*
    ALGORITHM
        fnum = fnum2 + (-fnum1).
        Use the SPAdd-Function for the subtraction. Therefore just flip the
        sign-bit of fnum2


*/
{
  return SPAdd(fnum2, fnum1 ^ FFPSign_Mask);
} /* SPSub */

