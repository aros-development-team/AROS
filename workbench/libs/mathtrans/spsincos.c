/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "mathtrans_intern.h"

/*****************************************************************************

    NAME */

        AROS_LH2(float, SPSincos,

/*  SYNOPSIS */
        AROS_LHA(IPTR *, pfnum2, D1),
        AROS_LHA(float, fnum1, D0),

/*  LOCATION */
        struct Library *, MathTransBase, 9, MathTrans)

/*  FUNCTION
        Calculate the cosine and the sine of the given ffp-number
        fnum1 that represents an angle in radians. The function
        returns the sine of that number as a result and puts
        the cosine of that number into *pfnum2 which must represent
        a valid pointer to a ffp-number.

    INPUTS

    RESULT
        *pfnum2       - Motorola fast floating point number
        direct result - Motorola fast floating point number

    BUGS

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    *pfnum2 = SPCos(fnum1);
    return SPSin(fnum1);
    
    AROS_LIBFUNC_EXIT
}
