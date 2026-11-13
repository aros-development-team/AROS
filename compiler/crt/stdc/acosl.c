/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    Desc: C99 function acos
*/

#include <math.h>
#include <errno.h>
#include <fenv.h>

#include <math_private.h>

#if LDBL_MANT_DIG != DBL_MANT_DIG
/*****************************************************************************

    NAME */
#include <math.h>

        long double acosl(

/*  SYNOPSIS */
        long double x)

/*  FUNCTION
        Computes the arc cosine (inverse cosine) of the input value `x`.

    INPUTS
        x - the value whose arc cosine is to be calculated. Must be in the
            range [-1, 1].

    RESULT
        Returns the arc cosine of `x`, in radians, in the range [0, p].
        If `x` is outside the valid domain, returns NaN and sets errno to EDOM.

    NOTES
        For values outside [-1, 1], the result is a domain error.
        For x == ±1, the result is 0 or p respectively.

    EXAMPLE
        double angle = acos(0.5);  // returns ~1.047 (60 degrees in radians)

    BUGS
        None known.

    SEE ALSO
        cos(), asin(), atan(), acosf(), acosl()

    INTERNALS
        Forwards to __ieee754_acos() unless input is invalid or exceptional.

******************************************************************************/
{
    FORWARD_IF_NAN_OR_INF(acosl, x);
    if (x < -1.0 || x > 1.0) {
        errno = EDOM;
        feraiseexcept(FE_INVALID);
        return NAN;
    }
    return __ieee754_acosl(x);
}
#endif
