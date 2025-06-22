/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    Desc: C99 function acosh
*/

#include <math.h>
#include <errno.h>
#include <fenv.h>

#include <math_private.h>

/*****************************************************************************

    NAME */
#include <math.h>

        double acosh(

/*  SYNOPSIS */
        double x)

/*  FUNCTION
        Computes the inverse hyperbolic cosine of `x`.

    INPUTS
        x - the value whose inverse hyperbolic cosine is to be calculated.
            Must be greater than or equal to 1.

    RESULT
        Returns the inverse hyperbolic cosine of `x`.
        If `x` < 1, returns NaN and sets errno to EDOM.

    NOTES
        The result is always non-negative.

    EXAMPLE
        double r = acosh(2.0);  // returns ~1.317

    BUGS
        None known.

    SEE ALSO
        cosh(), asinh(), atanh(), acoshf(), acoshl()

    INTERNALS
        Forwards to __ieee754_acosh() with domain checking.

******************************************************************************/
{
    FORWARD_IF_NAN_OR_INF(acosh, x);
    if (x < 1.0) {
        errno = EDOM;
        feraiseexcept(FE_INVALID);
        return NAN;
    }
    return __ieee754_acosh(x);
}
