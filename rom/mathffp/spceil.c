/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "mathffp_intern.h"

/*
    FUNCTION
        Calculate the least integer ffp-number greater than or equal to
        fnum1

    RESULT


        Flags:
          zero     : result is zero
          negative : result is negative
          overflow : 0

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        @Math.Floor@

    INTERNALS
      ALGORITHM:
         Ceil(y) = - Floor(-y)

    HISTORY
*/
AROS_LH1(float, SPCeil,
    AROS_LHA(float, y, D0),
    struct LibHeader *, MathBase, 16, Mathffp
)
{
    AROS_LIBFUNC_INIT
    
    /* Ceil(y) = -Floor(-y) */
    y = SPFloor(y ^ FFPSign_Mask);
    return (y ^ FFPSign_Mask);

    AROS_LIBFUNC_EXIT
}
