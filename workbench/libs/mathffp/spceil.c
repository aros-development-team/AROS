/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "mathffp_intern.h"

/*****************************************************************************

    NAME */

        AROS_LH1(float, SPCeil,

/*  SYNOPSIS */
        AROS_LHA(float, y, D0),

/*  LOCATION */
        struct LibHeader *, MathBase, 16, Mathffp)

/*  FUNCTION
        Calculate the least integer ffp-number
        greater than or equal to fnum1

    INPUTS

    RESULT
        Flags:
          zero     : result is zero
          negative : result is negative
          overflow : 0

    BUGS

    INTERNALS
        ALGORITHM:
        Ceil(y) = - Floor(-y)

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    /* Ceil(y) = -Floor(-y) */
    y = SPFloor(y ^ FFPSign_Mask);
    return (y ^ FFPSign_Mask);

    AROS_LIBFUNC_EXIT
}
