/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.2  1997/06/25 21:36:44  bergers
    *** empty log message ***

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
        struct MathBase *, MathBase, 12, Mathffp)

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
      ALGORITHM:
        fnum = fnum2 + (-fnum1).

    HISTORY

******************************************************************************/

{
  return SPAdd(fnum2, fnum1 ^ FFPSign_Mask);
} /* SPSub */

