/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.2  1997/06/25 21:36:44  bergers
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

        AROS_LH1(LONG, SPCeil,

/*  SYNOPSIS */
        AROS_LHA(LONG, y, D0),

/*  LOCATION */
        struct MathBase *, MathBase, 16, Mathffp)

/*  FUNCTION
        Calculate the least integer ffp-number greater than or equal to
        fnum1


    INPUTS
        y  - ffp number

    RESULT


        Flags:
          zero     : result is zero
          negative : result is negative
          overflow : 0

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        SPFloor()

    INTERNALS
      ALGORITHM:
         Ceil(y) = - Floor(-y)

    HISTORY

******************************************************************************/

{
  /* Ceil(y) = -Floor(-y) */
  y = SPFloor(y ^ FFPSign_Mask);
  return (y ^ FFPSign_Mask);

} /* SPCeil */

