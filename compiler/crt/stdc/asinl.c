/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    Desc: C99 function asin
*/

#include <math.h>
#include <errno.h>
#include <fenv.h>

#include <math_private.h>

#if (LDBL_MANT_DIG != DBL_MANT_DIG)
/*****************************************************************************

    NAME */
#include <math.h>

        long double asinl(

/*  SYNOPSIS */
        long double x)

/*  FUNCTION
        Computes the arc sine (inverse sine) of the input value `x`.

    INPUTS
        x - the value whose arc sine is to be calculated. Must be in the
            range [-1, 1].

    RESULT
        Returns the arc sine of `x`, in radians, in the range [-p/2, p/2].
        If `x` is outside the valid domain, returns NaN and sets errno to EDOM.

    NOTES
        For values outside [-1, 1], the result is a domain error.

    EXAMPLE
        double angle = asin(0.5);  // returns ~0.524 (30 degrees in radians)

    BUGS
        None known.

    SEE ALSO
        sin(), acos(), atan(), asinf(), asinl()

    INTERNALS
        Forwards to __ieee754_asin() with domain checks and error handling.

******************************************************************************/
{
    FORWARD_IF_NAN_OR_INF(asinl, x);
    if (x < -1.0 || x > 1.0) {
        errno = EDOM;
        feraiseexcept(FE_INVALID);
        return NAN;
    }
    return __ieee754_asinl(x);
}
#endif
