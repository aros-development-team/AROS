/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "mathieeesingtrans_intern.h"

/*****************************************************************************

    NAME */

        AROS_LH2(float, IEEESPSincos,

/*  SYNOPSIS */
        AROS_LHA(float *, z, A0),
        AROS_LHA(float, y, D0),

/*  LOCATION */
        struct Library *, MathIeeeSingTransBase, 9, MathIeeeSingTrans)

/*  FUNCTION
        Calculate the cosine and the sine of the given IEEE single
        precision number where y represents an angle in radians. The 
        function returns the sine of that number as a result and puts
        the cosine of that number into *z which must represent
        a valid pointer to a IEEE single precision number.

    INPUTS

    RESULT
        *z            - IEEE single precision floating point number
        direct result - IEEE single precision floating point number

    BUGS

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    *z = IEEESPCos(y);
    return IEEESPSin(y);

    AROS_LIBFUNC_EXIT
}
