/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "mathieeedoubtrans_intern.h"

/*****************************************************************************

    NAME */

        AROS_LH1QUAD1(double, IEEEDPSincos,

/*  SYNOPSIS */
        AROS_LHA(double *, z, A0),
        AROS_LHAQUAD(double, y, D0, D1),

/*  LOCATION */
        struct MathIeeeDoubTransBase *, MathIeeeDoubTransBase, 9, MathIeeeDoubTrans)

/*  FUNCTION
        Calculate the cosine and the sine of the given IEEE double
        precision number where y represents an angle in radians. The 
        function returns the sine of that number as a result and puts
        the cosine of that number into *z which must represent
        a valid pointer to a IEEE double precision number.

    INPUTS

    RESULT
        *z            - IEEE double precision floating point number
        direct result - IEEE double precision floating point number

    BUGS

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    *z = IEEEDPCos(y);
    return IEEEDPSin(y);

    AROS_LIBFUNC_EXIT
}
