/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    Desc: C99 function atanh
*/

#include <math.h>
#include <errno.h>
#include <fenv.h>

#include <math_private.h>

#if LDBL_MANT_DIG != DBL_MANT_DIG
/*****************************************************************************

    NAME */
#include <math.h>

        long double atanhl(

/*  SYNOPSIS */
        long double x)

/*  FUNCTION
        Computes the inverse hyperbolic tangent of `x`.

    INPUTS
        x - the value whose inverse hyperbolic tangent is to be calculated.
            Must be in the range (-1, 1).

    RESULT
        Returns the inverse hyperbolic tangent of `x`.
        Returns NaN and sets errno to EDOM if |x| > 1.
        Returns ±infinity and sets errno to ERANGE if |x| == 1.

    NOTES
        The function is odd: atanh(-x) = -atanh(x).

    EXAMPLE
        double r = atanh(0.5);  // returns ~0.549

    BUGS
        None known.

    SEE ALSO
        tanh(), asinh(), acosh(), atanhf(), atanhl()

    INTERNALS
        Forwards to __ieee754_atanh() with domain and range checks.

******************************************************************************/
{
    FORWARD_IF_NAN_OR_INF(atanhl, x);

    double absx = fabs(x);
    if (absx > 1.0) {
        errno = EDOM;
        feraiseexcept(FE_INVALID);
        return NAN;
    } else if (absx == 1.0) {
        errno = ERANGE;
        feraiseexcept(FE_OVERFLOW);
        return copysign(INFINITY, x);
    }

    return __ieee754_atanhl(x);
}
#endif
