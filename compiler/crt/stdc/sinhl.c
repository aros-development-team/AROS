/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    Desc: C99 function sinh
*/

#include <math.h>
#include <errno.h>
#include <fenv.h>

#include <math_private.h>

#if LDBL_MANT_DIG != DBL_MANT_DIG
/*****************************************************************************

    NAME */
#include <math.h>

        long double sinhl(

/*  SYNOPSIS */
        long double x)

/*  FUNCTION
        Computes the hyperbolic sine of x.

    INPUTS
        x - input value.

    RESULT
        Returns sinh(x) = (e^x - e^{-x}) / 2.

    NOTES
        Uses exponentials internally for calculation.

    EXAMPLE
        double val = sinh(1.0);

    BUGS
        None known.

    SEE ALSO
        cosh(), tanh(), sinh()

    INTERNALS
        Forwards to __ieee754_sinhl().

******************************************************************************/
{
    FORWARD_IF_NAN_OR_INF(sinhl, x);
    return __ieee754_sinhl(x);
}
#endif