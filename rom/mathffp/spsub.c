/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.4  1998/02/01 21:47:19  bergers
    Use float instead of LONG when calling these functions now.
    A define in mathffp_intern.h does the trick.

    Revision 1.3  1997/09/16 23:00:47  bergers
    Added the missing AROS_LIBFUNC_INITs and EXITs

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

        AROS_LH2(float, SPSub,

/*  SYNOPSIS */
        AROS_LHA(float, fnum1, D0),
        AROS_LHA(float, fnum2, D1),

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
AROS_LIBFUNC_INIT
  return SPAdd(fnum2, fnum1 ^ FFPSign_Mask);
AROS_LIBFUNC_EXIT
} /* SPSub */

